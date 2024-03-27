/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_DEBUG_H
#define DPU_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_debug.h
 * @brief C API for debug operations on DPUs.
 */

/**
 * @brief Fetches the PC for each thread of the DPU.
 *
 * The DPU should be stopped before calling this function.
 * The different fields of the debug context must be correctly allocated by the function caller.
 *
 * @param dpu the unique identifier of the dpu
 * @param context the debug context which will store the PC of all threads
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_extract_pcs_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

/**
 * @brief Fetches the internal context for each thread of the DPU.
 *
 * dpu_extract_pcs_for_dpu or dpu_initialize_fault_process_for_dpu must have been called before calling this function.
 * This function extracts the working register values, the carry and zero flags for each thread, and also the
 * atomic register values for the DPU.
 *
 * @param dpu the unique identifier of the dpu
 * @param context the debug context which will store the internal context of all threads
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_extract_context_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

/**
 * @brief Restores the internal context for each thread of the DPU.
 *
 * @param dpu the unique identifier of the dpu
 * @param context the debug context which stores the internal context of all threads
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_restore_context_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

/**
 * @brief Prepares the DPU to be debugged after an execution fault.
 *
 * The DPU should be in fault before calling this function.
 * The different fields of the debug context must be correctly allocated by the function caller.
 *
 * @param dpu the unique identifier of the dpu
 * @param context the debug context which will store the thread scheduling and the different fault states
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_initialize_fault_process_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

/**
 * @brief Restarts the DPU execution after a fault and a potential debugging procedure.
 *
 * dpu_initialize_fault_process_for_dpu must have been called before calling this function.
 *
 * @param dpu the unique identifier of the dpu
 * @param context the debug context containing the thread scheduling information
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_finalize_fault_process_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

/**
 * @brief When debugging, makes the specified DPU thread execute one instruction.
 *
 * dpu_initialize_fault_process_for_dpu must have been called before calling this function.
 * The specified thread must have been booted, and still running.
 *
 * @param dpu the unique identifier of the dpu
 * @param thread the DPU thread which will execute a step
 * @param context the debug context containing the thread scheduling information
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_execute_thread_step_in_fault_for_dpu(struct dpu_t *dpu, dpu_thread_t thread, struct dpu_context_t *context);

/**
 * @brief Update internal information on the current state of a Control Interface for a given rank.
 * @param rank the DPU rank
 * @param slice_id the slice on which we want to update information
 * @param structure_value the new STRUCTURE value for this Control Interface
 * @param slice_target the new target for Control Interface select commands
 * @param host_mux_mram_state the new MUX state for each MRAM
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_set_debug_slice_info(struct dpu_rank_t *rank,
    uint32_t slice_id,
    uint64_t structure_value,
    uint64_t slice_target,
    dpu_bitfield_t host_mux_mram_state);

/**
 * @brief Restore the MRAM MUX state from a previously saved state.
 * @param rank the DPU rank
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_restore_mux_context_for_rank(struct dpu_rank_t *rank);

/**
 * @brief Save internal DPU rank state to be restored later.
 * @param rank the DPU rank
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_save_context_for_rank(struct dpu_rank_t *rank);

/**
 * @brief Restore internal DPU rank state from a previously saved state.
 * @param rank the DPU rank
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_restore_context_for_rank(struct dpu_rank_t *rank);

/**
 * @brief Create a core dump file from the given DPU debug context.
 * @param rank the DPU rank containing the DPU
 * @param exe_path path to the DPU program binary
 * @param core_file_path path the output core dump file
 * @param context the DPU debug context
 * @param wram snapshot of the DPU WRAM
 * @param mram snapshot of the DPU MRAM
 * @param iram snapshot of the DPU IRAM
 * @param wram_size WRAM size in bytes
 * @param mram_size MRAM size in bytes
 * @param iram_size IRAM size in bytes
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_create_core_dump(struct dpu_rank_t *rank,
    const char *exe_path,
    const char *core_file_path,
    struct dpu_context_t *context,
    uint8_t *wram,
    uint8_t *mram,
    uint8_t *iram,
    uint32_t wram_size,
    uint32_t mram_size,
    uint32_t iram_size);

/**
 * @brief Free the memory allocated in a DPU debug context.
 * @param context the context to be freed
 */
void
dpu_free_dpu_context(struct dpu_context_t *context);

/**
 * @brief Fill DPU debug context information and allocate memory based on the given DPU rank.
 * @param context the context to be filled
 * @param rank the DPU rank from which the information is extracted.
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_context_fill_from_rank(struct dpu_context_t *context, struct dpu_rank_t *rank);

/**
 * @brief This function removes, if existing, the debug context from a DPU structure
 * @param dpu the unique identifier of the DPU
 * @param debug_context The debug context, if existing, associated to the DPU
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_pop_debug_context(struct dpu_t *dpu, struct dpu_context_t **debug_context);

#endif // DPU_DEBUG_H
