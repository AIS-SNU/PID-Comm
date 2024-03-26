/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_API_MEMORY_H
#define DPU_API_MEMORY_H

#include <dpu_types.h>
#include <dpu_error.h>

dpu_error_t
dpu_copy_to_symbol_rank(struct dpu_rank_t *rank,
    struct dpu_symbol_t symbol,
    uint32_t symbol_offset,
    const void *src,
    size_t length);
dpu_error_t
dpu_copy_to_symbol_matrix(struct dpu_rank_t *rank, struct dpu_symbol_t symbol, uint32_t symbol_offset, size_t length);
dpu_error_t
dpu_copy_from_symbol_matrix(struct dpu_rank_t *rank, struct dpu_symbol_t symbol, uint32_t symbol_offset, size_t length);

dpu_error_t
dpu_set_transfer_buffer_safe(struct dpu_t *dpu, void *buffer);

struct dpu_transfer_matrix *
dpu_get_transfer_matrix(struct dpu_rank_t *rank);

#endif /* DPU_API_MEMORY_H */
