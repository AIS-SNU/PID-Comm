/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <string.h>

#include <dpu_error.h>
#include <verbose_control.h>
#include <dpu_types.h>
#include <dpu_attributes.h>
#include <dpu_instruction_encoder.h>
#include <dpu_predef_programs.h>
#include <dpu_management.h>
#include <dpu_memory.h>
#include <dpu_config.h>
#include <dpu_debug.h>
#include <dpu_runner.h>

#include <dpu_rank.h>
#include <dpu_internals.h>
#include <dpu_api_log.h>
#include <dpu_mask.h>

static dpu_error_t
copy_from_mrams_using_dpu_program(struct dpu_rank_t *rank, const struct dpu_transfer_matrix *transfer_matrix);
static dpu_error_t
copy_to_mrams_using_dpu_program(struct dpu_rank_t *rank, const struct dpu_transfer_matrix *transfer_matrix);
static dpu_error_t
access_mram_using_dpu_program_individual(struct dpu_t *dpu,
    const struct dpu_transfer_matrix *transfer,
    dpu_transfer_type_t transfer_type,
    dpuinstruction_t *program,
    dpuinstruction_t *iram_save,
    iram_size_t nr_instructions,
    dpuword_t *wram_save,
    wram_size_t wram_size);

static inline uint32_t
_transfer_matrix_index(struct dpu_t *dpu)
{
    return get_transfer_matrix_index(dpu->rank, dpu->slice_id, dpu->dpu_id);
}

__API_SYMBOL__ dpu_error_t
dpu_copy_to_iram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
    uint32_t iram_instruction_index = matrix->offset;
    uint32_t nb_of_instructions = matrix->size;
    LOG_RANK(DEBUG, rank, "%p, %u, %u", matrix, iram_instruction_index, nb_of_instructions);

    verify_iram_access(iram_instruction_index, nb_of_instructions, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_irams_matrix)(rank, matrix);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_copy_from_iram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
    uint32_t iram_instruction_index = matrix->offset;
    uint32_t nb_of_instructions = matrix->size;
    LOG_RANK(DEBUG, rank, "%u, %u", iram_instruction_index, nb_of_instructions);

    verify_iram_access(iram_instruction_index, nb_of_instructions, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_from_irams_matrix)(rank, matrix);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_iram_for_rank(struct dpu_rank_t *rank,
    iram_addr_t iram_instruction_index,
    const dpuinstruction_t *source,
    iram_size_t nb_of_instructions)
{
    LOG_RANK(DEBUG, rank, "%u, %u", iram_instruction_index, nb_of_instructions);

    verify_iram_access(iram_instruction_index, nb_of_instructions, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_irams_rank)(rank, iram_instruction_index, source, nb_of_instructions);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_iram_for_dpu(struct dpu_t *dpu,
    iram_addr_t iram_instruction_index,
    const dpuinstruction_t *source,
    iram_size_t nb_of_instructions)
{
    LOG_DPU(DEBUG, dpu, "%u, %u", iram_instruction_index, nb_of_instructions);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    verify_iram_access(iram_instruction_index, nb_of_instructions, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_irams_dpu)(dpu, iram_instruction_index, source, nb_of_instructions);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_from_iram_for_dpu(struct dpu_t *dpu,
    dpuinstruction_t *destination,
    iram_addr_t iram_instruction_index,
    iram_size_t nb_of_instructions)
{
    LOG_DPU(DEBUG, dpu, "%u, %u", iram_instruction_index, nb_of_instructions);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    verify_iram_access(iram_instruction_index, nb_of_instructions, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_from_irams_dpu)(dpu, destination, iram_instruction_index, nb_of_instructions);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_wram_for_rank(struct dpu_rank_t *rank, wram_addr_t wram_word_offset, const dpuword_t *source, wram_size_t nb_of_words)
{
    LOG_RANK(DEBUG, rank, "%u, %u", wram_word_offset, nb_of_words);

    verify_wram_access(wram_word_offset, nb_of_words, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_wrams_rank)(rank, wram_word_offset, source, nb_of_words);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_wram_for_dpu(struct dpu_t *dpu, wram_addr_t wram_word_offset, const dpuword_t *source, wram_size_t nb_of_words)
{
    LOG_DPU(DEBUG, dpu, "%u, %u", wram_word_offset, nb_of_words);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    verify_wram_access(wram_word_offset, nb_of_words, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_wrams_dpu)(dpu, wram_word_offset, source, nb_of_words);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_wram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    uint32_t wram_word_offset = transfer_matrix->offset;
    uint32_t nb_of_words = transfer_matrix->size;
    LOG_RANK(DEBUG, rank, "%p, %u, %u", transfer_matrix, wram_word_offset, nb_of_words);

    verify_wram_access(wram_word_offset, nb_of_words, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_to_wrams_matrix)(rank, transfer_matrix);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_from_wram_for_dpu(struct dpu_t *dpu, dpuword_t *destination, wram_addr_t wram_word_offset, wram_size_t nb_of_words)
{
    LOG_DPU(DEBUG, dpu, "%u, %u", wram_word_offset, nb_of_words);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    verify_wram_access(wram_word_offset, nb_of_words, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_from_wrams_dpu)(dpu, destination, wram_word_offset, nb_of_words);
    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_from_wram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    uint32_t wram_word_offset = transfer_matrix->offset;
    uint32_t nb_of_words = transfer_matrix->size;
    LOG_RANK(DEBUG, rank, "%u, %u", wram_word_offset, nb_of_words);

    verify_wram_access(wram_word_offset, nb_of_words, rank);

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, copy_from_wrams_matrix)(rank, transfer_matrix);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_copy_to_mram(struct dpu_t *dpu, mram_addr_t mram_byte_offset, const uint8_t *source, mram_size_t nb_of_bytes)
{
    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    if (!nb_of_bytes) {
        return DPU_OK;
    }

    dpu_error_t status;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    struct dpu_transfer_matrix transfer_matrix;
    dpu_transfer_matrix_clear_all(rank, &transfer_matrix);

    dpu_transfer_matrix_add_dpu(dpu, &transfer_matrix, (void *)source);

    transfer_matrix.size = nb_of_bytes;
    transfer_matrix.offset = mram_byte_offset;
    status = dpu_copy_to_mrams(rank, &transfer_matrix);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_copy_from_mram(struct dpu_t *dpu, uint8_t *destination, mram_addr_t mram_byte_offset, mram_size_t nb_of_bytes)
{
    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    if (!nb_of_bytes) {
        return DPU_OK;
    }

    dpu_error_t status;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    struct dpu_transfer_matrix transfer_matrix;
    dpu_transfer_matrix_clear_all(rank, &transfer_matrix);

    dpu_transfer_matrix_add_dpu(dpu, &transfer_matrix, (void *)destination);

    transfer_matrix.size = nb_of_bytes;
    transfer_matrix.offset = mram_byte_offset;
    status = dpu_copy_from_mrams(rank, &transfer_matrix);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_mrams_RNS(
    struct dpu_rank_t *rank_src, 
    struct dpu_rank_t *rank_dst, 
    struct dpu_transfer_matrix *transfer_matrix_src, 
    struct dpu_transfer_matrix *transfer_matrix_dst)
{
    dpu_error_t status;

    uint32_t offset_src = transfer_matrix_src->offset;
    uint32_t offset_dst = transfer_matrix_dst->offset;
    uint32_t size = transfer_matrix_src->size;

    verify_mram_access_offset_and_size(offset_src, size, rank_src);
    verify_mram_access_offset_and_size(offset_dst, size, rank_dst);

    dpu_lock_rank(rank_src);
    dpu_lock_rank(rank_dst);

    if (rank_src->runtime.run_context.nb_dpu_running > 0) 
    {
        LOG_RANK(WARNING,
            rank_src,
            "Host does not have access to the MRAM because %u DPU%s running.",
            rank_src->runtime.run_context.nb_dpu_running,
            rank_src->runtime.run_context.nb_dpu_running > 1 ? "s are" : " is");
        dpu_unlock_rank(rank_src);
        dpu_unlock_rank(rank_dst);
        return DPU_ERR_MRAM_BUSY;
    }
    if (rank_dst->runtime.run_context.nb_dpu_running > 0) 
    {
        LOG_RANK(WARNING,
            rank_dst,
            "Host does not have access to the MRAM because %u DPU%s running.",
            rank_dst->runtime.run_context.nb_dpu_running,
            rank_dst->runtime.run_context.nb_dpu_running > 1 ? "s are" : " is");
        dpu_unlock_rank(rank_src);
        dpu_unlock_rank(rank_dst);
        return DPU_ERR_MRAM_BUSY;
    }

    {
        status = rank_src->handler_context->handler->copy_rank_RNS(rank_src, rank_dst, transfer_matrix_src, transfer_matrix_dst);
        // status = RANK_FEATURE(rank, copy_to_mrams)(rank, transfer_matrix);
    }

    dpu_unlock_rank(rank_src);
    dpu_unlock_rank(rank_dst);

    return status;
}


__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_to_mrams(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    // printf("\t\tdpu_copy_to_mrams Called\n");
    LOG_RANK(VERBOSE, rank, "%p", transfer_matrix);
    dpu_error_t status;

    uint32_t offset = transfer_matrix->offset;
    uint32_t size = transfer_matrix->size;

    verify_mram_access_offset_and_size(offset, size, rank);

    dpu_lock_rank(rank);

    if (rank->runtime.run_context.nb_dpu_running > 0) {
        LOG_RANK(WARNING,
            rank,
            "Host does not have access to the MRAM because %u DPU%s running.",
            rank->runtime.run_context.nb_dpu_running,
            rank->runtime.run_context.nb_dpu_running > 1 ? "s are" : " is");
        dpu_unlock_rank(rank);
        return DPU_ERR_MRAM_BUSY;
    }

    if (rank->description->configuration.mram_access_by_dpu_only) 
    {
        status = copy_to_mrams_using_dpu_program(rank, transfer_matrix);
    } else 
    {
        // status = rank->handler_context->handler->copy_to_rank(rank, transfer_matrix);
        status = RANK_FEATURE(rank, copy_to_mrams)(rank, transfer_matrix);
    }

    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ dpu_error_t
dpu_copy_from_mrams(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    LOG_RANK(VERBOSE, rank, "%p", transfer_matrix);
    dpu_error_t status;

    uint32_t offset = transfer_matrix->offset;
    uint32_t size = transfer_matrix->size;

    verify_mram_access_offset_and_size(offset, size, rank);

    dpu_lock_rank(rank);

    if (rank->runtime.run_context.nb_dpu_running > 0) {
        LOG_RANK(WARNING,
            rank,
            "Host does not have access to the MRAM because %u DPU%s running.",
            rank->runtime.run_context.nb_dpu_running,
            rank->runtime.run_context.nb_dpu_running > 1 ? "s are" : " is");
        dpu_unlock_rank(rank);
        return DPU_ERR_MRAM_BUSY;
    }

    if (rank->description->configuration.mram_access_by_dpu_only) {
        status = copy_from_mrams_using_dpu_program(rank, transfer_matrix);
    } else {
        status = RANK_FEATURE(rank, copy_from_mrams)(rank, transfer_matrix);
    }

    dpu_unlock_rank(rank);

    return status;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
dpu_transfer_matrix_add_dpu(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix, void *buffer)
{
    LOG_DPU(DEBUG, dpu, "%p, %p", transfer_matrix, buffer);

    transfer_matrix->ptr[_transfer_matrix_index(dpu)] = buffer;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
dpu_transfer_matrix_set_all(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix, void *buffer)
{
    LOG_RANK(DEBUG, rank, "%p, %p", transfer_matrix, buffer);
    uint8_t nr_of_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;
    uint8_t nr_of_cis = rank->description->hw.topology.nr_of_control_interfaces;

    for (dpu_slice_id_t each_ci = 0; each_ci < nr_of_cis; ++each_ci) {
        for (dpu_member_id_t each_dpu = 0; each_dpu < nr_of_dpus_per_ci; ++each_dpu) {
            struct dpu_t *dpu = DPU_GET_UNSAFE(rank, each_ci, each_dpu);

            dpu_transfer_matrix_add_dpu(dpu, transfer_matrix, dpu->enabled ? buffer : NULL);
        }
    }
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
dpu_transfer_matrix_clear_dpu(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix)
{
    LOG_DPU(DEBUG, dpu, "%p", __func__, dpu_get_id(dpu), transfer_matrix);

    transfer_matrix->ptr[_transfer_matrix_index(dpu)] = NULL;
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
dpu_transfer_matrix_clear_all(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    LOG_RANK(DEBUG, rank, "%p", transfer_matrix);

    memset(transfer_matrix, 0, sizeof(struct dpu_transfer_matrix));
}

__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
dpu_transfer_matrix_copy(struct dpu_transfer_matrix *dst, struct dpu_transfer_matrix *src)
{
    LOG_FN(DEBUG, "%p %p", dst, src);

    memcpy(dst, src, sizeof(struct dpu_transfer_matrix));
}

__API_SYMBOL__ void *
dpu_transfer_matrix_get_ptr(struct dpu_t *dpu, struct dpu_transfer_matrix *transfer_matrix)
{
    LOG_DPU(DEBUG, dpu, "%p", transfer_matrix);

    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");

        return NULL;
    }

    uint32_t dpu_index = _transfer_matrix_index(dpu);

    return transfer_matrix->ptr[dpu_index];
}

static dpu_error_t
copy_from_mrams_using_dpu_program(struct dpu_rank_t *rank, const struct dpu_transfer_matrix *transfer_matrix)
{
    LOG_RANK(VERBOSE, rank, "%p", transfer_matrix);
    uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;
    int idx = 0;
    dpu_error_t status = DPU_OK;
    bool arrays_initialized = false;
    dpuinstruction_t *program = NULL;
    dpuinstruction_t *iram_save = NULL;
    dpuword_t *wram_save = NULL;
    iram_size_t nr_saved_instructions = 0;
    wram_size_t wram_size_in_words = 0;
    dpuword_t *mram_buffer = NULL;

    for (dpu_member_id_t dpu_id = 0; dpu_id < nr_dpus_per_ci; ++dpu_id) {
        for (dpu_slice_id_t slice_id = 0; slice_id < nr_cis; ++slice_id, ++idx) {
            void *ptr = transfer_matrix->ptr[idx];
            const struct dpu_transfer_matrix *actual_transfer;
            struct dpu_transfer_matrix aligned_transfer;
            bool copy_needed;
            dpu_transfer_matrix_clear_all(rank, &aligned_transfer);

            if (ptr == NULL) {
                continue;
            }

            struct dpu_t *dpu = DPU_GET_UNSAFE(rank, slice_id, dpu_id);

            if (!dpu->enabled) {
                continue;
            }

            if (!arrays_initialized) {
                program = fetch_mram_access_program(&nr_saved_instructions);

                if (program == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto free_buffer;
                }

                uint32_t nr_saved_iram_bytes = nr_saved_instructions * sizeof(dpuinstruction_t);

                if ((iram_save = malloc(nr_saved_iram_bytes)) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    free(program);
                    goto free_buffer;
                }

                wram_size_in_words = rank->description->hw.memories.wram_size;

                if ((wram_save = malloc(wram_size_in_words * sizeof(*wram_save))) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    free(iram_save);
                    free(program);
                    goto free_buffer;
                }

                arrays_initialized = true;
            }

            if (((((unsigned long)ptr) & 3l) != 0) || ((transfer_matrix->offset & 7) != 0)
                || ((transfer_matrix->size & 7) != 0)) {
                uint32_t aligned_size = (transfer_matrix->size + (transfer_matrix->offset & 7) + 7) & ~7;

                if ((mram_buffer = realloc(mram_buffer, aligned_size)) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto free_arrays;
                }

                aligned_transfer.size = aligned_size;
                aligned_transfer.offset = transfer_matrix->offset & ~7;
                aligned_transfer.ptr[0] = mram_buffer;
                actual_transfer = &aligned_transfer;
                copy_needed = true;
            } else {
                actual_transfer = transfer_matrix;
                copy_needed = false;
            }

            if ((status = access_mram_using_dpu_program_individual(dpu,
                     actual_transfer,
                     DPU_TRANSFER_FROM_MRAM,
                     program,
                     iram_save,
                     nr_saved_instructions,
                     wram_save,
                     wram_size_in_words))
                != DPU_OK) {
                goto free_arrays;
            }

            if (copy_needed) {
                memcpy(ptr, ((uint8_t *)mram_buffer) + (transfer_matrix->offset & 7), transfer_matrix->size);
            }
        }
    }

free_arrays:
    if (arrays_initialized) {
        free(program);
        free(iram_save);
        free(wram_save);
    }
free_buffer:
    if (mram_buffer != NULL) {
        free(mram_buffer);
    }
    return status;
}

static dpu_error_t
copy_to_mrams_using_dpu_program(struct dpu_rank_t *rank, const struct dpu_transfer_matrix *transfer_matrix)
{
    uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;
    int idx = 0;
    dpu_error_t status = DPU_OK;
    bool arrays_initialized = false;
    dpuinstruction_t *program = NULL;
    dpuinstruction_t *iram_save = NULL;
    dpuword_t *wram_save = NULL;
    iram_size_t nr_saved_instructions = 0;
    wram_size_t wram_size_in_words = 0;
    dpuword_t *mram_buffer = NULL;

    for (dpu_member_id_t dpu_id = 0; dpu_id < nr_dpus_per_ci; ++dpu_id) {
        for (dpu_slice_id_t slice_id = 0; slice_id < nr_cis; ++slice_id, ++idx) {
            void *ptr = transfer_matrix->ptr[idx];
            const struct dpu_transfer_matrix *actual_transfer;
            struct dpu_transfer_matrix aligned_transfer;
            dpu_transfer_matrix_clear_all(rank, &aligned_transfer);

            if (ptr == NULL) {
                continue;
            }

            struct dpu_t *dpu = DPU_GET_UNSAFE(rank, slice_id, dpu_id);

            if (!dpu->enabled) {
                continue;
            }

            if (!arrays_initialized) {
                program = fetch_mram_access_program(&nr_saved_instructions);

                if (program == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto free_buffer;
                }

                uint32_t nr_saved_iram_bytes = nr_saved_instructions * sizeof(dpuinstruction_t);

                if ((iram_save = malloc(nr_saved_iram_bytes)) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    free(program);
                    goto free_buffer;
                }

                wram_size_in_words = rank->description->hw.memories.wram_size;

                if ((wram_save = malloc(wram_size_in_words * sizeof(*wram_save))) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    free(iram_save);
                    free(program);
                    goto free_buffer;
                }

                arrays_initialized = true;
            }

            if (((((unsigned long)ptr) & 3l) != 0) || ((transfer_matrix->offset & 7) != 0)
                || ((transfer_matrix->size & 7) != 0)) {
                uint32_t aligned_size = (transfer_matrix->size + (transfer_matrix->offset & 7) + 7) & ~7;

                if ((mram_buffer = realloc(mram_buffer, aligned_size)) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto free_arrays;
                }

                if ((transfer_matrix->offset & 7) != 0) {
                    aligned_transfer.offset = transfer_matrix->offset & ~7;
                    aligned_transfer.size = 8;
                    aligned_transfer.ptr[0] = mram_buffer;

                    if ((status = access_mram_using_dpu_program_individual(dpu,
                             &aligned_transfer,
                             DPU_TRANSFER_FROM_MRAM,
                             program,
                             iram_save,
                             nr_saved_instructions,
                             wram_save,
                             wram_size_in_words))
                        != DPU_OK) {
                        goto free_arrays;
                    }
                }

                if (((transfer_matrix->size + (transfer_matrix->offset & 7)) & 7) != 0) {
                    aligned_transfer.offset = transfer_matrix->offset + aligned_size - 8;
                    aligned_transfer.size = 8;
                    aligned_transfer.ptr[0] = ((uint8_t *)mram_buffer) + aligned_size - 8;

                    if ((status = access_mram_using_dpu_program_individual(dpu,
                             &aligned_transfer,
                             DPU_TRANSFER_FROM_MRAM,
                             program,
                             iram_save,
                             nr_saved_instructions,
                             wram_save,
                             wram_size_in_words))
                        != DPU_OK) {
                        goto free_arrays;
                    }
                }

                memcpy(((uint8_t *)mram_buffer) + (transfer_matrix->offset & 7), ptr, transfer_matrix->size);

                aligned_transfer.offset = transfer_matrix->offset & ~7;
                aligned_transfer.size = aligned_size;
                aligned_transfer.ptr[0] = mram_buffer;
                actual_transfer = &aligned_transfer;
            } else {
                actual_transfer = transfer_matrix;
            }

            if ((status = access_mram_using_dpu_program_individual(dpu,
                     actual_transfer,
                     DPU_TRANSFER_TO_MRAM,
                     program,
                     iram_save,
                     nr_saved_instructions,
                     wram_save,
                     wram_size_in_words))
                != DPU_OK) {
                goto free_arrays;
            }
        }
    }

free_arrays:
    if (arrays_initialized) {
        free(program);
        free(iram_save);
        free(wram_save);
    }
free_buffer:
    if (mram_buffer != NULL) {
        free(mram_buffer);
    }

    return status;
}

static dpu_error_t
access_mram_using_dpu_program_individual(struct dpu_t *dpu,
    const struct dpu_transfer_matrix *transfer,
    dpu_transfer_type_t transfer_type,
    dpuinstruction_t *program,
    dpuinstruction_t *iram_save,
    iram_size_t nr_instructions,
    dpuword_t *wram_save,
    wram_size_t wram_size)
{
    /*
     * Preconditions:
     *  - transfer->ptr[0]          must be aligned on 4 bytes
     *  - transfer->size            must be aligned on 8 bytes
     *  - transfer->offset          must be aligned on 8 bytes
     */

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

#define PROGRAM_CONTEXT_WRAM_SIZE 8
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    struct dpu_context_t context;
    bool context_init = false;

    uint32_t ptr_offset;

    dpu_lock_rank(rank);

    if (transfer->size == 0) {
        goto end;
    }

    FF(dpu_custom_for_dpu(dpu, DPU_COMMAND_EVENT_START, (dpu_custom_command_args_t)DPU_EVENT_MRAM_ACCESS_PROGRAM));

#define DMA_ACCESS_IDX_1 11
#define DMA_ACCESS_IDX_2 19
#define DMA_ACCESS_WRAM_REG REG_R2
#define DMA_ACCESS_MRAM_REG REG_R0
#define DMA_ACCESS_LENGTH_1 255
#define DMA_ACCESS_LENGTH_2 0

    // Patch DMA access instructions
    if (transfer_type == DPU_TRANSFER_TO_MRAM) {
        program[DMA_ACCESS_IDX_1] = SDMArri(DMA_ACCESS_WRAM_REG, DMA_ACCESS_MRAM_REG, DMA_ACCESS_LENGTH_1);
        program[DMA_ACCESS_IDX_2] = SDMArri(DMA_ACCESS_WRAM_REG, DMA_ACCESS_MRAM_REG, DMA_ACCESS_LENGTH_2);
    } else {
        program[DMA_ACCESS_IDX_1] = LDMArri(DMA_ACCESS_WRAM_REG, DMA_ACCESS_MRAM_REG, DMA_ACCESS_LENGTH_1);
        program[DMA_ACCESS_IDX_2] = LDMArri(DMA_ACCESS_WRAM_REG, DMA_ACCESS_MRAM_REG, DMA_ACCESS_LENGTH_2);
    }

    wram_size_t wram_save_size;

    if (((transfer->size / sizeof(dpuword_t)) + PROGRAM_CONTEXT_WRAM_SIZE) >= wram_size) {
        wram_save_size = wram_size;
    } else {
        wram_save_size = (transfer->size / sizeof(dpuword_t)) + PROGRAM_CONTEXT_WRAM_SIZE;
    }

    FF(dpu_context_fill_from_rank(&context, rank));
    context.nr_of_running_threads = 0;
    for (dpu_thread_t each_thread = 0; each_thread < rank->description->hw.dpu.nr_of_threads; ++each_thread) {
        context.scheduling[each_thread] = 0xff;
    }
    context_init = true;
    FF(dpu_initialize_fault_process_for_dpu(dpu, &context));
    FF(dpu_extract_pcs_for_dpu(dpu, &context));
    FF(dpu_extract_context_for_dpu(dpu, &context));

    // Saving IRAM & WRAM
    FF(dpu_copy_from_iram_for_dpu(dpu, iram_save, 0, nr_instructions));
    FF(dpu_copy_from_wram_for_dpu(dpu, wram_save, 0, wram_save_size));

    // Loading DPU program in IRAM
    FF(dpu_copy_to_iram_for_dpu(dpu, 0, program, nr_instructions));

    wram_size_t transfer_size = wram_save_size - PROGRAM_CONTEXT_WRAM_SIZE;
    wram_addr_t transfer_start = PROGRAM_CONTEXT_WRAM_SIZE;

    uint32_t transfer_size_in_bytes = transfer_size * sizeof(dpuword_t);
    uint32_t nr_complete_iterations = transfer->size / transfer_size_in_bytes;
    uint32_t remaining = transfer->size % transfer_size_in_bytes;

    // Write context
    dpuword_t program_context[3] = {
        transfer->offset, // MRAM offset
        transfer_size * sizeof(dpuword_t), // Buffer size
        ((nr_complete_iterations == 0) || ((nr_complete_iterations == 1) && (remaining == 0))) ? 1 : 0 // Last transfer marker
    };

    FF(dpu_copy_to_wram_for_dpu(dpu, 5, program_context, 3));
    if (transfer_type == DPU_TRANSFER_TO_MRAM) {
        // Loading WRAM
        FF(dpu_copy_to_wram_for_dpu(dpu, transfer_start, transfer->ptr[0], transfer_size));
    }

    // Program execution
    FF(dpu_thread_boot_safe_for_dpu(dpu, 0, NULL, false));

    // Polling
    bool run, fault;
    do {
        FF(dpu_poll_dpu(dpu, &run, &fault));
    } while (run && !fault);

    if (fault) {
        LOG_DPU(WARNING, dpu, "MRAM copy program has faulted");
        status = DPU_ERR_INTERNAL;
        goto end;
    }

    if (transfer_type == DPU_TRANSFER_FROM_MRAM) {
        // Fetching WRAM
        FF(dpu_copy_from_wram_for_dpu(dpu, transfer->ptr[0], transfer_start, transfer_size));
    }
    ptr_offset = transfer_size * sizeof(dpuword_t);

    for (uint32_t each_iteration = 1; each_iteration < nr_complete_iterations; ++each_iteration) {
        if (transfer_type == DPU_TRANSFER_TO_MRAM) {
            // Loading WRAM
            FF(dpu_copy_to_wram_for_dpu(dpu, transfer_start, transfer->ptr[0] + ptr_offset, transfer_size));
        }

        if ((each_iteration == (nr_complete_iterations - 1)) && (remaining == 0)) {
            dpuword_t last_transfer_marker = 1;
            FF(dpu_copy_to_wram_for_dpu(dpu, 7, &last_transfer_marker, 1));
        }

        FF(dpu_thread_boot_safe_for_dpu(dpu, 0, NULL, true));

        // Polling
        do {
            FF(dpu_poll_dpu(dpu, &run, &fault));
        } while (run && !fault);

        if (fault) {
            LOG_DPU(WARNING, dpu, "MRAM copy program has faulted");
            status = DPU_ERR_INTERNAL;
            goto end;
        }

        if (transfer_type == DPU_TRANSFER_FROM_MRAM) {
            // Fetching WRAM
            FF(dpu_copy_from_wram_for_dpu(dpu, transfer->ptr[0] + ptr_offset, transfer_start, transfer_size));
        }
        ptr_offset += transfer_size * sizeof(dpuword_t);
    }

    if (remaining > 0) {
        if (nr_complete_iterations > 0) {
            // Changing buffer size in the DPU program
            transfer_size = remaining / sizeof(dpuword_t);
            program_context[1] = remaining;
            program_context[2] = 1;

            FF(dpu_copy_to_wram_for_dpu(dpu, 6, program_context + 1, 2));

            if (transfer_type == DPU_TRANSFER_TO_MRAM) {
                // Loading WRAM
                FF(dpu_copy_to_wram_for_dpu(dpu, transfer_start, transfer->ptr[0] + ptr_offset, transfer_size));
            }

            FF(dpu_thread_boot_safe_for_dpu(dpu, 0, NULL, true));

            // Polling
            do {
                FF(dpu_poll_dpu(dpu, &run, &fault));
            } while (run && !fault);

            if (fault) {
                LOG_DPU(WARNING, dpu, "MRAM copy program has faulted");
                status = DPU_ERR_INTERNAL;
                goto end;
            }

            if (transfer_type == DPU_TRANSFER_FROM_MRAM) {
                // Fetching WRAM
                FF(dpu_copy_from_wram_for_dpu(dpu, transfer->ptr[0] + ptr_offset, transfer_start, transfer_size));
            }
        }
    }

    // Restoring IRAM & WRAM
    FF(dpu_copy_to_iram_for_dpu(dpu, 0, iram_save, nr_instructions));
    FF(dpu_copy_to_wram_for_dpu(dpu, 0, wram_save, wram_save_size));

    // Restoring context
    FF(dpu_restore_context_for_dpu(dpu, &context));
    FF(dpu_finalize_fault_process_for_dpu(dpu, &context));

    FF(dpu_custom_for_dpu(dpu, DPU_COMMAND_EVENT_END, (dpu_custom_command_args_t)DPU_EVENT_MRAM_ACCESS_PROGRAM));

end:
    if (context_init) {
        dpu_free_dpu_context(&context);
    }
    dpu_unlock_rank(rank);
    return status;
}
