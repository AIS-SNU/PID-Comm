/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>

#include "dpu_region_address_translation.h"

#define NB_ELEM_MATRIX 8
#define NB_WRQ_FIFO_ENTRIES 17
#define WRQ_FIFO_ENTRY_SIZE 128 // TODO check that, cache line is 128B (?!)

#define CIID 5

#define for_each_dpu_in_rank(idx, ci, dpu, nb_cis, nb_dpus_per_ci)                                                               \
    for (dpu = 0, idx = 0; dpu < nb_dpus_per_ci; ++dpu)                                                                          \
        for (ci = 0; ci < nb_cis; ++ci, ++idx)

void
byte_interleave(uint64_t *input, uint64_t *output)
{
    int i, j;

    for (i = 0; i < NB_ELEM_MATRIX; ++i)
        for (j = 0; j < (int)sizeof(uint64_t); ++j)
            ((uint8_t *)&output[i])[j] = ((uint8_t *)&input[j])[i];
}

/* Write NB_WRQ_FIFO_ENTRIES of 0 right after the CI */
// TODO check with Fabrice that it is not a problem to write 0 right
// after a command: will the first command be correctly sampled ?
void
flush_mc_fifo(void *base_ci_address)
{
    uint64_t *next_ci_address = (uint64_t *)base_ci_address + 0x800;

    for (int i = 0; i < NB_WRQ_FIFO_ENTRIES; ++i) {
        __asm__ __volatile__("isync; sync; dcbz %y0; sync; isync" : : "Z"(*(uint8_t *)next_ci_address) : "memory");
        for (int j = 0; j < (int)(WRQ_FIFO_ENTRY_SIZE / sizeof(uint64_t)); ++j)
            next_ci_address[j] = 0xdeadbeefULL;
        __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync; eieio;" : : "Z"(*(uint8_t *)next_ci_address) : "memory");

        next_ci_address += 0x4000 / sizeof(uint64_t);
    }
}

#define WRAP_OFFSET_CI 8

void
power9_read_from_cis(__attribute__((unused)) struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    void *block_data,
    __attribute__((unused)) uint32_t block_size)
{
    uint64_t tmp[10][16];
    uint64_t input[NB_ELEM_MATRIX * 2];
    uint64_t *ci_address;
    int i;

    ci_address = base_region_addr + 0x80; // + 0x4000;

    for (i = 0; i < 10; ++i) {
        input[0] = *((volatile uint64_t *)ci_address + 0);
        __asm__ __volatile__("isync; sync; eieio;" : : : "memory");

        input[1] = *((volatile uint64_t *)ci_address + 1);
        input[2] = *((volatile uint64_t *)ci_address + 2);
        input[3] = *((volatile uint64_t *)ci_address + 3);
        input[4] = *((volatile uint64_t *)ci_address + 4);
        input[5] = *((volatile uint64_t *)ci_address + 5);
        input[6] = *((volatile uint64_t *)ci_address + 6);
        input[7] = *((volatile uint64_t *)ci_address + 7);

        input[8] = *((volatile uint64_t *)ci_address + 8);
        input[9] = *((volatile uint64_t *)ci_address + 9);
        input[10] = *((volatile uint64_t *)ci_address + 10);
        input[11] = *((volatile uint64_t *)ci_address + 11);
        input[12] = *((volatile uint64_t *)ci_address + 12);
        input[13] = *((volatile uint64_t *)ci_address + 13);
        input[14] = *((volatile uint64_t *)ci_address + 14);
        input[15] = *((volatile uint64_t *)ci_address + 15);

        __asm__ __volatile__("isync; sync" : : : "memory");

        byte_interleave(input, tmp[i]);
        byte_interleave(&input[8], &tmp[i][8]);

        __asm__ __volatile__("isync; sync; dcbz %y0; sync; isync" : : "Z"(*(uint8_t *)ci_address) : "memory");
        __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync" : : "Z"(*(uint8_t *)ci_address) : "memory");
    }

    // printf("result: %016llx %016llx %016llx %016llx\n", (unsigned long long)tmp[0][CIID], (unsigned long long)tmp[1][CIID],
    // (unsigned long long)tmp[2][CIID], (unsigned long long)tmp[3][CIID]);

    memcpy(block_data, tmp[3], 64);
}

void
power9_write_to_cis(__attribute__((unused)) struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    void *block_data,
    __attribute__((unused)) uint32_t block_size)
{
    uint64_t output[2 * NB_ELEM_MATRIX];
    uint64_t *ci_address, command_msbs = 0;
    int i;
    uint8_t nb_cis;

    nb_cis = tr->interleave->nb_ci;

    /* 0/ Find out CI address */
    ci_address = base_region_addr + 0x80; // + 0x4000;

    /* 1/ Byte interleave the command */
    byte_interleave(block_data, output);

    __asm__ __volatile__("isync; sync; dcbz %y0; sync; isync" : : "Z"(*(uint8_t *)ci_address) : "memory");

    // printf("command magic: %llx\n", (unsigned long long)((uint64_t *)block_data)[CIID]);

    /* Nopify only commands != 0 */
    for (i = 0; i < nb_cis; ++i) {
        uint64_t command = ((uint64_t *)block_data)[i];
        if ((command >> 56) & 0xFF)
            command_msbs |= (0xFFULL << (i * 8));
    }

    *((volatile uint64_t *)ci_address + 7) = command_msbs;
    *((volatile uint64_t *)ci_address + 15) = command_msbs;
    __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync" : : "Z"(*(uint8_t *)ci_address) : "memory");

    *((volatile uint64_t *)ci_address + 0) = output[0];
    *((volatile uint64_t *)ci_address + 1) = output[1];
    *((volatile uint64_t *)ci_address + 2) = output[2];
    *((volatile uint64_t *)ci_address + 3) = output[3];
    *((volatile uint64_t *)ci_address + 4) = output[4];
    *((volatile uint64_t *)ci_address + 5) = output[5];
    *((volatile uint64_t *)ci_address + 6) = output[6];

    *((volatile uint64_t *)ci_address + 8) = output[0];
    *((volatile uint64_t *)ci_address + 9) = output[1];
    *((volatile uint64_t *)ci_address + 10) = output[2];
    *((volatile uint64_t *)ci_address + 11) = output[3];
    *((volatile uint64_t *)ci_address + 12) = output[4];
    *((volatile uint64_t *)ci_address + 13) = output[5];
    *((volatile uint64_t *)ci_address + 14) = output[6];

    __asm__ __volatile__("isync; sync" : : : "memory");

    /* 3/ Flush the MC fifo */
    // flush_mc_fifo(base_region_addr);

    *((volatile uint64_t *)ci_address + 7) = output[7];
    *((volatile uint64_t *)ci_address + 15) = 0ULL;
    __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync" : : "Z"(*(uint8_t *)ci_address) : "memory");
}

#define BANK_START(bank_start, dpu_id)                                                                                           \
    switch (dpu_id) {                                                                                                            \
        case 0:                                                                                                                  \
            bank_start = 0;                                                                                                      \
            break;                                                                                                               \
        case 1:                                                                                                                  \
            bank_start = 0x400;                                                                                                  \
            break;                                                                                                               \
        case 2:                                                                                                                  \
            bank_start = 0x200;                                                                                                  \
            break;                                                                                                               \
        case 3:                                                                                                                  \
            bank_start = 0x600;                                                                                                  \
            break;                                                                                                               \
        case 4:                                                                                                                  \
            bank_start = 0x100;                                                                                                  \
            break;                                                                                                               \
        case 5:                                                                                                                  \
            bank_start = 0x500;                                                                                                  \
            break;                                                                                                               \
        case 6:                                                                                                                  \
            bank_start = 0x300;                                                                                                  \
            break;                                                                                                               \
        case 7:                                                                                                                  \
            bank_start = 0x700;                                                                                                  \
            break;                                                                                                               \
    }

#define BANK_OFFSET_NEXT_DATA(i) ((0x80 / 2) * (i))
#define BANK_CHUNK_SIZE 0x80
#define BANK_NEXT_CHUNK_OFFSET 0x800

void
power9_write_to_rank(struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    struct dpu_transfer_matrix *xfer_matrix)
{
    uint64_t cache_line[16], cache_line_interleave[16];
    uint8_t idx, ci_id, dpu_id, nb_cis, nb_dpus_per_ci;

    nb_cis = tr->interleave->nb_ci;
    nb_dpus_per_ci = tr->interleave->nb_dpus_per_ci;

    /* Works only for transfers of same size and same offset on the
     * same line
     */
    for (dpu_id = 0, idx = 0; dpu_id < nb_dpus_per_ci; ++dpu_id, idx += 8) {
        uint8_t *ptr_dest = (uint8_t *)base_region_addr;
        uint32_t bank_start;
        uint32_t size_transfer = 0, i;
        uint32_t offset = 0;

        BANK_START(bank_start, dpu_id);
        ptr_dest += bank_start;

        for (ci_id = 0; ci_id < nb_cis; ++ci_id) {
            if (xfer_matrix[idx + ci_id].ptr) {
                size_transfer = xfer_matrix[idx + ci_id].size;
                offset = xfer_matrix[idx + ci_id].offset;
                break;
            }
        }

        if (!size_transfer)
            continue;

        for (i = 0; i < size_transfer / sizeof(uint64_t); i += 2) {
            uint64_t next_data = BANK_OFFSET_NEXT_DATA(i + offset / 8);
            uint64_t offset = (next_data % BANK_CHUNK_SIZE) + (next_data / BANK_CHUNK_SIZE) * BANK_NEXT_CHUNK_OFFSET;

            for (ci_id = 0; ci_id < nb_cis * 2; ++ci_id) {
                if (xfer_matrix[idx + ci_id % nb_cis].ptr)
                    cache_line[ci_id] = *((uint64_t *)xfer_matrix[idx + ci_id % nb_cis].ptr + i + ci_id / nb_cis);
            }

            byte_interleave(cache_line, cache_line_interleave);
            byte_interleave(&cache_line[8], &cache_line_interleave[8]);

            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 0 * sizeof(uint64_t))) = cache_line_interleave[0];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 1 * sizeof(uint64_t))) = cache_line_interleave[1];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 2 * sizeof(uint64_t))) = cache_line_interleave[2];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 3 * sizeof(uint64_t))) = cache_line_interleave[3];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 4 * sizeof(uint64_t))) = cache_line_interleave[4];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 5 * sizeof(uint64_t))) = cache_line_interleave[5];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 6 * sizeof(uint64_t))) = cache_line_interleave[6];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 7 * sizeof(uint64_t))) = cache_line_interleave[7];

            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 8 * sizeof(uint64_t))) = cache_line_interleave[8];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 9 * sizeof(uint64_t))) = cache_line_interleave[9];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 10 * sizeof(uint64_t))) = cache_line_interleave[10];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 11 * sizeof(uint64_t))) = cache_line_interleave[11];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 12 * sizeof(uint64_t))) = cache_line_interleave[12];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 13 * sizeof(uint64_t))) = cache_line_interleave[13];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 14 * sizeof(uint64_t))) = cache_line_interleave[14];
            *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 15 * sizeof(uint64_t))) = cache_line_interleave[15];

            __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync; eieio;"
                                 :
                                 : "Z"(*((uint8_t *)ptr_dest + offset))
                                 : "memory");
        }
    }
}

void
power9_read_from_rank(struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    struct dpu_transfer_matrix *xfer_matrix)
{
    uint64_t cache_line[16], cache_line_interleave[16];
    uint8_t idx, ci_id, dpu_id, nb_cis, nb_dpus_per_ci;

    nb_cis = tr->interleave->nb_ci;
    nb_dpus_per_ci = tr->interleave->nb_dpus_per_ci;

    /* Works only for transfers of same size and same offset on the
     * same line
     */
    for (dpu_id = 0, idx = 0; dpu_id < nb_dpus_per_ci; ++dpu_id, idx += 8) {
        uint32_t bank_start;
        uint8_t *ptr_dest = (uint8_t *)base_region_addr;
        uint32_t size_transfer = 0, i;
        uint32_t offset = 0;

        BANK_START(bank_start, dpu_id);
        ptr_dest += bank_start;

        for (ci_id = 0; ci_id < nb_cis; ++ci_id) {
            if (xfer_matrix[idx + ci_id].ptr) {
                size_transfer = xfer_matrix[idx + ci_id].size;
                offset = xfer_matrix[idx + ci_id].offset;
                break;
            }
        }

        if (!size_transfer)
            continue;

        for (i = 0; i < size_transfer / sizeof(uint64_t); i += 2) {
            uint64_t next_data = BANK_OFFSET_NEXT_DATA(i + offset / 8);
            uint64_t offset = (next_data % BANK_CHUNK_SIZE) + (next_data / BANK_CHUNK_SIZE) * BANK_NEXT_CHUNK_OFFSET;

            /* Invalidates possible prefetched cache line or old cache line */
            __asm__ __volatile__("isync; sync; dcbf %y0; sync; isync" : : "Z"(*(uint8_t *)ptr_dest + offset) : "memory");

            cache_line[0] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 0 * sizeof(uint64_t)));
            cache_line[1] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 1 * sizeof(uint64_t)));
            cache_line[2] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 2 * sizeof(uint64_t)));
            cache_line[3] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 3 * sizeof(uint64_t)));
            cache_line[4] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 4 * sizeof(uint64_t)));
            cache_line[5] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 5 * sizeof(uint64_t)));
            cache_line[6] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 6 * sizeof(uint64_t)));
            cache_line[7] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 7 * sizeof(uint64_t)));

            cache_line[8] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 8 * sizeof(uint64_t)));
            cache_line[9] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 9 * sizeof(uint64_t)));
            cache_line[10] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 10 * sizeof(uint64_t)));
            cache_line[11] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 11 * sizeof(uint64_t)));
            cache_line[12] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 12 * sizeof(uint64_t)));
            cache_line[13] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 13 * sizeof(uint64_t)));
            cache_line[14] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 14 * sizeof(uint64_t)));
            cache_line[15] = *((volatile uint64_t *)((uint8_t *)ptr_dest + offset + 15 * sizeof(uint64_t)));

            byte_interleave(cache_line, cache_line_interleave);
            byte_interleave(&cache_line[8], &cache_line_interleave[8]);

            for (ci_id = 0; ci_id < 2 * nb_cis; ++ci_id) {
                if (xfer_matrix[idx + ci_id % nb_cis].ptr) {
                    *((uint64_t *)xfer_matrix[idx + ci_id % nb_cis].ptr + i + ci_id / nb_cis) = cache_line_interleave[ci_id];
                }
            }
        }
    }

    __asm__ __volatile__("isync; sync; eieio;" : : : "memory");
}

struct dpu_region_interleaving power9_interleave = {
    .nb_ci = 8,
    .nb_dpus_per_ci = 8,
    .mram_size = 64 * 1024 * 1024,
};

struct dpu_region_address_translation power9_translate = {
    .interleave = &power9_interleave,
    .backend_id = DPU_BACKEND_POWER9,
    .capabilities = CAP_PERF | CAP_SAFE,
    //.init_rank	        = power9_init_rank,
    //.destroy_rank         = power9_destroy_rank,
    .write_to_rank = power9_write_to_rank,
    .read_from_rank = power9_read_from_rank,
    .write_to_cis = power9_write_to_cis,
    .read_from_cis = power9_read_from_cis,
};
