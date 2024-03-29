/* Copyright 2024 AISys. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <sys/time.h>

#include "dpu_region_address_translation.h"
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dpu.h>
#include <errno.h>
#include <limits.h>
#include <numa.h>
#include <numaif.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <sys/sysinfo.h>
#include <time.h>
#include "static_verbose.h"
#include <omp.h>

#define BANK_OFFSET_NEXT_DATA_a2a(i) (i * 16) // For each 64bit word, you must jump 16 * 64bit (2 cache lines)
#define BANK_CHUNK_SIZE_a2a 0x20000 //128kb
#define BANK_NEXT_CHUNK_OFFSET_a2a 0x100000 //1mb
#define MRAM_SIZE_a2a 0x4000000 // 64MB


#define CACHE_LINE 64
#define CACHE_LINE2 128


static uint32_t apply_address_translation_on_mram_offset_a2a(uint32_t byte_offset)
{
    /* We have observed that, within the 26 address bits of the MRAM address, we need to apply an address translation:
     *
     * virtual[13: 0] = physical[13: 0]
     * virtual[20:14] = physical[21:15]
     * virtual[   21] = physical[   14]
     * virtual[25:22] = physical[25:22]
     *
     * This function computes the "virtual" mram address based on the given "physical" mram address.
     */

    uint32_t mask_21_to_15 = ((1 << (21 - 15 + 1)) - 1) << 15;
    uint32_t mask_21_to_14 = ((1 << (21 - 14 + 1)) - 1) << 14;
    uint32_t bits_21_to_15 = (byte_offset & mask_21_to_15) >> 15;
    uint32_t bit_14 = (byte_offset >> 14) & 1;
    uint32_t unchanged_bits = byte_offset & ~mask_21_to_14;

    return unchanged_bits | (bits_21_to_15 << 14) | (bit_14 << 21);
}

/////////////////////////////////////////////////////////

#define RNS_FLUSH_DST(iter, dst_rank_bgwise_addr)                                  \
    do                                                                             \
    {                                                                              \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                        \
        void *dst_rank_clwise_addr2 = dst_rank_bgwise_addr + (CACHE_LINE2 * iter); \
                                                                                   \
        for (int cl = 0; cl < iter; cl++)                                          \
        {                                                                          \
            __builtin_ia32_clflushopt((uint8_t *)dst_rank_clwise_addr1);           \
            __builtin_ia32_clflushopt((uint8_t *)(dst_rank_clwise_addr2));         \
            dst_rank_clwise_addr1 += CACHE_LINE2;                                  \
            dst_rank_clwise_addr2 += CACHE_LINE2;                                  \
        }                                                                          \
    } while (0)

#define RNS_FLUSH_SRC(iter, src_rank_bgwise_addr)                                      \
    do                                                                                 \
    {                                                                                  \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                             \
                                                                                       \
        for (int cl = 0; cl < iter; cl++)                                              \
        {                                                                              \
            __builtin_ia32_clflushopt((uint8_t *)src_rank_clwise_addr);                \
            __builtin_ia32_clflushopt((uint8_t *)(src_rank_clwise_addr + CACHE_LINE)); \
            src_rank_clwise_addr += CACHE_LINE2;                                       \
        }                                                                              \
    } while (0)

#define RNS_SRC_BG_a2a 4
#define RNS_TAR_BG_a2a 8
#define chip_rank_a2a 64


#define RNS_COPY_a2a(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr)  \
    do                                                                                      \
    {                                                                                       \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                  \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                                 \
                                                                                            \
        __m512i reg1;                                                                       \
         __m512i reg1_rot;                                                                  \
                                                                                            \
        for (int cl = 0; cl < iter; cl++)                                                   \
        {                                                                                   \
            reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                \
            reg1_rot = _mm512_rol_epi64(reg1, rotate_bit);                                  \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                     \
        }                                                                                   \
    } while (0)

#define RNS_COPY_a2a_24(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, a_length) \
    do                                                                                                  \
    {                                                                                                   \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                              \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                                             \
                                                                                                        \
        __m512i reg1;                                                                                   \
        __m512i reg1_rot;                                                                               \
                                                                                                        \
        for (int cl = 0; cl < iter; cl++)                                                               \
        {                                                                                               \
            reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                            \
            if(a_length==4) {                                                                           \
                reg1_rot = _mm512_rol_epi32(reg1, rotate_bit);                                          \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                         \
            }                                                                                           \
            else if(a_length==2) {                                                                      \
                if(rotate==0) {                                                                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1);                         \
                }                                                                                       \
                else if(rotate==1) {                                                                    \
                    __m512i mask = _mm512_set_epi64(                                                    \
                                                    0x0e0f0c0d0a0b0809ULL,                              \
                                                    0x0607040502030001ULL,                              \
                                                    0x0e0f0c0d0a0b0809ULL,                              \
                                                    0x0607040502030001ULL,                              \
                                                    0x0e0f0c0d0a0b0809ULL,                              \
                                                    0x0607040502030001ULL,                              \
                                                    0x0e0f0c0d0a0b0809ULL,                              \
                                                    0x0607040502030001ULL);                             \
                    reg1_rot = _mm512_shuffle_epi8(reg1, mask);                                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                     \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define RNS_COPY_a2a_22_xz(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr) \
    do                                                                                                  \
    {                                                                                                   \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                              \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                                             \
                                                                                                        \
        __m512i reg1;                                                                                   \
        __m512i reg1_rot;                                                                               \
                                                                                                        \
        for (int cl = 0; cl < iter; cl++)                                                               \
        {                                                                                               \
            reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                            \
            if(rotate==0) {                                                                             \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1);                             \
            }                                                                                           \
            else if(rotate==1) {                                                                        \
                __m512i mask = _mm512_set_epi64(                                                        \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL);                                 \
                reg1_rot = _mm512_shuffle_epi8(reg1, mask);                                             \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                         \
            }                                                                                           \
            else if(rotate == 4){                                                                       \
                reg1_rot = _mm512_rol_epi64(reg1, rotate_bit);                                          \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                         \
            }                                                                                           \
            else if(rotate == 5){                                                                       \
                reg1_rot = _mm512_rol_epi64(reg1, rotate_bit);                                          \
                __m512i mask = _mm512_set_epi64(                                                        \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL,                                  \
                                                0x0e0f0c0d0a0b0809ULL,                                  \
                                                0x0607040502030001ULL);                                 \
                reg1_rot = _mm512_shuffle_epi8(reg1_rot, mask);                                         \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                         \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define RNS_COPY_a2a_22_y(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr) \
    do                                                                                                  \
    {                                                                                                   \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                              \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                                             \
                                                                                                        \
        __m512i reg1;                                                                                   \
        __m512i reg1_rot;                                                                               \
                                                                                                        \
        for (int cl = 0; cl < iter; cl++)                                                               \
        {                                                                                               \
            reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                            \
            if(rotate==0) {                                                                             \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1);                             \
            }                                                                                           \
            else if(rotate==1){                                                                         \
                reg1_rot = _mm512_rol_epi32(reg1, rotate_bit);                                          \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1_rot);                         \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define S_COPY_a2a(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr)    \
    do                                                                                      \
    {                                                                                       \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                  \
        void *dst_rank_clwise_addr1 = dst_rank_bgwise_addr;                                 \
                                                                                            \
        __m512i reg1;                                                                       \
                                                                                            \
        for (int cl = 0; cl < iter; cl++)                                                   \
        {                                                                                   \
            reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1), reg1);                     \
        }                                                                                   \
    } while (0)

#define COMM_FLUSH_a2a(iter, src_rank_bgwise_addr)                                      \
    do                                                                                  \
    {                                                                                   \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                              \
                                                                                        \
        for (int cl = 0; cl < iter; cl++)                                               \
        {                                                                               \
            __builtin_ia32_clflushopt((uint8_t *)src_rank_clwise_addr);                 \
                                                                                        \
            src_rank_clwise_addr += CACHE_LINE2;                                        \
        }                                                                               \
    } while (0)

#define RNS_COPY_ag(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array)     \
    do                                                                                              \
    {                                                                                               \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                          \
        void *dst_rank_clwise_addr1[8*iter];                                                        \
        __m512i reg1;                                                                               \
                                                                                                    \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                            \
                                                                                                    \
        for (uint32_t cl = 0; cl < 8*iter; cl++)                                                    \
        {                                                                                           \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                             \
        }                                                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+0]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+1]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+2]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+3]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+4]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+5]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+6]), reg1);                     \
        }                                                                                           \
        reg1 = _mm512_rol_epi64(reg1, 8);                                                           \
        for(uint32_t cl=0; cl<iter; cl++){                                                          \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[8*cl+7]), reg1);                     \
        }                                                                                           \
    } while (0)

#define RNS_COPY_ag_24(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array, a_length)    \
    do                                                                                                          \
    {                                                                                                           \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                                      \
        void *dst_rank_clwise_addr1[a_length*iter];                                                             \
        __m512i reg1;                                                                                           \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                                        \
                                                                                                                \
        for (uint32_t cl = 0; cl < a_length*iter; cl++)                                                         \
        {                                                                                                       \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                                         \
        }                                                                                                       \
                                                                                                                \
        if(a_length == 4) reg1 = _mm512_rol_epi32(reg1, 24);                                                    \
        else if(a_length == 2) {                                                                                \
            __m512i mask = _mm512_set_epi64(                                                                    \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL);                                             \
            reg1 = _mm512_shuffle_epi8(reg1, mask);                                                             \
        }                                                                                                       \
                                                                                                                \
        if(a_length==4){                                                                                        \
            reg1 = _mm512_rol_epi32(reg1, 8);                                                                   \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 0]), reg1);                    \
            }                                                                                                   \
            reg1 = _mm512_rol_epi32(reg1, 8);                                                                   \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 1]), reg1);                    \
            }                                                                                                   \
            reg1 = _mm512_rol_epi32(reg1, 8);                                                                   \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 2]), reg1);                    \
            }                                                                                                   \
            reg1 = _mm512_rol_epi32(reg1, 8);                                                                   \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 3]), reg1);                    \
            }                                                                                                   \
        }                                                                                                       \
        else if(a_length==2){                                                                                   \
            __m512i mask = _mm512_set_epi64(                                                                    \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL);                                             \
            reg1 = _mm512_shuffle_epi8(reg1, mask);                                                             \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 0]), reg1);                    \
            }                                                                                                   \
            reg1 = _mm512_shuffle_epi8(reg1, mask);                                                             \
            for(uint32_t cl=0; cl<iter; cl++){                                                                  \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 1]), reg1);                    \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)

#define RNS_COPY_ag_22(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array, a_length)    \
    do                                                                                                          \
    {                                                                                                           \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                                      \
        void *dst_rank_clwise_addr1[a_length*iter];                                                             \
        __m512i reg1;                                                                                           \
                                                                                                                \
        __m512i mask = _mm512_set_epi64(                                                                        \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL,                                              \
                                            0x0e0f0c0d0a0b0809ULL,                                              \
                                            0x0607040502030001ULL);                                             \
                                                                                                                \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                                        \
                                                                                                                \
        for (uint32_t cl = 0; cl < a_length*iter; cl++)                                                         \
        {                                                                                                       \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                                         \
        }                                                                                                       \
        for(uint32_t cl=0; cl<iter; cl++){                                                                      \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 0]), reg1);                        \
        }                                                                                                       \
        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                                 \
        for(uint32_t cl=0; cl<iter; cl++){                                                                      \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 1]), reg1);                        \
        }                                                                                                       \
        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                                 \
        reg1 = _mm512_rol_epi64(reg1, 32);                                                                      \
        for(uint32_t cl=0; cl<iter; cl++){                                                                      \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 2]), reg1);                        \
        }                                                                                                       \
        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                                 \
        for(uint32_t cl=0; cl<iter; cl++){                                                                      \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[a_length*cl + 3]), reg1);                        \
        }                                                                                                       \
    } while (0)


#define S_COPY_ag(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array)   \
    do                                                                                          \
    {                                                                                           \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                      \
        void *dst_rank_clwise_addr1[iter];                                                      \
        __m512i reg1;                                                                           \
                                                                                                \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                        \
                                                                                                \
        for (uint32_t cl = 0; cl < iter; cl++)                                                  \
        {                                                                                       \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                         \
        }                                                                                       \
        for (uint32_t cl = 0; cl < iter; cl++)                                                  \
        {                                                                                       \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[cl]), reg1);                     \
        }                                                                                       \
    } while (0)

#define S_COPY_ag_24(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array, a_length)  \
    do                                                                                                      \
    {                                                                                                       \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                                  \
        void *dst_rank_clwise_addr1[(8/a_length)*iter];                                                     \
        __m512i reg1;                                                                                       \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                                    \
                                                                                                            \
        for (uint32_t cl = 0; cl < (8/a_length)*iter; cl++)                                                 \
        {                                                                                                   \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                                     \
        }                                                                                                   \
                                                                                                            \
        if(a_length == 4) reg1 = _mm512_rol_epi64(reg1, 32);                                                \
        else if(a_length == 2) reg1 = _mm512_rol_epi64(reg1, 48);                                           \
                                                                                                            \
        if(a_length==4){                                                                                    \
            reg1 = _mm512_rol_epi64(reg1, 32);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+0]), reg1);              \
            }                                                                                               \
            reg1 = _mm512_rol_epi64(reg1, 32);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+1]), reg1);              \
            }                                                                                               \
        }                                                                                                   \
        else if(a_length==2){                                                                               \
            reg1 = _mm512_rol_epi64(reg1, 16);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+0]), reg1);              \
            }                                                                                               \
            reg1 = _mm512_rol_epi64(reg1, 16);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+1]), reg1);              \
            }                                                                                               \
            reg1 = _mm512_rol_epi64(reg1, 16);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+2]), reg1);              \
            }                                                                                               \
            reg1 = _mm512_rol_epi64(reg1, 16);                                                              \
            for(uint32_t cl=0; cl<iter; cl++){                                                              \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+3]), reg1);              \
            }                                                                                               \
        }                                                                                                   \
    } while (0)

#define S_COPY_ag_22(rotate, rotate_bit, iter, src_rank_bgwise_addr, dst_rank_bgwise_addr_array, a_length)  \
    do                                                                                                      \
    {                                                                                                       \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                                  \
        void *dst_rank_clwise_addr1[(8/a_length)*iter];                                                     \
        __m512i reg1;                                                                                       \
        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr));                                    \
                                                                                                            \
        for (uint32_t cl = 0; cl < (8/a_length)*iter; cl++)                                                 \
        {                                                                                                   \
            dst_rank_clwise_addr1[cl] = dst_rank_bgwise_addr_array[cl];                                     \
        }                                                                                                   \
                                                                                                            \
        for(uint32_t cl=0; cl<iter; cl++){                                                                  \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+0]), reg1);                  \
        }                                                                                                   \
        reg1 = _mm512_rol_epi32(reg1, 16);                                                                  \
        for(uint32_t cl=0; cl<iter; cl++){                                                                  \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr1[(8/a_length)*cl+1]), reg1);                  \
        }                                                                                                   \
    } while (0)

#define GATHER_COPY(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr)                           \
    do                                                                                          \
    {                                                                                           \
        uint64_t *src_rank_clwise_addr = src_rank_bgwise_addr;                                  \
        void *dst_rank_clwise_addr = dst_rank_bgwise_addr;                                      \
        __m512i reg1;                                                                           \
                                                                                                \
        __m512i mask = _mm512_set_epi64(                                                        \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL);                                 \
        __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                           \
                                                1, 3, 5, 7, 9, 11, 13, 15);                     \
        __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15); \
                                                                                                \
        for(uint32_t i=0; i<1; i++){                                                            \
            reg1 = _mm512_loadu_si512((void *)(src_rank_clwise_addr));                          \
                                                                                                \
            reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
            reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
            reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                \
            __mmask8 mask_all = 0xFF;                                                           \
            _mm512_mask_storeu_epi64((__m512i*)dst_rank_clwise_addr, mask_all, reg1);           \
        }                                                                                       \
    } while (0)


#define SCATTER_COPY(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr)                          \
    do                                                                                          \
    {                                                                                           \
        __m512i mask = _mm512_set_epi64(                                                        \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL);                                 \
        __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                           \
                                                1, 3, 5, 7, 9, 11, 13, 15);                     \
        __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15); \
        void *src_rank_clwise_addr = src_rank_bgwise_addr;                                      \
        uint64_t *dst_rank_clwise_addr = dst_rank_bgwise_addr;                                  \
        __m512i reg1;                                                                           \
        for(uint32_t i=0; i<1; i++){                                                            \
            reg1 = _mm512_loadu_si512((void *)(src_rank_clwise_addr));                          \
                                                                                                \
            reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
            reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
            reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                \
            _mm512_stream_si512((void *)(dst_rank_clwise_addr), reg1);                          \
        } \
    } while (0)

#define REDUCE_S_SUM_RS(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size)   \
    do                                                                                          \
    {                                                                                           \
        __m512i reg1;                                                                           \
                                                                                                \
        void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                     \
                                                                                                \
        __m512i sum = _mm512_set_epi64(                                                         \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL);                                 \
                                                                                                \
        __m512i mask = _mm512_set_epi64(                                                        \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL);                                 \
       __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                            \
                                                1, 3, 5, 7, 9, 11, 13, 15);                     \
        __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15); \
                                                                                                \
        if(size == 1){                                                                          \
            for (int cl = 0; cl < iter; cl++)                                                   \
            {                                                                                   \
                                                                                                \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                      \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    sum = _mm512_add_epi8(sum, reg1);                                           \
                }                                                                               \
                                                                                                \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                    \
                sum = _mm512_shuffle_epi8(sum, mask);                                           \
                sum = _mm512_permutexvar_epi32(perm, sum);                                      \
                __mmask8 mask_all = 0xFF;                                                       \
                _mm512_mask_storeu_epi64((__m512i*)dst_rank_clwise_addr9, mask_all, sum);       \
            }                                                                                   \
        }                                                                                       \
        else{                                                                                   \
            for (int cl = 0; cl < iter; cl++)                                                   \
            {                                                                                   \
                                                                                                \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                      \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                              \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                     \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                \
                                                                                                \
                    if(size == 2) sum = _mm512_add_epi16(sum, reg1);                            \
                    else if(size == 4) sum = _mm512_add_epi32(sum, reg1);                       \
                    else sum = _mm512_add_epi64(sum, reg1);                                     \
                }                                                                               \
                __mmask8 mask_all = 0xFF;                                                       \
                _mm512_mask_storeu_epi64((__m512i*)dst_rank_clwise_addr9, mask_all, sum);       \
            }                                                                                   \
        }                                                                                       \
    } while (0)

#define REDUCE_RNS_SUM_RS(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size)     \
    do                                                                                              \
    {                                                                                               \
                                                                                                    \
                                                                                                    \
        __m512i reg1;                                                                               \
        __m512i reg2;                                                                               \
        __m512i reg3;                                                                               \
        __m512i reg4;                                                                               \
        __m512i reg5;                                                                               \
        __m512i reg6;                                                                               \
        __m512i reg7;                                                                               \
        __m512i reg8;                                                                               \
                                                                                                    \
        void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                         \
                                                                                                    \
                                                                                                    \
        __m512i sum = _mm512_set_epi64(                                                             \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL,                                      \
                                        0x0000000000000000ULL);                                     \
                                                                                                    \
        __m512i mask = _mm512_set_epi64(                                                            \
                                        0x0f0b07030e0a0602ULL,                                      \
                                        0x0d0905010c080400ULL,                                      \
                                        0x0f0b07030e0a0602ULL,                                      \
                                        0x0d0905010c080400ULL,                                      \
                                        0x0f0b07030e0a0602ULL,                                      \
                                        0x0d0905010c080400ULL,                                      \
                                        0x0f0b07030e0a0602ULL,                                      \
                                        0x0d0905010c080400ULL);                                     \
        __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                         \
        __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);     \
                                                                                                    \
        if(size == 1){                                                                              \
            for (int cl = 0; cl < iter; cl++)                                                       \
            {                                                                                       \
                                                                                                    \
                for(uint32_t i=0; i<num_iter_src; i++){                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                        \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                      \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                      \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                      \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                      \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                      \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                      \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                      \
                                                                                                    \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                                                                                                    \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                               \
                                                                                                    \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                              \
                                                                                                    \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                              \
                                                                                                    \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));               \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                              \
                                                                                                    \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));               \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                              \
                                                                                                    \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));               \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                              \
                                                                                                    \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));               \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                              \
                                                                                                    \
                    sum = _mm512_add_epi8(sum, reg1);                                               \
                    sum = _mm512_add_epi8(sum, reg2);                                               \
                    sum = _mm512_add_epi8(sum, reg3);                                               \
                    sum = _mm512_add_epi8(sum, reg4);                                               \
                    sum = _mm512_add_epi8(sum, reg5);                                               \
                    sum = _mm512_add_epi8(sum, reg6);                                               \
                    sum = _mm512_add_epi8(sum, reg7);                                               \
                    sum = _mm512_add_epi8(sum, reg8);                                               \
                                                                                                    \
                }                                                                                   \
                                                                                                    \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                        \
                sum = _mm512_shuffle_epi8(sum, mask);                                               \
                sum = _mm512_permutexvar_epi32(perm, sum);                                          \
                __mmask8 mask_all = 0xFF;                                                           \
                _mm512_mask_storeu_epi64((__m512i*)dst_rank_clwise_addr9, mask_all, sum);           \
            }                                                                                       \
        }                                                                                           \
        else{                                                                                       \
            for (int cl = 0; cl < iter; cl++)                                                       \
            {                                                                                       \
                                                                                                    \
                for(uint32_t i=0; i<num_iter_src; i++){                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                        \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                      \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                      \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                      \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                      \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                      \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                      \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                      \
                                                                                                    \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                  \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                         \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                    \
                                                                                                    \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                               \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                  \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                         \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                    \
                                                                                                    \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                              \
                    reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                  \
                    reg3 = _mm512_shuffle_epi8(reg3, mask);                                         \
                    reg3 = _mm512_permutexvar_epi32(perm, reg3);                                    \
                                                                                                    \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                              \
                    reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                  \
                    reg4 = _mm512_shuffle_epi8(reg4, mask);                                         \
                    reg4 = _mm512_permutexvar_epi32(perm, reg4);                                    \
                                                                                                    \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));               \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                              \
                    reg5 = _mm512_permutexvar_epi32(vindex, reg5);                                  \
                    reg5 = _mm512_shuffle_epi8(reg5, mask);                                         \
                    reg5 = _mm512_permutexvar_epi32(perm, reg5);                                    \
                                                                                                    \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));               \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                              \
                    reg6 = _mm512_permutexvar_epi32(vindex, reg6);                                  \
                    reg6 = _mm512_shuffle_epi8(reg6, mask);                                         \
                    reg6 = _mm512_permutexvar_epi32(perm, reg6);                                    \
                                                                                                    \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));               \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                              \
                    reg7 = _mm512_permutexvar_epi32(vindex, reg7);                                  \
                    reg7 = _mm512_shuffle_epi8(reg7, mask);                                         \
                    reg7 = _mm512_permutexvar_epi32(perm, reg7);                                    \
                                                                                                    \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));               \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                              \
                    reg8 = _mm512_permutexvar_epi32(vindex, reg8);                                  \
                    reg8 = _mm512_shuffle_epi8(reg8, mask);                                         \
                    reg8 = _mm512_permutexvar_epi32(perm, reg8);                                    \
                                                                                                    \
                    if(size == 2){                                                                  \
                        sum = _mm512_add_epi16(sum, reg1);                                          \
                        sum = _mm512_add_epi16(sum, reg2);                                          \
                        sum = _mm512_add_epi16(sum, reg3);                                          \
                        sum = _mm512_add_epi16(sum, reg4);                                          \
                        sum = _mm512_add_epi16(sum, reg5);                                          \
                        sum = _mm512_add_epi16(sum, reg6);                                          \
                        sum = _mm512_add_epi16(sum, reg7);                                          \
                        sum = _mm512_add_epi16(sum, reg8);                                          \
                    }                                                                               \
                    else if(size == 4){                                                             \
                        sum = _mm512_add_epi32(sum, reg1);                                          \
                        sum = _mm512_add_epi32(sum, reg2);                                          \
                        sum = _mm512_add_epi32(sum, reg3);                                          \
                        sum = _mm512_add_epi32(sum, reg4);                                          \
                        sum = _mm512_add_epi32(sum, reg5);                                          \
                        sum = _mm512_add_epi32(sum, reg6);                                          \
                        sum = _mm512_add_epi32(sum, reg7);                                          \
                        sum = _mm512_add_epi32(sum, reg8);                                          \
                    }                                                                               \
                    else{                                                                           \
                        sum = _mm512_add_epi64(sum, reg1);                                          \
                        sum = _mm512_add_epi64(sum, reg2);                                          \
                        sum = _mm512_add_epi64(sum, reg3);                                          \
                        sum = _mm512_add_epi64(sum, reg4);                                          \
                        sum = _mm512_add_epi64(sum, reg5);                                          \
                        sum = _mm512_add_epi64(sum, reg6);                                          \
                        sum = _mm512_add_epi64(sum, reg7);                                          \
                        sum = _mm512_add_epi64(sum, reg8);                                          \
                    }                                                                               \
                                                                                                    \
                }                                                                                   \
                                                                                                    \
                __mmask8 mask_all = 0xFF;                                                           \
                _mm512_mask_storeu_epi64((__m512i*)dst_rank_clwise_addr9, mask_all, sum);           \
            }                                                                                       \
        }                                                                                           \
    } while (0)



#define S_SUM_RS(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size)          \
    do                                                                                          \
    {                                                                                           \
        __m512i reg1;                                                                           \
                                                                                                \
        void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                     \
                                                                                                \
        __m512i sum = _mm512_set_epi64(                                                         \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL);                                 \
                                                                                                \
        if(size == 1){                                                                          \
            for (int cl = 0; cl < iter; cl++)                                                   \
            {                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                      \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    sum = _mm512_add_epi8(sum, reg1);                                           \
                }                                                                               \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
            }                                                                                   \
        }                                                                                       \
        else{                                                                                   \
            __m512i mask = _mm512_set_epi64(                                                    \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL);                                 \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                       \
                                                1, 3, 5, 7, 9, 11, 13, 15);                     \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8,                         \
                                                12, 9, 13, 10, 14, 11, 15);                     \
                                                                                                \
            for (int cl = 0; cl < iter; cl++){                                                  \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                      \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                              \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                     \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                \
                                                                                                \
                    if(size == 2) sum = _mm512_add_epi16(sum, reg1);                            \
                    else if(size == 4) sum = _mm512_add_epi32(sum, reg1);                       \
                    else sum = _mm512_add_epi64(sum, reg1);                                     \
                }                                                                               \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                    \
                sum = _mm512_shuffle_epi8(sum, mask);                                           \
                sum = _mm512_permutexvar_epi32(perm, sum);                                      \
                                                                                                \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
            }                                                                                   \
        }                                                                                       \
    } while (0)

#define REDUCE_SCATTER_CPU_Y_24(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length) \
    do                                                                                                          \
    {                                                                                                           \
        a_length+=0;                                                                                            \
        __m512i reg1;                                                                                           \
        __m512i reg2;                                                                                           \
        __m512i reg3;                                                                                           \
        __m512i reg4;                                                                                           \
                                                                                                                \
                                                                                                                \
                                                                                                                \
        __m512i sum = _mm512_set_epi64(                                                                         \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL);                                                 \
                                                                                                                \
        if(size == 1){                                                                                          \
            for (int cl = 0; cl < iter; cl++)                                                                   \
            {                                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                         \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];                       \
                    if(a_length==2){                                                                            \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[(8/a_length)*i+2];                   \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[(8/a_length)*i+3];                   \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                       \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                                       \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                       \
                        reg2 = _mm512_rol_epi64(reg2, 16);                                                      \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                       \
                        reg3 = _mm512_rol_epi64(reg3, 32);                                                      \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                       \
                        reg4 = _mm512_rol_epi64(reg4, 48);                                                      \
                        sum = _mm512_add_epi8(sum, reg1);                                                       \
                        sum = _mm512_add_epi8(sum, reg2);                                                       \
                        sum = _mm512_add_epi8(sum, reg3);                                                       \
                        sum = _mm512_add_epi8(sum, reg4);                                                       \
                    }                                                                                           \
                    else if(a_length==4){                                                                       \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                       \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                                       \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                       \
                        reg2 = _mm512_rol_epi64(reg2, 32);                                                      \
                        sum = _mm512_add_epi8(sum, reg1);                                                       \
                        sum = _mm512_add_epi8(sum, reg2);                                                       \
                    }                                                                                           \
                }                                                                                               \
                for(uint32_t i=0; i<1; i++){                                                                    \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                  \
                }                                                                                               \
            }                                                                                                   \
        }                                                                                                       \
        else{                                                                                                   \
            __m512i mask = _mm512_set_epi64(                                                                    \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL);                                             \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                                       \
                                                1, 3, 5, 7, 9, 11, 13, 15);                                     \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);             \
                                                                                                                \
            for (int cl = 0; cl < iter; cl++)                                                                   \
            {                                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                         \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];                       \
                    if(a_length==2){                                                                            \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[(8/a_length)*i+2];                   \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[(8/a_length)*i+3];                   \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                       \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                                       \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                          \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                 \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                            \
                                                                                                                \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                       \
                        reg2 = _mm512_rol_epi64(reg2, 16);                                                      \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                          \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                                 \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                            \
                                                                                                                \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                       \
                        reg3 = _mm512_rol_epi64(reg3, 32);                                                      \
                        reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                          \
                        reg3 = _mm512_shuffle_epi8(reg3, mask);                                                 \
                        reg3 = _mm512_permutexvar_epi32(perm, reg3);                                            \
                                                                                                                \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                       \
                        reg4 = _mm512_rol_epi64(reg4, 48);                                                      \
                        reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                          \
                        reg4 = _mm512_shuffle_epi8(reg4, mask);                                                 \
                        reg4 = _mm512_permutexvar_epi32(perm, reg4);                                            \
                                                                                                                \
                        if(size==2){                                                                            \
                            sum = _mm512_add_epi16(sum, reg1);                                                  \
                            sum = _mm512_add_epi16(sum, reg2);                                                  \
                            sum = _mm512_add_epi16(sum, reg3);                                                  \
                            sum = _mm512_add_epi16(sum, reg4);                                                  \
                        }                                                                                       \
                        else if(size==4){                                                                       \
                            sum = _mm512_add_epi32(sum, reg1);                                                  \
                            sum = _mm512_add_epi32(sum, reg2);                                                  \
                            sum = _mm512_add_epi32(sum, reg3);                                                  \
                            sum = _mm512_add_epi32(sum, reg4);                                                  \
                        }                                                                                       \
                        else if(size==8){                                                                       \
                            sum = _mm512_add_epi64(sum, reg1);                                                  \
                            sum = _mm512_add_epi64(sum, reg2);                                                  \
                            sum = _mm512_add_epi64(sum, reg3);                                                  \
                            sum = _mm512_add_epi64(sum, reg4);                                                  \
                        }                                                                                       \
                    }                                                                                           \
                    else if(a_length==4){                                                                       \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                       \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                                       \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                          \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                 \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                            \
                                                                                                                \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                       \
                        reg2 = _mm512_rol_epi64(reg2, 32);                                                      \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                          \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                                 \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                            \
                                                                                                                \
                        if(size==2){                                                                            \
                            sum = _mm512_add_epi16(sum, reg1);                                                  \
                            sum = _mm512_add_epi16(sum, reg2);                                                  \
                        }                                                                                       \
                        else if(size==4){                                                                       \
                            sum = _mm512_add_epi32(sum, reg1);                                                  \
                            sum = _mm512_add_epi32(sum, reg2);                                                  \
                        }                                                                                       \
                        else if(size==8){                                                                       \
                            sum = _mm512_add_epi64(sum, reg1);                                                  \
                            sum = _mm512_add_epi64(sum, reg2);                                                  \
                        }                                                                                       \
                    }                                                                                           \
                }                                                                                               \
                                                                                                                \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                                    \
                sum = _mm512_shuffle_epi8(sum, mask);                                                           \
                sum = _mm512_permutexvar_epi32(perm, sum);                                                      \
                                                                                                                \
                for(uint32_t i=0; i<1; i++){                                                                    \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                  \
                }                                                                                               \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)

#define REDUCE_SCATTER_CPU_Y_22(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length) \
    do                                                                                                          \
    {                                                                                                           \
        __m512i reg1;                                                                                           \
        __m512i reg2;                                                                                           \
                                                                                                                \
        __m512i sum = _mm512_set_epi64(                                                                         \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL,                                                  \
                                        0x0000000000000000ULL);                                                 \
                                                                                                                \
        if(size == 1){                                                                                          \
            for (int cl = 0; cl < iter; cl++)                                                                   \
            {                                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                         \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];                       \
                                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                    reg1 = _mm512_rol_epi32(reg1, 0);                                                           \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                    reg2 = _mm512_rol_epi32(reg2, 16);                                                          \
                    sum = _mm512_add_epi8(sum, reg1);                                                           \
                    sum = _mm512_add_epi8(sum, reg2);                                                           \
                }                                                                                               \
                for(uint32_t i=0; i<1; i++){                                                                    \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                  \
                }                                                                                               \
            }                                                                                                   \
        }                                                                                                       \
        else{                                                                                                   \
            __m512i mask = _mm512_set_epi64(                                                                    \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL,                                              \
                                            0x0f0b07030e0a0602ULL,                                              \
                                            0x0d0905010c080400ULL);                                             \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                                       \
                                                1, 3, 5, 7, 9, 11, 13, 15);                                     \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);             \
                                                                                                                \
            for (int cl = 0; cl < iter; cl++)                                                                   \
            {                                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                         \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];                       \
                                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                    reg1 = _mm512_rol_epi32(reg1, 0);                                                           \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                              \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                                     \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                                \
                                                                                                                \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                    reg2 = _mm512_rol_epi32(reg2, 16);                                                          \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                              \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                                     \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                                \
                                                                                                                \
                    if(size==2){                                                                                \
                        sum = _mm512_add_epi16(sum, reg1);                                                      \
                        sum = _mm512_add_epi16(sum, reg2);                                                      \
                    }                                                                                           \
                    else if(size==4){                                                                           \
                        sum = _mm512_add_epi32(sum, reg1);                                                      \
                        sum = _mm512_add_epi32(sum, reg2);                                                      \
                    }                                                                                           \
                    else if(size==8){                                                                           \
                        sum = _mm512_add_epi64(sum, reg1);                                                      \
                        sum = _mm512_add_epi64(sum, reg2);                                                      \
                    }                                                                                           \
                }                                                                                               \
                                                                                                                \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                                    \
                sum = _mm512_shuffle_epi8(sum, mask);                                                           \
                sum = _mm512_permutexvar_epi32(perm, sum);                                                      \
                                                                                                                \
                for(uint32_t i=0; i<1; i++){                                                                    \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                         \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                  \
                }                                                                                               \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)

#define RNS_SUM_RS(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size)        \
    do                                                                                          \
    {                                                                                           \
        __m512i reg1;                                                                           \
        __m512i reg2;                                                                           \
        __m512i reg3;                                                                           \
        __m512i reg4;                                                                           \
        __m512i reg5;                                                                           \
        __m512i reg6;                                                                           \
        __m512i reg7;                                                                           \
        __m512i reg8;                                                                           \
                                                                                                \
        void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                     \
                                                                                                \
        __m512i sum = _mm512_set_epi64(                                                         \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL,                                  \
                                        0x0000000000000000ULL);                                 \
                                                                                                \
        if(size == 1){                                                                          \
            for (int cl = 0; cl < iter; cl++)                                                   \
            {                                                                                   \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                    \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                  \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                  \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                  \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                  \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                  \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                  \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                  \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                           \
                                                                                                \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));           \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                           \
                                                                                                \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));           \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                          \
                                                                                                \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));           \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                          \
                                                                                                \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));           \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                          \
                                                                                                \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));           \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                          \
                                                                                                \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));           \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                          \
                                                                                                \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));           \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                          \
                                                                                                \
                    sum = _mm512_add_epi8(sum, reg1);                                           \
                    sum = _mm512_add_epi8(sum, reg2);                                           \
                    sum = _mm512_add_epi8(sum, reg3);                                           \
                    sum = _mm512_add_epi8(sum, reg4);                                           \
                    sum = _mm512_add_epi8(sum, reg5);                                           \
                    sum = _mm512_add_epi8(sum, reg6);                                           \
                    sum = _mm512_add_epi8(sum, reg7);                                           \
                    sum = _mm512_add_epi8(sum, reg8);                                           \
                                                                                                \
                }                                                                               \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
            }                                                                                   \
        }                                                                                       \
        else{                                                                                   \
            __m512i mask = _mm512_set_epi64(                                                    \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL,                                  \
                                        0x0f0b07030e0a0602ULL,                                  \
                                        0x0d0905010c080400ULL);                                 \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                       \
                                                1, 3, 5, 7, 9, 11, 13, 15);                     \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8,                         \
                                                12, 9, 13, 10, 14, 11, 15);                     \
                                                                                                \
            for (int cl = 0; cl < iter; cl++){                                                  \
                for(uint32_t i=0; i<num_iter_src; i++){                                         \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                    \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                  \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                  \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                  \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                  \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                  \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                  \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                  \
                                                                                                \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));           \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                           \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                              \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                     \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                \
                                                                                                \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));           \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                           \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                              \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                     \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                \
                                                                                                \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));           \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                          \
                    reg3 = _mm512_permutexvar_epi32(vindex, reg3);                              \
                    reg3 = _mm512_shuffle_epi8(reg3, mask);                                     \
                    reg3 = _mm512_permutexvar_epi32(perm, reg3);                                \
                                                                                                \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));           \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                          \
                    reg4 = _mm512_permutexvar_epi32(vindex, reg4);                              \
                    reg4 = _mm512_shuffle_epi8(reg4, mask);                                     \
                    reg4 = _mm512_permutexvar_epi32(perm, reg4);                                \
                                                                                                \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));           \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                          \
                    reg5 = _mm512_permutexvar_epi32(vindex, reg5);                              \
                    reg5 = _mm512_shuffle_epi8(reg5, mask);                                     \
                    reg5 = _mm512_permutexvar_epi32(perm, reg5);                                \
                                                                                                \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));           \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                          \
                    reg6 = _mm512_permutexvar_epi32(vindex, reg6);                              \
                    reg6 = _mm512_shuffle_epi8(reg6, mask);                                     \
                    reg6 = _mm512_permutexvar_epi32(perm, reg6);                                \
                                                                                                \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));           \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                          \
                    reg7 = _mm512_permutexvar_epi32(vindex, reg7);                              \
                    reg7 = _mm512_shuffle_epi8(reg7, mask);                                     \
                    reg7 = _mm512_permutexvar_epi32(perm, reg7);                                \
                                                                                                \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));           \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                          \
                    reg8 = _mm512_permutexvar_epi32(vindex, reg8);                              \
                    reg8 = _mm512_shuffle_epi8(reg8, mask);                                     \
                    reg8 = _mm512_permutexvar_epi32(perm, reg8);                                \
                                                                                                \
                    if(size == 1){                                                              \
                        sum = _mm512_add_epi8(sum, reg1);                                       \
                        sum = _mm512_add_epi8(sum, reg2);                                       \
                        sum = _mm512_add_epi8(sum, reg3);                                       \
                        sum = _mm512_add_epi8(sum, reg4);                                       \
                        sum = _mm512_add_epi8(sum, reg5);                                       \
                        sum = _mm512_add_epi8(sum, reg6);                                       \
                        sum = _mm512_add_epi8(sum, reg7);                                       \
                        sum = _mm512_add_epi8(sum, reg8);                                       \
                    }                                                                           \
                    else if(size == 2){                                                         \
                        sum = _mm512_add_epi16(sum, reg1);                                      \
                        sum = _mm512_add_epi16(sum, reg2);                                      \
                        sum = _mm512_add_epi16(sum, reg3);                                      \
                        sum = _mm512_add_epi16(sum, reg4);                                      \
                        sum = _mm512_add_epi16(sum, reg5);                                      \
                        sum = _mm512_add_epi16(sum, reg6);                                      \
                        sum = _mm512_add_epi16(sum, reg7);                                      \
                        sum = _mm512_add_epi16(sum, reg8);                                      \
                    }                                                                           \
                    else if(size == 4){                                                         \
                        sum = _mm512_add_epi32(sum, reg1);                                      \
                        sum = _mm512_add_epi32(sum, reg2);                                      \
                        sum = _mm512_add_epi32(sum, reg3);                                      \
                        sum = _mm512_add_epi32(sum, reg4);                                      \
                        sum = _mm512_add_epi32(sum, reg5);                                      \
                        sum = _mm512_add_epi32(sum, reg6);                                      \
                        sum = _mm512_add_epi32(sum, reg7);                                      \
                        sum = _mm512_add_epi32(sum, reg8);                                      \
                    }                                                                           \
                    else{                                                                       \
                        sum = _mm512_add_epi64(sum, reg1);                                      \
                        sum = _mm512_add_epi64(sum, reg2);                                      \
                        sum = _mm512_add_epi64(sum, reg3);                                      \
                        sum = _mm512_add_epi64(sum, reg4);                                      \
                        sum = _mm512_add_epi64(sum, reg5);                                      \
                        sum = _mm512_add_epi64(sum, reg6);                                      \
                        sum = _mm512_add_epi64(sum, reg7);                                      \
                        sum = _mm512_add_epi64(sum, reg8);                                      \
                    }                                                                           \
                }                                                                               \
                                                                                                \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                    \
                sum = _mm512_shuffle_epi8(sum, mask);                                           \
                sum = _mm512_permutexvar_epi32(perm, sum);                                      \
                                                                                                \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
            }                                                                                   \
        }                                                                                       \
    } while (0)

#define REDUCE_SCATTER_CPU_X_24(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)     \
    do                                                                                                              \
    {                                                                                                               \
        __m512i reg1;                                                                                               \
        __m512i reg2;                                                                                               \
        __m512i reg3;                                                                                               \
        __m512i reg4;                                                                                               \
                                                                                                                    \
        __m512i sum = _mm512_set_epi64(                                                                             \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL);                                                     \
        __m512i mask_ar = _mm512_set_epi64(                                                                         \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL);                                                     \
        if(size == 1){                                                                                              \
            for (int cl = 0; cl < iter; cl++)                                                                       \
            {                                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                               \
                                                                                                                    \
                    if(a_length==4){                                                                                \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                           \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                           \
                                                                                                                    \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                                           \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                        reg2 = _mm512_rol_epi32(reg2, 8);                                                           \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                           \
                        reg3 = _mm512_rol_epi32(reg3, 16);                                                          \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                           \
                        reg4 = _mm512_rol_epi32(reg4, 24);                                                          \
                        sum = _mm512_add_epi8(sum, reg1);                                                           \
                        sum = _mm512_add_epi8(sum, reg2);                                                           \
                        sum = _mm512_add_epi8(sum, reg3);                                                           \
                        sum = _mm512_add_epi8(sum, reg4);                                                           \
                    }                                                                                               \
                    else if(a_length==2){                                                                           \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                                           \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                        reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                                  \
                        sum = _mm512_add_epi8(sum, reg1);                                                           \
                        sum = _mm512_add_epi8(sum, reg2);                                                           \
                    }                                                                                               \
                                                                                                                    \
                }                                                                                                   \
                void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                                 \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                          \
            }                                                                                                       \
        }                                                                                                           \
        else{                                                                                                       \
            __m512i mask = _mm512_set_epi64(                                                                        \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL);                                                 \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                                           \
                                                1, 3, 5, 7, 9, 11, 13, 15);                                         \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);                 \
                                                                                                                    \
            for (int cl = 0; cl < iter; cl++)                                                                       \
            {                                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                               \
                    if(a_length==4){                                                                                \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                           \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                           \
                                                                                                                    \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                                           \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                              \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                     \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                                \
                                                                                                                    \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                        reg2 = _mm512_rol_epi32(reg2, 8);                                                           \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                              \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                                     \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                                \
                                                                                                                    \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                           \
                        reg3 = _mm512_rol_epi32(reg3, 16);                                                          \
                        reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                              \
                        reg3 = _mm512_shuffle_epi8(reg3, mask);                                                     \
                        reg3 = _mm512_permutexvar_epi32(perm, reg3);                                                \
                                                                                                                    \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                           \
                        reg4 = _mm512_rol_epi32(reg4, 24);                                                          \
                        reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                              \
                        reg4 = _mm512_shuffle_epi8(reg4, mask);                                                     \
                        reg4 = _mm512_permutexvar_epi32(perm, reg4);                                                \
                    }                                                                                               \
                    else if(a_length==2){                                                                           \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                           \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                                           \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                              \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                                     \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                                \
                                                                                                                    \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                           \
                        reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                                  \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                              \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                                     \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                                \
                    }                                                                                               \
                                                                                                                    \
                    if(size == 2){                                                                                  \
                        sum = _mm512_add_epi16(sum, reg1);                                                          \
                        sum = _mm512_add_epi16(sum, reg2);                                                          \
                        if(a_length==4){                                                                            \
                            sum = _mm512_add_epi16(sum, reg3);                                                      \
                            sum = _mm512_add_epi16(sum, reg4);                                                      \
                        }                                                                                           \
                    }                                                                                               \
                    else if(size == 4){                                                                             \
                        sum = _mm512_add_epi32(sum, reg1);                                                          \
                        sum = _mm512_add_epi32(sum, reg2);                                                          \
                        if(a_length==4){                                                                            \
                            sum = _mm512_add_epi32(sum, reg3);                                                      \
                            sum = _mm512_add_epi32(sum, reg4);                                                      \
                        }                                                                                           \
                    }                                                                                               \
                    else{                                                                                           \
                        sum = _mm512_add_epi64(sum, reg1);                                                          \
                        sum = _mm512_add_epi64(sum, reg2);                                                          \
                        if(a_length==4){                                                                            \
                            sum = _mm512_add_epi64(sum, reg3);                                                      \
                            sum = _mm512_add_epi64(sum, reg4);                                                      \
                        }                                                                                           \
                    }                                                                                               \
                }                                                                                                   \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                                        \
                sum = _mm512_shuffle_epi8(sum, mask);                                                               \
                sum = _mm512_permutexvar_epi32(perm, sum);                                                          \
                                                                                                                    \
                                                                                                                    \
                void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                                 \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                          \
            }                                                                                                       \
        }                                                                                                           \
    } while (0)

#define REDUCE_SCATTER_CPU_X_22(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)     \
    do                                                                                                              \
    {                                                                                                               \
        __m512i reg1;                                                                                               \
        __m512i reg2;                                                                                               \
        __m512i reg3;                                                                                               \
        __m512i reg4;                                                                                               \
                                                                                                                    \
        __m512i sum = _mm512_set_epi64(                                                                             \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL,                                                      \
                                        0x0000000000000000ULL);                                                     \
        __m512i mask_ar = _mm512_set_epi64(                                                                         \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL,                                                      \
                                        0x0e0f0c0d0a0b0809ULL,                                                      \
                                        0x0607040502030001ULL);                                                     \
        if(size == 1){                                                                                              \
            for (int cl = 0; cl < iter; cl++)                                                                       \
            {                                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                               \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                               \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                               \
                                                                                                                    \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                               \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                               \
                    reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                                      \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                               \
                    reg3 = _mm512_rol_epi64(reg3, 32);                                                              \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                               \
                    reg4 = _mm512_rol_epi64(reg4, 32);                                                              \
                    reg4 = _mm512_shuffle_epi8(reg4, mask_ar);                                                      \
                    sum = _mm512_add_epi8(sum, reg1);                                                               \
                    sum = _mm512_add_epi8(sum, reg2);                                                               \
                    sum = _mm512_add_epi8(sum, reg3);                                                               \
                    sum = _mm512_add_epi8(sum, reg4);                                                               \
                }                                                                                                   \
                void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                                 \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                          \
            }                                                                                                       \
        }                                                                                                           \
        else{                                                                                                       \
            __m512i mask = _mm512_set_epi64(                                                                        \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL,                                                  \
                                            0x0f0b07030e0a0602ULL,                                                  \
                                            0x0d0905010c080400ULL);                                                 \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                                           \
                                                1, 3, 5, 7, 9, 11, 13, 15);                                         \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);                 \
                                                                                                                    \
            for (int cl = 0; cl < iter; cl++)                                                                       \
            {                                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                             \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                               \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                               \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                               \
                                                                                                                    \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                               \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                                  \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                                         \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                                    \
                                                                                                                    \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                               \
                    reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                                      \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                                  \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                                         \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                                    \
                                                                                                                    \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                               \
                    reg3 = _mm512_rol_epi64(reg3, 32);                                                              \
                    reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                                  \
                    reg3 = _mm512_shuffle_epi8(reg3, mask);                                                         \
                    reg3 = _mm512_permutexvar_epi32(perm, reg3);                                                    \
                                                                                                                    \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                               \
                    reg4 = _mm512_rol_epi64(reg4, 32);                                                              \
                    reg4 = _mm512_shuffle_epi8(reg4, mask_ar);                                                      \
                    reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                                  \
                    reg4 = _mm512_shuffle_epi8(reg4, mask);                                                         \
                    reg4 = _mm512_permutexvar_epi32(perm, reg4);                                                    \
                                                                                                                    \
                    if(size == 2){                                                                                  \
                        sum = _mm512_add_epi16(sum, reg1);                                                          \
                        sum = _mm512_add_epi16(sum, reg2);                                                          \
                        sum = _mm512_add_epi16(sum, reg3);                                                          \
                        sum = _mm512_add_epi16(sum, reg4);                                                          \
                    }                                                                                               \
                    else if(size == 4){                                                                             \
                        sum = _mm512_add_epi32(sum, reg1);                                                          \
                        sum = _mm512_add_epi32(sum, reg2);                                                          \
                        sum = _mm512_add_epi32(sum, reg3);                                                          \
                        sum = _mm512_add_epi32(sum, reg4);                                                          \
                    }                                                                                               \
                    else{                                                                                           \
                        sum = _mm512_add_epi64(sum, reg1);                                                          \
                        sum = _mm512_add_epi64(sum, reg2);                                                          \
                        sum = _mm512_add_epi64(sum, reg3);                                                          \
                        sum = _mm512_add_epi64(sum, reg4);                                                          \
                    }                                                                                               \
                }                                                                                                   \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                                        \
                sum = _mm512_shuffle_epi8(sum, mask);                                                               \
                sum = _mm512_permutexvar_epi32(perm, sum);                                                          \
                                                                                                                    \
                                                                                                                    \
                void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr;                                                 \
                _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                                          \
            }                                                                                                       \
        }                                                                                                           \
    } while (0)

#define S_SUM_AR(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, reduce_type)     \
    do                                                                                                  \
    {                                                                                                   \
        __m512i reg1;                                                                                   \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
                                                                                                        \
        if(size == 1){                                                                                  \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                              \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    if(reduce_type == 0) sum = _mm512_add_epi8(sum, reg1);                              \
                    else if (reduce_type == 1) sum = _mm512_max_epi8(sum, reg1);                        \
                }                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[i];                              \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
                                                                                                        \
            __m512i mask = _mm512_set_epi64(                                                            \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL);                                         \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8,                                 \
                                                12, 9, 13, 10, 14, 11, 15);                             \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[i];                              \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                        \
                    if(reduce_type == 0){                                                               \
                        if(size == 2) sum = _mm512_add_epi16(sum, reg1);                                \
                        else if(size == 4) sum = _mm512_add_epi32(sum, reg1);                           \
                        else sum = _mm512_add_epi64(sum, reg1);                                         \
                    }                                                                                   \
                    else if(reduce_type == 1){                                                          \
                        if(size == 2) sum = _mm512_max_epi16(sum, reg1);                                \
                        else if(size == 4) sum = _mm512_max_epi32(sum, reg1);                           \
                        else sum = _mm512_max_epi64(sum, reg1);                                         \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[i];                              \
                                                                                                        \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                                                                                                        \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define S_SUM_AR_24(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)     \
    do                                                                                                  \
    {                                                                                                   \
                                                                                                        \
        a_length+=0;                                                                                    \
        __m512i reg1;                                                                                   \
        __m512i reg2;                                                                                   \
        __m512i reg3;                                                                                   \
        __m512i reg4;                                                                                   \
                                                                                                        \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
                                                                                                        \
        if(size == 1){                                                                                  \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];               \
                    if(a_length==2){                                                                    \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[(8/a_length)*i+2];           \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[(8/a_length)*i+3];           \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi64(reg2, 16);                                              \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                        reg3 = _mm512_rol_epi64(reg3, 32);                                              \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                        reg4 = _mm512_rol_epi64(reg4, 48);                                              \
                        sum = _mm512_add_epi8(sum, reg1);                                               \
                        sum = _mm512_add_epi8(sum, reg2);                                               \
                        sum = _mm512_add_epi8(sum, reg3);                                               \
                        sum = _mm512_add_epi8(sum, reg4);                                               \
                    }                                                                                   \
                    else if(a_length==4){                                                               \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi64(reg2, 32);                                              \
                        sum = _mm512_add_epi8(sum, reg1);                                               \
                        sum = _mm512_add_epi8(sum, reg2);                                               \
                    }                                                                                   \
                }                                                                                       \
                if(a_length==2){sum = _mm512_rol_epi64(sum, 48);}                                       \
                else if(a_length==4){sum = _mm512_rol_epi64(sum, 32);}                                  \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[(8/a_length)*i+1];              \
                    if(a_length==2){                                                                    \
                        void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[(8/a_length)*i+2];          \
                        void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[(8/a_length)*i+3];          \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                     \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                     \
                    }                                                                                   \
                    else if(a_length==4){                                                               \
                        sum = _mm512_rol_epi64(sum, 32);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi64(sum, 32);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
            __m512i mask = _mm512_set_epi64(                                                            \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL);                                     \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);     \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];               \
                    if(a_length==2){                                                                    \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[(8/a_length)*i+2];           \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[(8/a_length)*i+3];           \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                  \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                         \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                    \
                                                                                                        \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi64(reg2, 16);                                              \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                  \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                         \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                    \
                                                                                                        \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                        reg3 = _mm512_rol_epi64(reg3, 32);                                              \
                        reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                  \
                        reg3 = _mm512_shuffle_epi8(reg3, mask);                                         \
                        reg3 = _mm512_permutexvar_epi32(perm, reg3);                                    \
                                                                                                        \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                        reg4 = _mm512_rol_epi64(reg4, 48);                                              \
                        reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                  \
                        reg4 = _mm512_shuffle_epi8(reg4, mask);                                         \
                        reg4 = _mm512_permutexvar_epi32(perm, reg4);                                    \
                                                                                                        \
                        if(size==2){                                                                    \
                            sum = _mm512_add_epi16(sum, reg1);                                          \
                            sum = _mm512_add_epi16(sum, reg2);                                          \
                            sum = _mm512_add_epi16(sum, reg3);                                          \
                            sum = _mm512_add_epi16(sum, reg4);                                          \
                        }                                                                               \
                        if(size==4){                                                                    \
                            sum = _mm512_add_epi32(sum, reg1);                                          \
                            sum = _mm512_add_epi32(sum, reg2);                                          \
                            sum = _mm512_add_epi32(sum, reg3);                                          \
                            sum = _mm512_add_epi32(sum, reg4);                                          \
                        }                                                                               \
                        else if(size==8){                                                               \
                            sum = _mm512_add_epi64(sum, reg1);                                          \
                            sum = _mm512_add_epi64(sum, reg2);                                          \
                            sum = _mm512_add_epi64(sum, reg3);                                          \
                            sum = _mm512_add_epi64(sum, reg4);                                          \
                        }                                                                               \
                    }                                                                                   \
                    else if(a_length==4){                                                               \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                  \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                         \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                    \
                                                                                                        \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi64(reg2, 32);                                              \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                  \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                         \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                    \
                                                                                                        \
                        if(size==2){                                                                    \
                            sum = _mm512_add_epi16(sum, reg1);                                          \
                            sum = _mm512_add_epi16(sum, reg2);                                          \
                        }                                                                               \
                        else if(size==4){                                                               \
                            sum = _mm512_add_epi32(sum, reg1);                                          \
                            sum = _mm512_add_epi32(sum, reg2);                                          \
                        }                                                                               \
                        else if(size==8){                                                               \
                            sum = _mm512_add_epi64(sum, reg1);                                          \
                            sum = _mm512_add_epi64(sum, reg2);                                          \
                        }                                                                               \
                    }                                                                                   \
                }                                                                                       \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                                                                                                        \
                if(a_length==2){sum = _mm512_rol_epi64(sum, 48);}                                       \
                else if(a_length==4){sum = _mm512_rol_epi64(sum, 32);}                                  \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[(8/a_length)*i+1];              \
                    if(a_length==2){                                                                    \
                        void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[(8/a_length)*i+2];          \
                        void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[(8/a_length)*i+3];          \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                     \
                        sum = _mm512_rol_epi64(sum, 16);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                     \
                    }                                                                                   \
                    else if(a_length==4){                                                               \
                        sum = _mm512_rol_epi64(sum, 32);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi64(sum, 32);                                                \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define S_SUM_AR_22(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)     \
    do                                                                                                  \
    {                                                                                                   \
        __m512i reg1;                                                                                   \
        __m512i reg2;                                                                                   \
                                                                                                        \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
                                                                                                        \
        if(size == 1){                                                                                  \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];               \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_rol_epi32(reg2, 16);                                                  \
                    sum = _mm512_add_epi8(sum, reg1);                                                   \
                    sum = _mm512_add_epi8(sum, reg2);                                                   \
                }                                                                                       \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[(8/a_length)*i+1];              \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                    sum = _mm512_rol_epi32(sum, 16);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
            __m512i mask = _mm512_set_epi64(                                                            \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL);                                     \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);     \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[(8/a_length)*i+1];               \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                        \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_rol_epi32(reg2, 16);                                                  \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                      \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                             \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                        \
                                                                                                        \
                    if(size==2){                                                                        \
                        sum = _mm512_add_epi16(sum, reg1);                                              \
                        sum = _mm512_add_epi16(sum, reg2);                                              \
                    }                                                                                   \
                    else if(size==4){                                                                   \
                        sum = _mm512_add_epi32(sum, reg1);                                              \
                        sum = _mm512_add_epi32(sum, reg2);                                              \
                    }                                                                                   \
                    else if(size==8){                                                                   \
                        sum = _mm512_add_epi64(sum, reg1);                                              \
                        sum = _mm512_add_epi64(sum, reg2);                                              \
                    }                                                                                   \
                }                                                                                       \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                                                                                                        \
                sum = _mm512_rol_epi32(sum, 16);                                                        \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[(8/a_length)*i];                 \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[(8/a_length)*i+1];              \
                    sum = _mm512_rol_epi32(sum, 16);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                    sum = _mm512_rol_epi32(sum, 16);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define RNS_SUM_AR(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, reduce_type)   \
    do                                                                                                  \
    {                                                                                                   \
        __m512i reg1;                                                                                   \
        __m512i reg2;                                                                                   \
        __m512i reg3;                                                                                   \
        __m512i reg4;                                                                                   \
        __m512i reg5;                                                                                   \
        __m512i reg6;                                                                                   \
        __m512i reg7;                                                                                   \
        __m512i reg8;                                                                                   \
                                                                                                        \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
        if(size == 0){                                                                                  \
            for (int cl = 0; cl < iter; cl++){                                                          \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                            \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                          \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                          \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                          \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                          \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                          \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                          \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                          \
                                                                                                        \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                                   \
                                                                                                        \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                                   \
                                                                                                        \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                   \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                                  \
                                                                                                        \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                   \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                                  \
                                                                                                        \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));                   \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                                  \
                                                                                                        \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));                   \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                                  \
                                                                                                        \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));                   \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                                  \
                                                                                                        \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));                   \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                                  \
                                                                                                        \
                                                                                                        \
                    sum = _mm512_or_epi64(sum, reg1);                                                   \
                    sum = _mm512_or_epi64(sum, reg2);                                                   \
                    sum = _mm512_or_epi64(sum, reg3);                                                   \
                    sum = _mm512_or_epi64(sum, reg4);                                                   \
                    sum = _mm512_or_epi64(sum, reg5);                                                   \
                    sum = _mm512_or_epi64(sum, reg6);                                                   \
                    sum = _mm512_or_epi64(sum, reg7);                                                   \
                    sum = _mm512_or_epi64(sum, reg8);                                                   \
                                                                                                        \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_rol_epi64(sum, 8);                                                         \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[8*i];                            \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[8*i+1];                         \
                    void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[8*i+2];                         \
                    void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[8*i+3];                         \
                    void *dst_rank_clwise_addr13 = dst_rank_bgwise_addr[8*i+4];                         \
                    void *dst_rank_clwise_addr14 = dst_rank_bgwise_addr[8*i+5];                         \
                    void *dst_rank_clwise_addr15 = dst_rank_bgwise_addr[8*i+6];                         \
                    void *dst_rank_clwise_addr16 = dst_rank_bgwise_addr[8*i+7];                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr13), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr14), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr15), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr16), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else if(size == 1){                                                                             \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                            \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                          \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                          \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                          \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                          \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                          \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                          \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                          \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                                   \
                                                                                                        \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                                   \
                                                                                                        \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                   \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                                  \
                                                                                                        \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                   \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                                  \
                                                                                                        \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));                   \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                                  \
                                                                                                        \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));                   \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                                  \
                                                                                                        \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));                   \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                                  \
                                                                                                        \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));                   \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                                  \
                                                                                                        \
                    if(reduce_type == 0){                                                               \
                        sum = _mm512_add_epi8(sum, reg1);                                               \
                        sum = _mm512_add_epi8(sum, reg2);                                               \
                        sum = _mm512_add_epi8(sum, reg3);                                               \
                        sum = _mm512_add_epi8(sum, reg4);                                               \
                        sum = _mm512_add_epi8(sum, reg5);                                               \
                        sum = _mm512_add_epi8(sum, reg6);                                               \
                        sum = _mm512_add_epi8(sum, reg7);                                               \
                        sum = _mm512_add_epi8(sum, reg8);                                               \
                    }                                                                                   \
                    else if(reduce_type == 1){                                                          \
                        sum = _mm512_max_epi8(sum, reg1);                                               \
                        sum = _mm512_max_epi8(sum, reg2);                                               \
                        sum = _mm512_max_epi8(sum, reg3);                                               \
                        sum = _mm512_max_epi8(sum, reg4);                                               \
                        sum = _mm512_max_epi8(sum, reg5);                                               \
                        sum = _mm512_max_epi8(sum, reg6);                                               \
                        sum = _mm512_max_epi8(sum, reg7);                                               \
                        sum = _mm512_max_epi8(sum, reg8);                                               \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_rol_epi64(sum, 8);                                                         \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[8*i];                            \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[8*i+1];                         \
                    void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[8*i+2];                         \
                    void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[8*i+3];                         \
                    void *dst_rank_clwise_addr13 = dst_rank_bgwise_addr[8*i+4];                         \
                    void *dst_rank_clwise_addr14 = dst_rank_bgwise_addr[8*i+5];                         \
                    void *dst_rank_clwise_addr15 = dst_rank_bgwise_addr[8*i+6];                         \
                    void *dst_rank_clwise_addr16 = dst_rank_bgwise_addr[8*i+7];                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr13), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr14), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr15), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr16), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
                                                                                                        \
            __m512i mask = _mm512_set_epi64(                                                            \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL,                                          \
                                        0x0f0b07030e0a0602ULL,                                          \
                                        0x0d0905010c080400ULL);                                         \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8,                                 \
                                                12, 9, 13, 10, 14, 11, 15);                             \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[8*i];                            \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[8*i+1];                          \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[8*i+2];                          \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[8*i+3];                          \
                    void *src_rank_clwise_addr5 = src_rank_bgwise_addr[8*i+4];                          \
                    void *src_rank_clwise_addr6 = src_rank_bgwise_addr[8*i+5];                          \
                    void *src_rank_clwise_addr7 = src_rank_bgwise_addr[8*i+6];                          \
                    void *src_rank_clwise_addr8 = src_rank_bgwise_addr[8*i+7];                          \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_rol_epi64(reg1, 0);                                                   \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                        \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_rol_epi64(reg2, 8);                                                   \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                      \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                             \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                        \
                                                                                                        \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                   \
                    reg3 = _mm512_rol_epi64(reg3, 16);                                                  \
                    reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                      \
                    reg3 = _mm512_shuffle_epi8(reg3, mask);                                             \
                    reg3 = _mm512_permutexvar_epi32(perm, reg3);                                        \
                                                                                                        \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                   \
                    reg4 = _mm512_rol_epi64(reg4, 24);                                                  \
                    reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                      \
                    reg4 = _mm512_shuffle_epi8(reg4, mask);                                             \
                    reg4 = _mm512_permutexvar_epi32(perm, reg4);                                        \
                                                                                                        \
                    reg5 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr5));                   \
                    reg5 = _mm512_rol_epi64(reg5, 32);                                                  \
                    reg5 = _mm512_permutexvar_epi32(vindex, reg5);                                      \
                    reg5 = _mm512_shuffle_epi8(reg5, mask);                                             \
                    reg5 = _mm512_permutexvar_epi32(perm, reg5);                                        \
                                                                                                        \
                    reg6 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr6));                   \
                    reg6 = _mm512_rol_epi64(reg6, 40);                                                  \
                    reg6 = _mm512_permutexvar_epi32(vindex, reg6);                                      \
                    reg6 = _mm512_shuffle_epi8(reg6, mask);                                             \
                    reg6 = _mm512_permutexvar_epi32(perm, reg6);                                        \
                                                                                                        \
                    reg7 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr7));                   \
                    reg7 = _mm512_rol_epi64(reg7, 48);                                                  \
                    reg7 = _mm512_permutexvar_epi32(vindex, reg7);                                      \
                    reg7 = _mm512_shuffle_epi8(reg7, mask);                                             \
                    reg7 = _mm512_permutexvar_epi32(perm, reg7);                                        \
                                                                                                        \
                    reg8 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr8));                   \
                    reg8 = _mm512_rol_epi64(reg8, 56);                                                  \
                    reg8 = _mm512_permutexvar_epi32(vindex, reg8);                                      \
                    reg8 = _mm512_shuffle_epi8(reg8, mask);                                             \
                    reg8 = _mm512_permutexvar_epi32(perm, reg8);                                        \
                                                                                                        \
                    if(reduce_type == 0){                                                               \
                        if(size == 2){                                                                  \
                            sum = _mm512_add_epi16(sum, reg1);                                          \
                            sum = _mm512_add_epi16(sum, reg2);                                          \
                            sum = _mm512_add_epi16(sum, reg3);                                          \
                            sum = _mm512_add_epi16(sum, reg4);                                          \
                            sum = _mm512_add_epi16(sum, reg5);                                          \
                            sum = _mm512_add_epi16(sum, reg6);                                          \
                            sum = _mm512_add_epi16(sum, reg7);                                          \
                            sum = _mm512_add_epi16(sum, reg8);                                          \
                        }                                                                               \
                        else if(size == 4){                                                             \
                            sum = _mm512_add_epi32(sum, reg1);                                          \
                            sum = _mm512_add_epi32(sum, reg2);                                          \
                            sum = _mm512_add_epi32(sum, reg3);                                          \
                            sum = _mm512_add_epi32(sum, reg4);                                          \
                            sum = _mm512_add_epi32(sum, reg5);                                          \
                            sum = _mm512_add_epi32(sum, reg6);                                          \
                            sum = _mm512_add_epi32(sum, reg7);                                          \
                            sum = _mm512_add_epi32(sum, reg8);                                          \
                        }                                                                               \
                        else{                                                                           \
                            sum = _mm512_add_epi64(sum, reg1);                                          \
                            sum = _mm512_add_epi64(sum, reg2);                                          \
                            sum = _mm512_add_epi64(sum, reg3);                                          \
                            sum = _mm512_add_epi64(sum, reg4);                                          \
                            sum = _mm512_add_epi64(sum, reg5);                                          \
                            sum = _mm512_add_epi64(sum, reg6);                                          \
                            sum = _mm512_add_epi64(sum, reg7);                                          \
                            sum = _mm512_add_epi64(sum, reg8);                                          \
                        }                                                                               \
                    }                                                                                   \
                    else if(reduce_type == 1){                                                          \
                        if(size == 2){                                                                  \
                            sum = _mm512_max_epi16(sum, reg1);                                          \
                            sum = _mm512_max_epi16(sum, reg2);                                          \
                            sum = _mm512_max_epi16(sum, reg3);                                          \
                            sum = _mm512_max_epi16(sum, reg4);                                          \
                            sum = _mm512_max_epi16(sum, reg5);                                          \
                            sum = _mm512_max_epi16(sum, reg6);                                          \
                            sum = _mm512_max_epi16(sum, reg7);                                          \
                            sum = _mm512_max_epi16(sum, reg8);                                          \
                        }                                                                               \
                        else if(size == 4){                                                             \
                            sum = _mm512_max_epi32(sum, reg1);                                          \
                            sum = _mm512_max_epi32(sum, reg2);                                          \
                            sum = _mm512_max_epi32(sum, reg3);                                          \
                            sum = _mm512_max_epi32(sum, reg4);                                          \
                            sum = _mm512_max_epi32(sum, reg5);                                          \
                            sum = _mm512_max_epi32(sum, reg6);                                          \
                            sum = _mm512_max_epi32(sum, reg7);                                          \
                            sum = _mm512_max_epi32(sum, reg8);                                          \
                        }                                                                               \
                        else{                                                                           \
                            sum = _mm512_max_epi64(sum, reg1);                                          \
                            sum = _mm512_max_epi64(sum, reg2);                                          \
                            sum = _mm512_max_epi64(sum, reg3);                                          \
                            sum = _mm512_max_epi64(sum, reg4);                                          \
                            sum = _mm512_max_epi64(sum, reg5);                                          \
                            sum = _mm512_max_epi64(sum, reg6);                                          \
                            sum = _mm512_max_epi64(sum, reg7);                                          \
                            sum = _mm512_max_epi64(sum, reg8);                                          \
                        }                                                                               \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                sum = _mm512_rol_epi64(sum, 8);                                                         \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[8*i];                            \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[8*i+1];                         \
                    void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[8*i+2];                         \
                    void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[8*i+3];                         \
                    void *dst_rank_clwise_addr13 = dst_rank_bgwise_addr[8*i+4];                         \
                    void *dst_rank_clwise_addr14 = dst_rank_bgwise_addr[8*i+5];                         \
                    void *dst_rank_clwise_addr15 = dst_rank_bgwise_addr[8*i+6];                         \
                    void *dst_rank_clwise_addr16 = dst_rank_bgwise_addr[8*i+7];                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr13), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr14), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr15), sum);                         \
                                                                                                        \
                    sum = _mm512_rol_epi64(sum, 56);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr16), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define RNS_SUM_AR_24(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)   \
    do                                                                                                  \
    {                                                                                                   \
        __m512i reg1;                                                                                   \
        __m512i reg2;                                                                                   \
        __m512i reg3;                                                                                   \
        __m512i reg4;                                                                                   \
                                                                                                        \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
        __m512i mask_ar = _mm512_set_epi64(                                                             \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL);                         \
                                                                                                        \
        if(size == 1){                                                                                  \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                     \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                   \
                                                                                                        \
                    if(a_length==4){                                                                    \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];               \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];               \
                                                                                                        \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                               \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi32(reg2, 8);                                               \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                        reg3 = _mm512_rol_epi32(reg3, 16);                                              \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                        reg4 = _mm512_rol_epi32(reg4, 24);                                              \
                        sum = _mm512_add_epi8(sum, reg1);                                               \
                        sum = _mm512_add_epi8(sum, reg2);                                               \
                        sum = _mm512_add_epi8(sum, reg3);                                               \
                        sum = _mm512_add_epi8(sum, reg4);                                               \
                    }                                                                                   \
                    else if(a_length==2){                                                               \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi64(reg1, 0);                                               \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                      \
                        sum = _mm512_add_epi8(sum, reg1);                                               \
                        sum = _mm512_add_epi8(sum, reg2);                                               \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                if(a_length==4){sum = _mm512_rol_epi32(sum, 24);}                                       \
                else if(a_length==2){sum = _mm512_shuffle_epi8(sum, mask_ar);}                          \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[a_length*i];                     \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[a_length*i+1];                  \
                    if(a_length==4){                                                                    \
                        void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[a_length*i+2];              \
                        void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[a_length*i+3];              \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                     \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                     \
                    }                                                                                   \
                    else if(a_length==2){                                                               \
                        sum = _mm512_shuffle_epi8(sum, mask_ar);                                        \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_shuffle_epi8(sum, mask_ar);                                        \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
                                                                                                        \
            __m512i mask = _mm512_set_epi64(                                                            \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL);                                     \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);     \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                     \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                   \
                    if(a_length==4){                                                                    \
                        void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];               \
                        void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];               \
                                                                                                        \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                               \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                  \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                         \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                    \
                                                                                                        \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_rol_epi32(reg2, 8);                                               \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                  \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                         \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                    \
                                                                                                        \
                        reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));               \
                        reg3 = _mm512_rol_epi32(reg3, 16);                                              \
                        reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                  \
                        reg3 = _mm512_shuffle_epi8(reg3, mask);                                         \
                        reg3 = _mm512_permutexvar_epi32(perm, reg3);                                    \
                                                                                                        \
                        reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));               \
                        reg4 = _mm512_rol_epi32(reg4, 24);                                              \
                        reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                  \
                        reg4 = _mm512_shuffle_epi8(reg4, mask);                                         \
                        reg4 = _mm512_permutexvar_epi32(perm, reg4);                                    \
                    }                                                                                   \
                    else if(a_length==2){                                                               \
                        reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));               \
                        reg1 = _mm512_rol_epi32(reg1, 0);                                               \
                        reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                  \
                        reg1 = _mm512_shuffle_epi8(reg1, mask);                                         \
                        reg1 = _mm512_permutexvar_epi32(perm, reg1);                                    \
                                                                                                        \
                        reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));               \
                        reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                      \
                        reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                  \
                        reg2 = _mm512_shuffle_epi8(reg2, mask);                                         \
                        reg2 = _mm512_permutexvar_epi32(perm, reg2);                                    \
                    }                                                                                   \
                                                                                                        \
                    if(size == 2){                                                                      \
                        sum = _mm512_add_epi16(sum, reg1);                                              \
                        sum = _mm512_add_epi16(sum, reg2);                                              \
                        if(a_length==4){                                                                \
                            sum = _mm512_add_epi16(sum, reg3);                                          \
                            sum = _mm512_add_epi16(sum, reg4);                                          \
                        }                                                                               \
                    }                                                                                   \
                    else if(size == 4){                                                                 \
                        sum = _mm512_add_epi32(sum, reg1);                                              \
                        sum = _mm512_add_epi32(sum, reg2);                                              \
                        if(a_length==4){                                                                \
                            sum = _mm512_add_epi32(sum, reg3);                                          \
                            sum = _mm512_add_epi32(sum, reg4);                                          \
                        }                                                                               \
                    }                                                                                   \
                    else{                                                                               \
                        sum = _mm512_add_epi64(sum, reg1);                                              \
                        sum = _mm512_add_epi64(sum, reg2);                                              \
                        if(a_length==4){                                                                \
                            sum = _mm512_add_epi64(sum, reg3);                                          \
                            sum = _mm512_add_epi64(sum, reg4);                                          \
                        }                                                                               \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                                                                                                        \
                if(a_length==4){sum = _mm512_rol_epi32(sum, 24);}                                       \
                else if(a_length==2){sum = _mm512_shuffle_epi8(sum, mask_ar);}                          \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[a_length*i];                     \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[a_length*i+1];                  \
                    if(a_length==4){                                                                    \
                        void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[a_length*i+2];              \
                        void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[a_length*i+3];              \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                     \
                        sum = _mm512_rol_epi32(sum, 8);                                                 \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                     \
                    }                                                                                   \
                    else if(a_length==2){                                                               \
                        sum = _mm512_shuffle_epi8(sum, mask_ar);                                        \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                      \
                        sum = _mm512_shuffle_epi8(sum, mask_ar);                                        \
                        _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                     \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#define RNS_SUM_AR_22(iter, src_rank_bgwise_addr, dst_rank_bgwise_addr, num_iter_src, size, a_length)   \
    do                                                                                                  \
    {                                                                                                   \
        __m512i reg1;                                                                                   \
        __m512i reg2;                                                                                   \
        __m512i reg3;                                                                                   \
        __m512i reg4;                                                                                   \
                                                                                                        \
        __m512i sum = _mm512_set_epi64(                                                                 \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL,                                          \
                                        0x0000000000000000ULL);                                         \
        __m512i mask_ar = _mm512_set_epi64(                                                             \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL,                          \
                                                        0x0e0f0c0d0a0b0809ULL,                          \
                                                        0x0607040502030001ULL);                         \
                                                                                                        \
        if(size == 1){                                                                                  \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                     \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                   \
                                                                                                        \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                   \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                   \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                          \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                   \
                    reg3 = _mm512_rol_epi64(reg3, 32);                                                  \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                   \
                    reg4 = _mm512_rol_epi64(reg4, 32);                                                  \
                    reg4 = _mm512_shuffle_epi8(reg4, mask_ar);                                          \
                    sum = _mm512_add_epi8(sum, reg1);                                                   \
                    sum = _mm512_add_epi8(sum, reg2);                                                   \
                    sum = _mm512_add_epi8(sum, reg3);                                                   \
                    sum = _mm512_add_epi8(sum, reg4);                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_rol_epi64(sum, 32);                                                        \
                sum = _mm512_shuffle_epi8(sum, mask_ar);                                                \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[a_length*i];                     \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[a_length*i+1];                  \
                    void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[a_length*i+2];                  \
                    void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[a_length*i+3];                  \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    sum = _mm512_rol_epi64(sum, 32);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    sum = _mm512_rol_epi64(sum, 32);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                         \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        else{                                                                                           \
                                                                                                        \
            __m512i mask = _mm512_set_epi64(                                                            \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL,                                      \
                                            0x0f0b07030e0a0602ULL,                                      \
                                            0x0d0905010c080400ULL);                                     \
            __m512i vindex = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14,                               \
                                                1, 3, 5, 7, 9, 11, 13, 15);                             \
            __m512i perm = _mm512_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);     \
                                                                                                        \
            for (int cl = 0; cl < iter; cl++)                                                           \
            {                                                                                           \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                    void *src_rank_clwise_addr1 = src_rank_bgwise_addr[a_length*i];                     \
                    void *src_rank_clwise_addr2 = src_rank_bgwise_addr[a_length*i+1];                   \
                    void *src_rank_clwise_addr3 = src_rank_bgwise_addr[a_length*i+2];                   \
                    void *src_rank_clwise_addr4 = src_rank_bgwise_addr[a_length*i+3];                   \
                                                                                                        \
                    reg1 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr1));                   \
                    reg1 = _mm512_permutexvar_epi32(vindex, reg1);                                      \
                    reg1 = _mm512_shuffle_epi8(reg1, mask);                                             \
                    reg1 = _mm512_permutexvar_epi32(perm, reg1);                                        \
                                                                                                        \
                    reg2 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr2));                   \
                    reg2 = _mm512_shuffle_epi8(reg2, mask_ar);                                          \
                    reg2 = _mm512_permutexvar_epi32(vindex, reg2);                                      \
                    reg2 = _mm512_shuffle_epi8(reg2, mask);                                             \
                    reg2 = _mm512_permutexvar_epi32(perm, reg2);                                        \
                                                                                                        \
                    reg3 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr3));                   \
                    reg3 = _mm512_rol_epi64(reg3, 32);                                                  \
                    reg3 = _mm512_permutexvar_epi32(vindex, reg3);                                      \
                    reg3 = _mm512_shuffle_epi8(reg3, mask);                                             \
                    reg3 = _mm512_permutexvar_epi32(perm, reg3);                                        \
                                                                                                        \
                    reg4 = _mm512_stream_load_si512((void *)(src_rank_clwise_addr4));                   \
                    reg4 = _mm512_rol_epi64(reg4, 32);                                                  \
                    reg4 = _mm512_shuffle_epi8(reg4, mask_ar);                                          \
                    reg4 = _mm512_permutexvar_epi32(vindex, reg4);                                      \
                    reg4 = _mm512_shuffle_epi8(reg4, mask);                                             \
                    reg4 = _mm512_permutexvar_epi32(perm, reg4);                                        \
                                                                                                        \
                    if(size == 2){                                                                      \
                        sum = _mm512_add_epi16(sum, reg1);                                              \
                        sum = _mm512_add_epi16(sum, reg2);                                              \
                        sum = _mm512_add_epi16(sum, reg3);                                              \
                        sum = _mm512_add_epi16(sum, reg4);                                              \
                    }                                                                                   \
                    else if(size == 4){                                                                 \
                        sum = _mm512_add_epi32(sum, reg1);                                              \
                        sum = _mm512_add_epi32(sum, reg2);                                              \
                        sum = _mm512_add_epi32(sum, reg3);                                              \
                        sum = _mm512_add_epi32(sum, reg4);                                              \
                    }                                                                                   \
                    else{                                                                               \
                        sum = _mm512_add_epi64(sum, reg1);                                              \
                        sum = _mm512_add_epi64(sum, reg2);                                              \
                        sum = _mm512_add_epi64(sum, reg3);                                              \
                        sum = _mm512_add_epi64(sum, reg4);                                              \
                    }                                                                                   \
                }                                                                                       \
                                                                                                        \
                sum = _mm512_permutexvar_epi32(vindex, sum);                                            \
                sum = _mm512_shuffle_epi8(sum, mask);                                                   \
                sum = _mm512_permutexvar_epi32(perm, sum);                                              \
                                                                                                        \
                sum = _mm512_rol_epi64(sum, 32);                                                        \
                sum = _mm512_shuffle_epi8(sum, mask_ar);                                                \
                                                                                                        \
                for(uint32_t i=0; i<num_iter_src; i++){                                                 \
                                                                                                        \
                    void *dst_rank_clwise_addr9 = dst_rank_bgwise_addr[a_length*i];                     \
                    void *dst_rank_clwise_addr10 = dst_rank_bgwise_addr[a_length*i+1];                  \
                    void *dst_rank_clwise_addr11 = dst_rank_bgwise_addr[a_length*i+2];                  \
                    void *dst_rank_clwise_addr12 = dst_rank_bgwise_addr[a_length*i+3];                  \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    sum = _mm512_rol_epi64(sum, 32);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr9), sum);                          \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr10), sum);                         \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    sum = _mm512_rol_epi64(sum, 32);                                                    \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr11), sum);                         \
                    sum = _mm512_shuffle_epi8(sum, mask_ar);                                            \
                    _mm512_stream_si512((void *)(dst_rank_clwise_addr12), sum);                         \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    } while (0)

void xeon_sp_trans_all_reduce_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t iter_dst_a, uint32_t src_start_offset, uint32_t dst_start_offset,\
                                 uint32_t byte_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type, uint32_t num_thread, uint32_t thread_id){
    void *src_rank_base_addr;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;
    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    src_mram_offset = size*reduce_type;
    if(alltoall_comm_type==0){
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length*8*iter_dst_a;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }

    uint32_t iter_length = byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    uint32_t src_rotate_group_offset_256_64;

    void *src_rank_addr_iter[num_iter_src * 8];
    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;
    void *src_rank_addr;

    void *dst_rank_addr;
    void *dst_rank_addr_iter[num_iter_src * 8];

    dst_rank_base_addr = base_region_addr_dst;

    //threaded
    src_mram_offset = src_start_offset + 1024*1024 + 8*(iter_length/num_thread)*thread_id;
    dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length*8*iter_dst_a + 8*(iter_length/num_thread)*thread_id;

    //for(uint32_t i=0; i<iter_length; i++){
    for(uint32_t i=(iter_length/num_thread)*thread_id; i<(iter_length/num_thread)*(thread_id+1); i++){

        for(uint32_t j=0; j<num_iter_src; j++){
            dst_rank_base_addr = base_region_addr_src[j];
            dst_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<8; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                dst_rank_addr_iter[j*8 + k] = dst_rank_addr + dst_rotate_group_offset_256_64;
            }
        }

        for(uint32_t j=0; j<num_iter_src; j++){
            src_rank_base_addr = base_region_addr_src[j];
            src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<8; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                src_rank_addr_iter[j*8 + k] = src_rank_addr + src_rotate_group_offset_256_64;
            }
        }
            
        RNS_SUM_AR(iteration, src_rank_addr_iter, dst_rank_addr_iter, num_iter_src, size, reduce_type);

        _mm_mfence();

        for(uint32_t j=0; j<num_iter_src*8; j++){
            RNS_FLUSH_SRC(iteration, src_rank_addr_iter[j]);
        }
        for(uint32_t j=0; j<num_iter_src*8; j++){
            RNS_FLUSH_DST(iteration, dst_rank_addr_iter[j]);
        }

        src_mram_offset+=8;
        dst_mram_offset+=8;
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_reduce_y_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t iter_dst_b, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type){

    void *src_rank_base_addr;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;

    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    if(alltoall_comm_type==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length*1*iter_dst_b;
    }

    uint32_t iter_length = byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    uint32_t src_rotate_group_offset_256_64;

    void *src_rank_addr_iter[num_iter_src * 8];
    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;
    void *src_rank_addr;

    void *dst_rank_addr;
    void *dst_rank_addr_iter[num_iter_src * 8];

    dst_rank_base_addr = base_region_addr_dst;

    for(uint32_t i=0; i<iter_length; i++){

        for(uint32_t j=0; j<num_iter_src; j++){
            dst_rank_base_addr = base_region_addr_src[j];
            dst_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<1; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                dst_rank_addr_iter[j + k] = dst_rank_addr + dst_rotate_group_offset_256_64;
            }
        }

        for(uint32_t j=0; j<num_iter_src; j++){
            src_rank_base_addr = base_region_addr_src[j];
            src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<1; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                src_rank_addr_iter[j + k] = src_rank_addr + src_rotate_group_offset_256_64;
            }
        }

        S_SUM_AR(iteration, src_rank_addr_iter, dst_rank_addr_iter, num_iter_src, size, reduce_type);

        _mm_mfence();

        for(uint32_t j=0; j<num_iter_src; j++){
            RNS_FLUSH_SRC(iteration, src_rank_addr_iter[j]);
        }
        for(uint32_t j=0; j<num_iter_src; j++){
            RNS_FLUSH_DST(iteration, dst_rank_addr_iter[j]);
        }

        src_mram_offset+=8;
        dst_mram_offset+=8;
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_reduce_rg_24(void *base_region_addr_dst, void **base_region_addr_src, uint32_t dst_rg_id, uint32_t* src_rg_id, uint32_t iter_dst_a, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t a_length, uint32_t size){

    void *dst_rank_base_addr = base_region_addr_dst;
    void *src_rank_base_addr_arr[num_iter_dst];
    uint32_t src_mram_offset;
    uint32_t src_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_src_offset_1mb_wise[8];
    for(uint32_t i=0; i<num_iter_dst; i++){
        src_rank_base_addr_arr[i]=base_region_addr_src[i];
        src_rotate_group_offset_256_64[i] = (src_rg_id[i]%4) * (256*1024) + (src_rg_id[i]/4) * 64;
    }
    src_mram_offset = src_start_offset + 1024*1024;
    
    int packet_size = 8;
    int iteration = packet_size / 8;
    int64_t mram_dst_offset_1mb_wise[8];

    uint32_t dst_mram_offset=0;
    if(!comm_type) dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length * a_length * iter_dst_a;
    else dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length * (8/a_length) * iter_dst_a;
    
    uint32_t remain_length;
    remain_length=byte_length/8;

    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    
    if(comm_type==0){ // parallel to entangled group
        {
            for(uint32_t i=0; i<remain_length; i++){
                void *dst_rank_addr_array[num_iter_dst * a_length];
                dst_rank_addr_array[0] = dst_rank_base_addr + dst_rotate_group_offset_256_64;
                void *src_rank_addr_array[num_iter_dst * a_length];
                uint32_t mram_64_bit_word_offset;
                uint64_t next_data;

                for(uint32_t j=0; j<num_iter_dst; j++){
                    for(uint32_t k=0; k<a_length; k++){
                        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+byte_length*k);
                        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                        mram_src_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                        void *src_rank_addr = src_rank_base_addr_arr[j] + mram_src_offset_1mb_wise[k];
                        src_rank_addr_array[j*a_length + k]=(src_rank_addr+src_rotate_group_offset_256_64[j]);
                    }
                }

                for(uint32_t j=0; j<num_iter_dst; j++){
                    for(uint32_t k=0; k<a_length; k++){
                        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+byte_length*k);
                        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                        mram_dst_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                        void *dst_rank_addr = src_rank_base_addr_arr[j] + mram_dst_offset_1mb_wise[k];
                        dst_rank_addr_array[j*a_length + k]=(dst_rank_addr+src_rotate_group_offset_256_64[j]);
                    }
                }
                
                RNS_SUM_AR_24(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, a_length);
                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    else{ //perpendicular to entangled group
        
        for(uint32_t i=0; i<remain_length; i++){
            void *dst_rank_addr_array[(8/a_length)*num_iter_dst];
            void *src_rank_addr_array[(8/a_length)*num_iter_dst];
            uint32_t rg_offset = 0;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            for(rg_offset=0; rg_offset<(8/a_length); rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<(8/a_length) * num_iter_dst; rg_offset++){    
                void *src_rank_addr = src_rank_base_addr_arr[rg_offset/(8/a_length)] + mram_src_offset_1mb_wise[rg_offset%(8/a_length)];
                src_rank_addr_array[rg_offset]=(src_rank_addr + src_rotate_group_offset_256_64[rg_offset/(8/a_length)]);
            }

            for(rg_offset=0; rg_offset<(8/a_length); rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<(8/a_length) * num_iter_dst; rg_offset++){    
                void *dst_rank_addr = src_rank_base_addr_arr[rg_offset/(8/a_length)] + mram_dst_offset_1mb_wise[rg_offset%(8/a_length)];
                dst_rank_addr_array[rg_offset]=(dst_rank_addr + src_rotate_group_offset_256_64[rg_offset/(8/a_length)]);
            }

            S_SUM_AR_24(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, a_length);

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_reduce_rg_22(void *base_region_addr_dst, void **base_region_addr_src, uint32_t dst_rg_id, uint32_t* src_rg_id, uint32_t iter_dst_a, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_axis_x, uint32_t comm_axis_y, uint32_t comm_axis_z, uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t size){

    void *dst_rank_base_addr = base_region_addr_dst;
    void *src_rank_base_addr_arr[num_iter_dst];
    uint32_t src_mram_offset;
    uint32_t src_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_src_offset_1mb_wise[8];
    for(uint32_t i=0; i<num_iter_dst; i++){
        src_rank_base_addr_arr[i]=base_region_addr_src[i];
        src_rotate_group_offset_256_64[i] = (src_rg_id[i]%4) * (256*1024) + (src_rg_id[i]/4) * 64;
    }
    src_mram_offset = src_start_offset + 1024*1024;
    
    int packet_size = 8;
    int iteration = packet_size / 8;
    int64_t mram_dst_offset_1mb_wise[8];

    uint32_t dst_mram_offset=0;
    if(comm_axis_x) dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length * 4 * iter_dst_a;
    else dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + byte_length * 2 * iter_dst_a;

    uint32_t remain_length;
    remain_length=byte_length/8;

    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    
    if(comm_axis_x && !comm_axis_y && comm_axis_z){
        {
            for(uint32_t i=0; i<remain_length; i++){
                void *dst_rank_addr_array[num_iter_dst * 4];
                dst_rank_addr_array[0] = dst_rank_base_addr + dst_rotate_group_offset_256_64;
                void *src_rank_addr_array[num_iter_dst * 4];
                uint32_t mram_64_bit_word_offset;
                uint64_t next_data;

                for(uint32_t j=0; j<num_iter_dst; j++){
                    for(uint32_t k=0; k<4; k++){
                        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+byte_length*k);
                        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                        mram_src_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                        void *src_rank_addr = src_rank_base_addr_arr[j] + mram_src_offset_1mb_wise[k];
                        src_rank_addr_array[j*4 + k]=(src_rank_addr+src_rotate_group_offset_256_64[j]);
                    }
                }

                for(uint32_t j=0; j<num_iter_dst; j++){
                    for(uint32_t k=0; k<4; k++){
                        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+byte_length*k);
                        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                        mram_dst_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                        void *dst_rank_addr = src_rank_base_addr_arr[j] + mram_dst_offset_1mb_wise[k];
                        dst_rank_addr_array[j*4 + k]=(dst_rank_addr+src_rotate_group_offset_256_64[j]);
                    }
                }
                
                RNS_SUM_AR_22(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, 4);
                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    else if(!comm_axis_x && comm_axis_y && !comm_axis_z){
        
        for(uint32_t i=0; i<remain_length; i++){
            void *dst_rank_addr_array[2*num_iter_dst];
            void *src_rank_addr_array[2*num_iter_dst];
            uint32_t rg_offset = 0;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            for(rg_offset=0; rg_offset<2; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<2 * num_iter_dst; rg_offset++){    
                void *src_rank_addr = src_rank_base_addr_arr[rg_offset/2] + mram_src_offset_1mb_wise[rg_offset%2];
                src_rank_addr_array[rg_offset]=(src_rank_addr + src_rotate_group_offset_256_64[rg_offset/2]);
            }

            for(rg_offset=0; rg_offset<2; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<2 * num_iter_dst; rg_offset++){    
                void *dst_rank_addr = src_rank_base_addr_arr[rg_offset/2] + mram_dst_offset_1mb_wise[rg_offset%2];
                dst_rank_addr_array[rg_offset]=(dst_rank_addr + src_rotate_group_offset_256_64[rg_offset/2]);
            }

            S_SUM_AR_22(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, 4);

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_reduce_scatter_cpu_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length,\
                                             uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t num_thread, uint32_t thread_id){

    void *src_rank_base_addr;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;

    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=size*0;
    uint32_t dst_mram_offset=0;
    if(alltoall_comm_type==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024 + communication_buffer_offset;
        dst_mram_offset = dst_start_offset + 1024*1024;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }

    uint32_t iter_length = byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    uint32_t src_rotate_group_offset_256_64;

    void *src_rank_addr_iter[num_iter_src * 8];
    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;
    void *src_rank_addr;

    //threaded
    src_mram_offset = src_start_offset + 1024*1024 + communication_buffer_offset + 8*(iter_length/num_thread)*thread_id;
    dst_mram_offset = dst_start_offset + 1024*1024  + 8*(iter_length/num_thread)*thread_id;

    //for(uint32_t i=0; i<iter_length; i++){
    for(uint32_t i=(iter_length/num_thread)*thread_id; i<(iter_length/num_thread)*(thread_id+1); i++){
            
        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
        mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
        void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
        void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

        for(uint32_t j=0; j<num_iter_src; j++){
            src_rank_base_addr = base_region_addr_src[j];
            src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<8; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                src_rank_addr_iter[j*8 + k] = src_rank_addr + src_rotate_group_offset_256_64;
            }
        }

        RNS_SUM_RS(iteration, src_rank_addr_iter, dst_rank_addr_iter, num_iter_src, size);

        _mm_mfence();

        for(uint32_t j=0; j<num_iter_src*8; j++){
            RNS_FLUSH_SRC(iteration, src_rank_addr_iter[j]);
        }

        RNS_FLUSH_DST(iteration, dst_rank_addr_iter);


        src_mram_offset+=8;
        dst_mram_offset+=8;
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_reduce_scatter_cpu_y_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size){

    void *src_rank_base_addr;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    if(alltoall_comm_type==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024 + communication_buffer_offset;
        dst_mram_offset = dst_start_offset + 1024*1024;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024+ communication_buffer_offset;
        dst_mram_offset = dst_start_offset + 1024*1024 ;
    }

    uint32_t iter_length = byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
    uint32_t src_rotate_group_offset_256_64;

    void *src_rank_addr_iter[num_iter_src * 8];
    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;
    void *src_rank_addr;

    for(uint32_t i=0; i<iter_length; i++){
            
        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
        mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
        void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
        void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

        for(uint32_t j=0; j<num_iter_src; j++){
            src_rank_base_addr = base_region_addr_src[j];
            src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

            for(uint32_t k=0; k<1; k++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset + byte_length*k);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                src_rank_addr_iter[j + k] = src_rank_addr + src_rotate_group_offset_256_64;
            }
        }

        for(uint32_t j=0; j<num_iter_src; j++){
            RNS_FLUSH_SRC(iteration, src_rank_addr_iter[j]);
        }
        
        RNS_FLUSH_DST(iteration, dst_rank_addr_iter);
        _mm_mfence();

        S_SUM_RS(iteration, src_rank_addr_iter, dst_rank_addr_iter, num_iter_src, size);

        _mm_mfence();

        for(uint32_t j=0; j<num_iter_src; j++){
            RNS_FLUSH_SRC(iteration, src_rank_addr_iter[j]);
        }        
        RNS_FLUSH_DST(iteration, dst_rank_addr_iter);

        src_mram_offset+=8;
        dst_mram_offset+=8;
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_reduce_scatter_cpu_rg_24(void *base_region_addr_dst, void **base_region_addr_src, uint32_t dst_rg_id, uint32_t* src_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t allgather_comm_type, uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t a_length, uint32_t size){

    void *dst_rank_base_addr = base_region_addr_dst;
    void *src_rank_base_addr_arr[num_iter_dst]; 
    uint32_t src_mram_offset;
    uint32_t src_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_src_offset_1mb_wise[8];
    for(uint32_t i=0; i<num_iter_dst; i++){
        src_rank_base_addr_arr[i]=base_region_addr_src[i];
        src_rotate_group_offset_256_64[i] = (src_rg_id[i]%4) * (256*1024) + (src_rg_id[i]/4) * 64;
    }
    src_mram_offset = src_start_offset + 1024*1024 + communication_buffer_offset;
    
    int packet_size = 8;
    int iteration = packet_size / 8;
    int64_t mram_dst_offset_1mb_wise[8];

    uint32_t dst_mram_offset=0;
    dst_mram_offset = dst_start_offset + 1024*1024;
    
    uint32_t remain_length;
    remain_length=byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64; //because of bank structure

    
    if(allgather_comm_type==0){
        for(uint32_t i=0; i<remain_length; i++){
            void *src_rank_addr_array[num_iter_dst*a_length];
            void *dst_rank_addr_array;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise[0] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise[0];
            dst_rank_addr_array=(dst_rank_addr + dst_rotate_group_offset_256_64);

            for(uint32_t j=0; j<num_iter_dst; j++){
                for(uint32_t k=0; k<a_length; k++){
                    mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+byte_length*k);
                    next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                    mram_src_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                    void *src_rank_addr = src_rank_base_addr_arr[j] + mram_src_offset_1mb_wise[k];
                    src_rank_addr_array[j*a_length + k]=(src_rank_addr+src_rotate_group_offset_256_64[j]);
                }
            }

            //flush before RNS
            _mm_mfence();
            RNS_FLUSH_DST(iteration, dst_rank_addr_array);
            _mm_mfence();

            //RnS
            REDUCE_SCATTER_CPU_X_24(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, a_length);
            _mm_mfence();

            for(uint32_t j=0; j<num_iter_dst*a_length; j++){
                RNS_FLUSH_DST(iteration, src_rank_addr_array[j]);
            }
            RNS_FLUSH_DST(iteration, dst_rank_addr_array);
            _mm_mfence();
            

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    else{ //perpendicular to entangled group

        for(uint32_t i=0; i<remain_length; i++){ 
            void *src_rank_addr_array[(8/a_length)*num_iter_dst];
            //void *dst_rank_addr_array[num_iter_dst];
            void *dst_rank_addr_array[1];
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            for(rg_offset=0; rg_offset<(8/a_length); rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<(8/a_length) * num_iter_dst; rg_offset++){    
                void *src_rank_addr = src_rank_base_addr_arr[rg_offset/(8/a_length)] + mram_src_offset_1mb_wise[rg_offset%(8/a_length)];
                src_rank_addr_array[rg_offset]=(src_rank_addr + src_rotate_group_offset_256_64[rg_offset/(8/a_length)]);
            }

            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise[0] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise[0];
            dst_rank_addr_array[0]=(dst_rank_addr + dst_rotate_group_offset_256_64);

            //flush before RNS
            _mm_mfence();
            for(uint32_t j=0; j<num_iter_dst*(8/a_length); j++){
                RNS_FLUSH_DST(iteration, dst_rank_addr_array[j]);
            }
            _mm_mfence();

            //RnS
            REDUCE_SCATTER_CPU_Y_24(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, a_length);
            
            _mm_mfence();

            for(uint32_t j=0; j<num_iter_dst*(8/a_length); j++){
                RNS_FLUSH_DST(iteration, dst_rank_addr_array[j]);
            }
            RNS_FLUSH_DST(iteration, src_rank_addr_array[0]);
            _mm_mfence();

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_reduce_scatter_cpu_rg_22(void *base_region_addr_dst, void **base_region_addr_src, uint32_t dst_rg_id, uint32_t* src_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_axis_x, uint32_t comm_axis_y, uint32_t comm_axis_z, uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t size){

    void *dst_rank_base_addr = base_region_addr_dst;
    void *src_rank_base_addr_arr[num_iter_dst]; 
    uint32_t src_mram_offset;
    uint32_t src_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_src_offset_1mb_wise[8];
    for(uint32_t i=0; i<num_iter_dst; i++){
        src_rank_base_addr_arr[i]=base_region_addr_src[i];
        src_rotate_group_offset_256_64[i] = (src_rg_id[i]%4) * (256*1024) + (src_rg_id[i]/4) * 64;
    }
    src_mram_offset = src_start_offset + 1024*1024 + communication_buffer_offset;
    
    int packet_size = 8;
    int iteration = packet_size / 8;
    int64_t mram_dst_offset_1mb_wise[8];

    uint32_t dst_mram_offset=0;
    dst_mram_offset = dst_start_offset + 1024*1024;
    
    uint32_t remain_length;
    remain_length=byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64; //because of bank structure

    
    if(comm_axis_x && !comm_axis_y && comm_axis_z){

        for(uint32_t i=0; i<remain_length; i++){
            void *src_rank_addr_array[num_iter_dst*4];
            void *dst_rank_addr_array;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise[0] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise[0];
            dst_rank_addr_array=(dst_rank_addr + dst_rotate_group_offset_256_64);

            for(uint32_t j=0; j<num_iter_dst; j++){
                for(uint32_t k=0; k<4; k++){
                    mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+byte_length*k);
                    next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                    mram_src_offset_1mb_wise[k] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                    void *src_rank_addr = src_rank_base_addr_arr[j] + mram_src_offset_1mb_wise[k];
                    src_rank_addr_array[j*4 + k]=(src_rank_addr+src_rotate_group_offset_256_64[j]);
                }
            }

            //flush before RNS
            _mm_mfence();
            RNS_FLUSH_DST(iteration, dst_rank_addr_array);
            _mm_mfence();

            //RnS
            REDUCE_SCATTER_CPU_X_22(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, 4);
            _mm_mfence();

            for(uint32_t j=0; j<4*num_iter_dst; j++){
                RNS_FLUSH_DST(iteration, src_rank_addr_array[j]);
            }
            RNS_FLUSH_DST(iteration, dst_rank_addr_array);
            _mm_mfence();
            

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    else if(!comm_axis_x && comm_axis_y && !comm_axis_z){
        for(uint32_t i=0; i<remain_length; i++){ 
            void *src_rank_addr_array[2*num_iter_dst];
            //void *dst_rank_addr_array[num_iter_dst];
            void *dst_rank_addr_array[1];
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset;
            uint64_t next_data;

            for(rg_offset=0; rg_offset<2; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<2 * num_iter_dst; rg_offset++){    
                void *src_rank_addr = src_rank_base_addr_arr[rg_offset/2] + mram_src_offset_1mb_wise[rg_offset%2];
                src_rank_addr_array[rg_offset]=(src_rank_addr + src_rotate_group_offset_256_64[rg_offset/2]);
            }

            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise[0] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise[0];
            dst_rank_addr_array[0]=(dst_rank_addr + dst_rotate_group_offset_256_64);

            //flush before RNS
            _mm_mfence();
            for(uint32_t j=0; j<num_iter_dst*2; j++){
            //for(uint32_t j=0; j<num_iter_dst; j++){
                RNS_FLUSH_DST(iteration, dst_rank_addr_array[j]);
            }
            _mm_mfence();

            //RnS
            REDUCE_SCATTER_CPU_Y_22(iteration, src_rank_addr_array, dst_rank_addr_array, num_iter_dst, size, 4);
            
            _mm_mfence();

            for(uint32_t j=0; j<num_iter_dst*2; j++){
            //for(uint32_t j=0; j<num_iter_dst; j++){
                RNS_FLUSH_DST(iteration, dst_rank_addr_array[j]);
            }
            RNS_FLUSH_DST(iteration, src_rank_addr_array[0]);
            _mm_mfence();

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_to_all_rg(void *base_region_addr_src, void *base_region_addr_dst, uint32_t src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length,\
                                     uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t num_thread, uint32_t thread_id){
    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    int src_off_gran = 128 * iteration;
    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;

    if(alltoall_comm_type==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024 ;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }

    uint32_t iter_length = num_thread*thread_id;
    uint32_t remain_length;
    if(src_start_offset%64==0 && dst_start_offset%64==0){
        iter_length=byte_length/64;
        remain_length = (byte_length%64 + 7)/8;
    }
    else{
        iter_length=0;
        remain_length = byte_length/8;
    }

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;



    if(alltoall_comm_type==0){ //parallel to the entangled group

        /* //threaded
        src_mram_offset = src_start_offset + 1024*1024 + 8*(byte_length/num_thread)*thread_id;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + 8*(byte_length/num_thread)*thread_id; */
        
        for(uint32_t rns_chip_id=0; rns_chip_id<8; rns_chip_id++){
        //for(uint32_t rns_chip_id=(8/num_thread)*thread_id; rns_chip_id<(8/num_thread)*(thread_id+1); rns_chip_id++){
            for(uint32_t i=0; i<iter_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;

                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                    src_rank_addr_before_rns += (src_off_gran);
                }
                _mm_mfence();
                
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
                if(rns_chip_id==0){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==1){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(1, 8, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==2){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==3){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(3, 24, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==4){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==5){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(5, 40, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==6){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(6, 48, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==7){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(7, 56, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }

                _mm_mfence();

                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                    src_rank_addr_after_rns += (src_off_gran);
                }
            
                void *dst_rank_addr_after_rns = dst_rank_addr+ dst_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                    dst_rank_addr_after_rns += (src_off_gran);
                }
                
                src_mram_offset+=64;
                dst_mram_offset+=64;
            }

            for(uint32_t i=0; i<remain_length; i++){

                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;

                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                _mm_mfence();

                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
                if(rns_chip_id==0){
                    RNS_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==1){
                    RNS_COPY_a2a(1, 8, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==2){
                    RNS_COPY_a2a(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==3){
                    RNS_COPY_a2a(3, 24, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==4){
                    RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==5){
                    RNS_COPY_a2a(5, 40, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==6){
                    RNS_COPY_a2a(6, 48, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==7){
                    RNS_COPY_a2a(7, 56, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }

                _mm_mfence();

                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                
                //flush dst
                void *dst_rank_addr_after_rns = dst_rank_addr+ dst_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                _mm_mfence();

                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
        
    }
    else{ //perpendicular to the entangled group

        for(uint32_t i=0; i<iter_length; i++){
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

            void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;

            //flush before RnS
            void *src_rank_addr_before_rns = src_rank_addr+ (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
            for(int j=0; j<8; j++){
                COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                src_rank_addr_before_rns += (src_off_gran);
            }
            
            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
            for(int iter_8=0; iter_8<8; iter_8++){
                S_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                src_rank_addr_iter += (src_off_gran);
                dst_rank_addr_iter += (src_off_gran);
            }

            void *src_rank_addr_after_rns = src_rank_addr+ (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
            for(int j=0; j<8; j++){
                COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                src_rank_addr_after_rns += (src_off_gran);
            }
        
            void *dst_rank_addr_after_rns = dst_rank_addr+ (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
            for(int j=0; j<8; j++){
                COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                dst_rank_addr_after_rns += (src_off_gran);
            }

            src_mram_offset+=64;
            dst_mram_offset+=64;
        }
        for(uint32_t i=0; i<remain_length; i++){
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

            void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;

            //flush before RnS
            void *src_rank_addr_before_rns = src_rank_addr+ (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
            for(int j=0; j<1; j++){
                COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                src_rank_addr_before_rns += (src_off_gran);
            }
            
            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
            S_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);

            void *src_rank_addr_after_rns = src_rank_addr+ (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
            for(int j=0; j<1; j++){
                COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                src_rank_addr_after_rns += (src_off_gran);
            }
        
            //flush dst
            void *dst_rank_addr_after_rns = dst_rank_addr+ (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;
            for(int j=0; j<1; j++){
                COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                dst_rank_addr_after_rns += (src_off_gran);
            }
            
            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_to_all_rg_24(void *base_region_addr_src, void *base_region_addr_dst, uint32_t src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t a_length){
    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    int src_off_gran = 128 * iteration;
    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    if(alltoall_comm_type==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }

    uint32_t iter_length;
    uint32_t remain_length;
    if(src_start_offset%64==0 && dst_start_offset%64==0){
        iter_length=byte_length/64;
        remain_length = (byte_length%64 + 7)/8;
    }
    else{
        iter_length=0;
        remain_length = byte_length/8;
    }

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;

    if(alltoall_comm_type==0){
        for(uint32_t rns_chip_id=0; rns_chip_id<a_length; rns_chip_id++){
            for(uint32_t i=0; i<iter_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RNS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                    src_rank_addr_before_rns += (src_off_gran);
                }
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

                if(rns_chip_id==0){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a_24(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==1){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a_24(1, 8, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==2){ //If a_len==2, it doesn't work.
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a_24(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==3){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a_24(3, 24, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                _mm_mfence();

                //flush after RNS
                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                    src_rank_addr_after_rns += (src_off_gran);
                }
                _mm_mfence();
                
                src_mram_offset+=64;
                dst_mram_offset+=64;
            }
            for(uint32_t i=0; i<remain_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RNS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
                if(rns_chip_id==0){
                    RNS_COPY_a2a_24(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                }
                else if(rns_chip_id==1){
                    RNS_COPY_a2a_24(1, 8, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                }
                else if(rns_chip_id==2){
                    RNS_COPY_a2a_24(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                }
                else if(rns_chip_id==3){
                    RNS_COPY_a2a_24(3, 24, iteration, src_rank_addr_iter, dst_rank_addr_iter, a_length);
                }
                _mm_mfence();

                //flush after RNS
                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                _mm_mfence();

                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    else{ //alltoall_comm_type==1
        for(uint32_t rns_chip_id=0; rns_chip_id<(8/a_length); rns_chip_id++){
            for(uint32_t i=0; i<iter_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RnS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                    src_rank_addr_before_rns += (src_off_gran);
                }
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
                if(rns_chip_id==0){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==1 && a_length==4){ //a_length==4
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==1){ //a_length==2
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==2){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                else if(rns_chip_id==3){
                    for(int iter_8=0; iter_8<8; iter_8++){
                        RNS_COPY_a2a(6, 48, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                        src_rank_addr_iter += (src_off_gran);
                        dst_rank_addr_iter += (src_off_gran);
                    }
                }
                _mm_mfence();

                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                    src_rank_addr_after_rns += (src_off_gran);
                }
            
                //flush dst
                void *dst_rank_addr_after_rns = dst_rank_addr+ dst_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                    dst_rank_addr_after_rns += (src_off_gran);
                }
                _mm_mfence();

                src_mram_offset+=64;
                dst_mram_offset+=64;
            }
            for(uint32_t i=0; i<remain_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RnS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

                if(rns_chip_id==0){
                    RNS_COPY_a2a(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==1 && a_length==4){
                    RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==1){
                    RNS_COPY_a2a(2, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==2){
                    RNS_COPY_a2a(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==3){
                    RNS_COPY_a2a(6, 48, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                _mm_mfence();

                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                
                //flush dst
                void *dst_rank_addr_after_rns = dst_rank_addr+ dst_rotate_group_offset_256_64;
                COMM_FLUSH_a2a(iteration, dst_rank_addr_after_rns);
                _mm_mfence();

                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_to_all_rg_22(void *base_region_addr_src, void *base_region_addr_dst, uint32_t src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_axis_x, uint32_t comm_axis_y, uint32_t comm_axis_z, uint32_t communication_buffer_offset){
    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    int src_off_gran = 128 * iteration;
    int64_t mram_src_offset_1mb_wise=0;
    int64_t mram_dst_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    if(comm_axis_x==0){ //x-axis include O
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }
    else{ //x-axis include X
        src_mram_offset = src_start_offset + 1024*1024;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;
    }

    uint32_t iter_length;

    iter_length=byte_length/8;

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;

    if(comm_axis_x && !comm_axis_y && comm_axis_z){
        for(uint32_t rns_chip_id=0; rns_chip_id<4; rns_chip_id++){
            for(uint32_t i=0; i<iter_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RNS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                }
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

                if(rns_chip_id==0){
                    RNS_COPY_a2a_22_xz(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==1){
                    RNS_COPY_a2a_22_xz(1, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==2){
                    RNS_COPY_a2a_22_xz(4, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==3){
                    RNS_COPY_a2a_22_xz(5, 32, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                _mm_mfence();

                //flush after RNS
                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                    src_rank_addr_after_rns += (src_off_gran);
                }
                _mm_mfence();
                
                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }

    else if(!comm_axis_x && comm_axis_y && !comm_axis_z){
        for(uint32_t rns_chip_id=0; rns_chip_id<2; rns_chip_id++){
            for(uint32_t i=0; i<iter_length; i++){
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;

                void *src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
                
                //flush before RNS
                _mm_mfence();
                void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_before_rns);
                }
                _mm_mfence();

                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;

                if(rns_chip_id==0){
                    RNS_COPY_a2a_22_y(0, 0, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                else if(rns_chip_id==1){
                    RNS_COPY_a2a_22_y(1, 16, iteration, src_rank_addr_iter, dst_rank_addr_iter);
                }
                _mm_mfence();

                //flush after RNS
                void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
                for(int j=0; j<8; j++){
                    COMM_FLUSH_a2a(iteration, src_rank_addr_after_rns);
                    src_rank_addr_after_rns += (src_off_gran);
                }
                _mm_mfence();
                
                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_gather_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer, uint32_t host_buffer_first_index){
    src_rg_id[0]+=0;
    base_region_addr_dst+=0;
    src_start_offset+=0;
    dst_rg_id+=0;
    void *src_rank_base_addr=base_region_addr_src[0];
    src_rank_base_addr+=0;
    num_iter_src+=0;
    communication_buffer_offset+=0;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    iteration+=0;
    
    alltoall_comm_type+=0;
    int64_t mram_dst_offset_1mb_wise=0;
    uint32_t dst_mram_offset = dst_start_offset + 1024*1024;
    dst_mram_offset = dst_start_offset + 1024*1024;

    uint32_t iter_length = byte_length/8;
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;

    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;

    if(alltoall_comm_type==0){
        for(uint32_t i=0; i<iter_length; i++){
            
            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
            void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
            
            void *dst_host_buffer = (*(host_buffer+host_buffer_first_index)+ (64)*i);

            for(int iter_8=0; iter_8<1; iter_8++){
                GATHER_COPY(iteration, dst_rank_addr_iter, dst_host_buffer);
            }
            dst_mram_offset+=8;
        }
    }
    
    _mm_mfence();
    return;
}

void xeon_sp_trans_scatter_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer, uint32_t host_buffer_first_index){
    src_rg_id[0]+=0;
    src_start_offset+=0;
    dst_rg_id+=0;

    void *src_rank_base_addr=base_region_addr_src[0];
    src_rank_base_addr+=0;
    num_iter_src+=0;
    communication_buffer_offset+=0;
    void *dst_rank_base_addr = base_region_addr_dst;
    int packet_size = 8;
    int iteration = packet_size / 8;
    iteration+=0;
    
    alltoall_comm_type+=0;
    int64_t mram_dst_offset_1mb_wise=0;
    uint32_t dst_mram_offset = dst_start_offset + 1024*1024;

    uint32_t iter_length = byte_length/8;
    
    uint32_t dst_rotate_group_offset_256_64= (dst_rg_id%4) * (256*1024) + (dst_rg_id/4) * 64;

    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;

    //uint32_t flag=1;
    for(uint32_t i=0; i<iter_length; i++){
        
        mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
        next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
        mram_dst_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
        void *dst_rank_addr = dst_rank_base_addr + mram_dst_offset_1mb_wise;
        void *dst_rank_addr_iter = dst_rank_addr + dst_rotate_group_offset_256_64;
        
        void *dst_host_buffer = (*(host_buffer+host_buffer_first_index)+ (64)*i);

        for(int iter_8=0; iter_8<1; iter_8++){                    
            SCATTER_COPY(iteration, dst_host_buffer, dst_rank_addr_iter);
        }

        dst_mram_offset+=8;
    }

    
    _mm_mfence();
    return;
}

void xeon_sp_trans_reduce_rg(void **base_region_addr_src, void *base_region_addr_dst, uint32_t* src_rg_id, uint32_t dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t total_length, uint32_t num_iter_src, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer, uint32_t host_buffer_first_index, uint32_t data_type){
    base_region_addr_dst+=0;
    dst_rg_id+=0;
    communication_buffer_offset+=0;
    void *src_rank_base_addr;
    int packet_size = 8;
    int iteration = packet_size / 8;
    alltoall_comm_type+=0;
    int64_t mram_src_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    uint32_t dst_mram_offset=0;
    src_mram_offset = src_start_offset + 1024*1024;
    dst_mram_offset = dst_start_offset + 1024*1024;

    uint32_t iter_length= byte_length/8; 
    uint32_t total_iter_length = total_length/64;
    total_iter_length += 0;

    uint32_t src_rotate_group_offset_256_64;

    void *src_rank_addr_iter[num_iter_src * 8];
    uint32_t mram_64_bit_word_offset;
    uint64_t next_data;
    void *src_rank_addr;

    if(alltoall_comm_type==0){ //parallel to the entangled group
        for(uint32_t i=0; i<iter_length; i++){
            

            for(uint32_t j=0; j<num_iter_src; j++){
                src_rank_base_addr = base_region_addr_src[j];
                src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

                for(uint32_t k=0; k<8; k++){
                    mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset + byte_length*k);
                    next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                    mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                    src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                    src_rank_addr_iter[j*8 + k] = src_rank_addr + src_rotate_group_offset_256_64;
                }
            }
            void *dst_host_buffer = (*(host_buffer+(host_buffer_first_index/num_iter_src))+ (64)*i + 64*(host_buffer_first_index % num_iter_src));
            (dst_host_buffer)+=0;
            for(int iter_8=0; iter_8<1; iter_8++){
                REDUCE_RNS_SUM_RS(iteration, src_rank_addr_iter, dst_host_buffer, num_iter_src, data_type);
            }

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    else{ //perpendicular to the entangled group
        for(uint32_t i=0; i<iter_length; i++){

            for(uint32_t j=0; j<num_iter_src; j++){
                src_rank_base_addr = base_region_addr_src[j];
                src_rotate_group_offset_256_64= (src_rg_id[j]%4) * (256*1024) + (src_rg_id[j]/4) * 64;

                for(uint32_t k=0; k<1; k++){
                    mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                    next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                    mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                    src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;
                    src_rank_addr_iter[j + k] = src_rank_addr + src_rotate_group_offset_256_64;
                }
            }

            void *dst_host_buffer = (*(host_buffer+(host_buffer_first_index/num_iter_src))+ (64)*i) + 64*iter_length*(host_buffer_first_index % num_iter_src);
            for(int iter_8=0; iter_8<1; iter_8++){
                REDUCE_S_SUM_RS(iteration, src_rank_addr_iter, dst_host_buffer, num_iter_src, data_type);
            }

            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    
    _mm_mfence();
    return;
}


//all_gather
void xeon_sp_trans_all_gather_rg(void *base_region_addr_src, void **base_region_addr_dst, uint32_t src_rg_id, uint32_t* dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t allgather_comm_type, \
                                                         uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t num_thread, uint32_t thread_id){

    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr_arr[num_iter_dst];
    uint32_t dst_mram_offset;
    uint32_t dst_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_dst_offset_1mb_wise[8];


    for(uint32_t i=0; i<num_iter_dst; i++){
        dst_rank_base_addr_arr[i]=base_region_addr_dst[i];

        dst_rotate_group_offset_256_64[i] = (dst_rg_id[i]%4) * (256*1024) + (dst_rg_id[i]/4) * 64;
    }
    dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;

    int64_t mram_src_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    src_mram_offset = src_start_offset + 1024*1024;

    uint32_t iter_length;
    //uint32_t remain_length=0;

    iter_length = byte_length/8;

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    
    if(allgather_comm_type==0){ //parallel to the entangled group

        //threaded
        src_mram_offset = src_start_offset + 1024*1024 + 8*(iter_length/num_thread)*thread_id;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + 8*(iter_length/num_thread)*thread_id;

        for(uint32_t i=(iter_length/num_thread)*thread_id; i<(iter_length/num_thread)*(thread_id+1); i++){
            void *dst_rank_addr_array[8*num_iter_dst];
            void *src_rank_addr;
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            int64_t mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

            for(rg_offset=0; rg_offset<8; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<8*num_iter_dst; rg_offset++){    
                void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset/8] + mram_dst_offset_1mb_wise[rg_offset%8];
                dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset/8]);
            }

            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            for(int iter_8=0; iter_8<1; iter_8++){

                RNS_COPY_ag(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array);
            }

            RNS_FLUSH_SRC(1, src_rank_addr_iter);
            for(uint32_t j=0; j<num_iter_dst*8; j++){
                RNS_FLUSH_DST(1, dst_rank_addr_array[j]);
            }

            src_mram_offset+=8;
            dst_mram_offset+=8;

            
        }
        
    }

        
    else{ //perepndicular to the entangled group
        
        src_mram_offset = src_start_offset + 1024*1024 + 8*(iter_length/num_thread)*thread_id;
        dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset + 8*(iter_length/num_thread)*thread_id;

        for(uint32_t i=(iter_length/num_thread)*thread_id; i<(iter_length/num_thread)*(thread_id+1); i++){
            void *dst_rank_addr_array[num_iter_dst];
            void *src_rank_addr;
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

            mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset);
            next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_dst_offset_1mb_wise[0] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a; //x-axis아니라서 통신하는 애들은 rg내의 번호가 모두 같음
            for(rg_offset=0; rg_offset<num_iter_dst; rg_offset++){    
                void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset] + mram_dst_offset_1mb_wise[0];
                dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset]);
            }

            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            
            S_COPY_ag(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array);

            RNS_FLUSH_SRC(1, src_rank_addr_iter);
            for(uint32_t j=0; j<num_iter_dst; j++){
                RNS_FLUSH_DST(1, dst_rank_addr_array[j]);
            }
            
            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_gather_rg_24(void *base_region_addr_src, void **base_region_addr_dst, uint32_t src_rg_id, uint32_t* dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t allgather_comm_type,\
                                                                         uint32_t communication_buffer_offset, uint32_t num_iter_dst, uint32_t a_length){
    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr_arr[num_iter_dst];
    uint32_t dst_mram_offset;
    uint32_t dst_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_dst_offset_1mb_wise[8];

    for(uint32_t i=0; i<num_iter_dst; i++){
        dst_rank_base_addr_arr[i]=base_region_addr_dst[i];
        dst_rotate_group_offset_256_64[i] = (dst_rg_id[i]%4) * (256*1024) + (dst_rg_id[i]/4) * 64;
    }
    dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;

    int64_t mram_src_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    src_mram_offset = src_start_offset + 1024*1024;

    uint32_t remain_length;

    remain_length = byte_length/8;

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    
    if(allgather_comm_type==0){
        {
            for(uint32_t i=0; i<remain_length; i++){
                void *dst_rank_addr_array[a_length*num_iter_dst];
                void *src_rank_addr;
                uint32_t rg_offset=0;
                uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
                uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

                for(rg_offset=0; rg_offset<a_length; rg_offset++){
                    mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                    next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                    mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
                }
                for(rg_offset=0; rg_offset<a_length*num_iter_dst; rg_offset++){    
                    void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset/a_length] + mram_dst_offset_1mb_wise[rg_offset%a_length];
                    dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset/a_length]);
                }
                
                //RnS
                void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
                RNS_COPY_ag_24(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array, a_length);
                
                src_mram_offset+=8;
                dst_mram_offset+=8;
            }
        }
    }
    else{ //allgather_comm_type==1
       
        for(uint32_t i=0; i<remain_length; i++){
            void *dst_rank_addr_array[(8/a_length) * num_iter_dst];
            void *src_rank_addr;
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

            for(rg_offset=0; rg_offset<(8/a_length); rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<(8/a_length) * num_iter_dst; rg_offset++){
                void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset/(8/a_length)] + mram_dst_offset_1mb_wise[rg_offset%(8/a_length)]; //!
                dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset/(8/a_length)]);
            }

            void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
            COMM_FLUSH_a2a(1, src_rank_addr_before_rns);
            
            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            S_COPY_ag_24(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array, a_length);

            void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
            COMM_FLUSH_a2a(1, src_rank_addr_after_rns);
            
            //flush dst
            for(rg_offset=0; rg_offset<(8/a_length)*num_iter_dst; rg_offset++){
                void *dst_rank_addr_after_rns = dst_rank_addr_array[rg_offset];
                COMM_FLUSH_a2a(1, dst_rank_addr_after_rns);
            }
            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}

void xeon_sp_trans_all_gather_rg_22(void *base_region_addr_src, void **base_region_addr_dst, uint32_t src_rg_id, uint32_t* dst_rg_id, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length,\
                                             uint32_t comm_axis_x, uint32_t comm_axis_y, uint32_t comm_axis_z, uint32_t communication_buffer_offset, uint32_t num_iter_dst){
    void *src_rank_base_addr = base_region_addr_src;
    void *dst_rank_base_addr_arr[num_iter_dst];
    uint32_t dst_mram_offset;
    uint32_t dst_rotate_group_offset_256_64[num_iter_dst];
    int64_t mram_dst_offset_1mb_wise[8];

    for(uint32_t i=0; i<num_iter_dst; i++){
        dst_rank_base_addr_arr[i]=base_region_addr_dst[i];
        dst_rotate_group_offset_256_64[i] = (dst_rg_id[i]%4) * (256*1024) + (dst_rg_id[i]/4) * 64;
    }
    dst_mram_offset = dst_start_offset + 1024*1024 + communication_buffer_offset;

    int64_t mram_src_offset_1mb_wise=0;

    uint32_t src_mram_offset=0;
    src_mram_offset = src_start_offset + 1024*1024;

    uint32_t remain_length;

    remain_length = byte_length/8;

    uint32_t src_rotate_group_offset_256_64= (src_rg_id%4) * (256*1024) + (src_rg_id/4) * 64;
    
    if(comm_axis_x && !comm_axis_y && comm_axis_z){
        for(uint32_t i=0; i<remain_length; i++){
            void *dst_rank_addr_array[4*num_iter_dst];
            void *src_rank_addr;
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

            for(rg_offset=0; rg_offset<4; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<4*num_iter_dst; rg_offset++){    
                void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset/4] + mram_dst_offset_1mb_wise[rg_offset%4];
                dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset/4]);
            }
            
            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            RNS_COPY_ag_22(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array, 4);
            
            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    else if(!comm_axis_x && comm_axis_y && !comm_axis_z){
        for(uint32_t i=0; i<remain_length; i++){
            void *dst_rank_addr_array[2 * num_iter_dst];
            void *src_rank_addr;
            uint32_t rg_offset=0;
            uint32_t mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(src_mram_offset);
            uint64_t next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
            mram_src_offset_1mb_wise = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            src_rank_addr = src_rank_base_addr + mram_src_offset_1mb_wise;

            for(rg_offset=0; rg_offset<2; rg_offset++){
                mram_64_bit_word_offset = apply_address_translation_on_mram_offset_a2a(dst_mram_offset+(rg_offset)*byte_length);
                next_data = BANK_OFFSET_NEXT_DATA_a2a(mram_64_bit_word_offset);
                mram_dst_offset_1mb_wise[rg_offset] = (next_data % BANK_CHUNK_SIZE_a2a) + (next_data / BANK_CHUNK_SIZE_a2a) * BANK_NEXT_CHUNK_OFFSET_a2a;
            }
            for(rg_offset=0; rg_offset<2 * num_iter_dst; rg_offset++){
                void *dst_rank_addr = dst_rank_base_addr_arr[rg_offset/2] + mram_dst_offset_1mb_wise[rg_offset%2]; //!
                dst_rank_addr_array[rg_offset]=(dst_rank_addr+dst_rotate_group_offset_256_64[rg_offset/2]);
            }

            void *src_rank_addr_before_rns = src_rank_addr+ src_rotate_group_offset_256_64;
            COMM_FLUSH_a2a(1, src_rank_addr_before_rns);
            
            //RnS
            void *src_rank_addr_iter = src_rank_addr + src_rotate_group_offset_256_64;
            S_COPY_ag_22(0, 0, num_iter_dst, src_rank_addr_iter, dst_rank_addr_array, 4);

            void *src_rank_addr_after_rns = src_rank_addr+ src_rotate_group_offset_256_64;
            COMM_FLUSH_a2a(1, src_rank_addr_after_rns);
            
            //flush dst
            for(rg_offset=0; rg_offset<2*num_iter_dst; rg_offset++){
                void *dst_rank_addr_after_rns = dst_rank_addr_array[rg_offset];
                COMM_FLUSH_a2a(1, dst_rank_addr_after_rns);
            }
            src_mram_offset+=8;
            dst_mram_offset+=8;
        }
    }
    _mm_mfence();
    return;
}