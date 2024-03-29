/* Copyright 2024 AISys. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _COMMON_H_
#define _COMMON_H_

// Data type
#ifdef UINT32
#define T uint32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT32
#define T int32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT16
#define T int16_t
#define byte_dt 2
#define DIV 1 // Shift right to divide by sizeof(T)
#elif INT8
#define T int8_t
#define byte_dt 1
#define DIV 0 // Shift right to divide by sizeof(T)
#elif INT64
#define T int64_t
#define byte_dt 8
#define DIV 3 // Shift right to divide by sizeof(T)
#endif

/* Structures used by both the host and the dpu to communicate information */
typedef struct {
    uint32_t start_offset;
    uint32_t target_offset;
    uint32_t total_data_size;
    uint32_t num_comm_dpu;
    uint32_t each_dpu;
    bool no_rotate;
    uint32_t num_row;
    uint32_t comm_type;
    uint32_t a_length;
    uint32_t num_comm_rg;
} dpu_arguments_comm_t;


#endif