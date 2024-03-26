/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_TRANSFER_MATRIX_H
#define DPU_TRANSFER_MATRIX_H

#include <stdbool.h>
#include <stdint.h>

#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_transfer_matrix.h
 * @brief C API for DPU transfer matrix operations.
 */

/**
 * @brief Maximum number of DPUs in a single DPU rank.
 * @hideinitializer
 */
#define MAX_NR_DPUS_PER_RANK 64

/**
 * @brief Internal definition used when defining the dpu_transfer_matrix structure.
 * @private
 * @hideinitializer
 */
#define struct_dpu_transfer_matrix_t

/**
 * @brief Context of a DPU memory transfer.
 */
struct dpu_transfer_matrix {
    /** Host buffers for the transfer, used as source or destination, depending on the transfer direction. */
    void *ptr[MAX_NR_DPUS_PER_RANK];
    /** Memory offset in bytes for the transfer. */
    uint32_t offset;
    /** Transfer size in bytes. */
    uint32_t size;
};

/**
 * @brief Add the specified DPU to the given memory transfer matrix.
 * @param dpu the DPU
 * @param transfer_matrix the memory transfer matrix
 * @param buffer the host buffer associated to the DPU in the transfer
 */
void
dpu_transfer_matrix_add_dpu(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix, void *buffer);

/**
 * @brief Associate the specified host buffer to all DPUs of the DPU rank for the given memory transfer matrix.
 * @param rank the DPU rank
 * @param transfer_matrix the memory transfer matrix
 * @param buffer the host buffer associated to the DPUs in the transfer
 */
void
dpu_transfer_matrix_set_all(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix, void *buffer);

/**
 * @brief Remove the specified DPU to the given memory transfer matrix.
 * @param dpu the DPU
 * @param transfer_matrix the memory transfer matrix
 */
void
dpu_transfer_matrix_clear_dpu(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix);

/**
 * @brief Remove all DPUs of the DPU rank from the given memory transfer matrix.
 * @param rank the DPU rank
 * @param transfer_matrix the memory transfer matrix
 */
void
dpu_transfer_matrix_clear_all(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);

/**
 * @brief Copy the memory transfer matrix.
 * @param dst the destination matrix
 * @param src the source matrix.
 */
void
dpu_transfer_matrix_copy(struct dpu_transfer_matrix *dst, struct dpu_transfer_matrix *src);

/**
 * @brief Get the host buffer for the specified DPU in the given memory transfer matrix.
 * @param dpu the DPU
 * @param transfer_matrix the memory transfer matrix
 * @return The host buffer associated to the DPU.
 */
void *
dpu_transfer_matrix_get_ptr(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix);

#endif // DPU_TRANSFER_MATRIX_H
