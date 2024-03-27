/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_MEMORY_H
#define DPU_MEMORY_H

#include <dpu_error.h>
#include <dpu_types.h>
#include <dpu_transfer_matrix.h>

/**
 * @file dpu_memory.h
 * @brief C API to access DPU memory.
 */

/**
 * @brief Copy some instructions to the IRAM of all DPUs of a rank.
 * @param rank the DPU rank
 * @param iram_instruction_index where to start to copy the instructions in IRAM
 * @param source the buffer containing the instructions to copy
 * @param nb_of_instructions the number of instructions to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_iram_for_rank(struct dpu_rank_t *rank,
    iram_addr_t iram_instruction_index,
    const dpuinstruction_t *source,
    iram_size_t nb_of_instructions);

/**
 * @brief Copy some instructions to the IRAM of a DPU.
 * @param dpu the DPU
 * @param iram_instruction_index where to start to copy the instructions in IRAM
 * @param source the buffer containing the instructions to copy
 * @param nb_of_instructions the number of instructions to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_iram_for_dpu(struct dpu_t *dpu,
    iram_addr_t iram_instruction_index,
    const dpuinstruction_t *source,
    iram_size_t nb_of_instructions);

/**
 * @brief Copy some instructions from the IRAM of a DPU.
 * @param dpu the DPU
 * @param destination the buffer where the instructions will be stored
 * @param iram_instruction_index where to start to copy the instructions from IRAM
 * @param nb_of_instructions the number of instructions to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_from_iram_for_dpu(struct dpu_t *dpu,
    dpuinstruction_t *destination,
    iram_addr_t iram_instruction_index,
    iram_size_t nb_of_instructions);

/**
 * @brief Copy instructions to the IRAM of the DPUs of the rank specified in the transfer matrix
 * @param rank the DPU rank
 * @param matrix the transfer matrix
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_iram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix);

/**
 * @brief Copy instructions from the IRAM of the DPUs of the rank specified in the transfer matrix
 * @param rank the DPU rank
 * @param matrix the transfer matrix
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_from_iram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix);

/**
 * @brief Copy some words to the WRAM of all DPUs of a rank.
 * @param rank the DPU rank
 * @param wram_word_offset where to start to copy the words in WRAM
 * @param source the buffer containing the data to copy
 * @param nb_of_words the number of words to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_wram_for_rank(struct dpu_rank_t *rank,
    wram_addr_t wram_word_offset,
    const dpuword_t *source,
    wram_size_t nb_of_words);

/**
 * @brief Copy some words to the WRAM of a DPU.
 * @param dpu the DPU
 * @param wram_word_offset where to start to copy the words in WRAM
 * @param source the buffer containing the data to copy
 * @param nb_of_words the number of words to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_wram_for_dpu(struct dpu_t *dpu, wram_addr_t wram_word_offset, const dpuword_t *source, wram_size_t nb_of_words);

/**
 * @brief Copy some words to the WRAM of some DPUs of a rank.
 * @param rank the DPU rank
 * @param transfer_matrix the transfer matrix
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_to_wram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);

/**
 * @brief Copy some words from the WRAM of a DPU.
 * @param dpu the DPU
 * @param destination the buffer where the words will be stored
 * @param wram_word_offset where to start to copy the words from WRAM
 * @param nb_of_words the number of words to copy
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_from_wram_for_dpu(struct dpu_t *dpu, dpuword_t *destination, wram_addr_t wram_word_offset, wram_size_t nb_of_words);

/**
 * @brief Copy some words from the WRAM of some DPUs of a rank.
 * @param rank the DPU rank
 * @param transfer_matrix the transfer matrix
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_copy_from_wram_for_matrix(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);

/**
 * @brief Copy data to the MRAM of a DPU.
 * @param dpu the DPU
 * @param mram_byte_offset offset in wram from where to start the copy
 * @param source the data buffer for the MRAM data
 * @param nb_of_bytes the number of bytes to copy
 * @return ``DPU_OK`` in case of success, ``DPU_ERR_MRAM_BUSY`` means the host cannot write at the moment to the
 * MRAM and the access must be retried later, another value precising the error otherwise.
 */
dpu_error_t
dpu_copy_to_mram(struct dpu_t *dpu, mram_addr_t mram_byte_offset, const uint8_t *source, mram_size_t nb_of_bytes);

/**
 * @brief Copy data from the MRAM of a DPU.
 * @param dpu the DPU
 * @param destination the data buffer for the MRAM data
 * @param mram_byte_offset offset in wram from where to start the copy
 * @param nb_of_bytes the number of bytes to copy
 * @return ``DPU_OK`` in case of success, ``DPU_ERR_MRAM_BUSY`` means the host cannot write at the moment to the
 * MRAM and the access must be retried later, another value precising the error otherwise.
 */
dpu_error_t
dpu_copy_from_mram(struct dpu_t *dpu, uint8_t *destination, mram_addr_t mram_byte_offset, mram_size_t nb_of_bytes);

/**
 * @brief Copy data to the MRAMs.
 * @param rank the DPU rank
 * @param transfer_matrix Dynamically allocated matrix[dpus_per_slice][slice] describing the transfers for the whole rank
 * @return ``DPU_OK`` in case of success, ``DPU_ERR_MRAM_BUSY`` means the host cannot write at the moment to the
 * MRAM and the access must be retried later, another value precising the error otherwise.
 */
dpu_error_t
dpu_copy_to_mrams(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);

/**
 * @brief Copy data from the MRAMs.
 * @param rank the DPU rank
 * @param transfer_matrix Dynamically allocated matrix[dpus_per_slice][slice] describing the transfers for the whole rank
 * @return ``DPU_OK`` in case of success, ``DPU_ERR_MRAM_BUSY`` means the host cannot write at the moment to the
 * MRAM and the access must be retried later, another value precising the error otherwise.
 */
dpu_error_t
dpu_copy_from_mrams(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);


// Custom Functions

dpu_error_t
dpu_copy_mrams_RNS(struct dpu_rank_t *rank_src, struct dpu_rank_t *rank_dst, 
    struct dpu_transfer_matrix *transfer_matrix_src, 
    struct dpu_transfer_matrix *transfer_matrix_dst);

#endif // DPU_MEMORY_H
