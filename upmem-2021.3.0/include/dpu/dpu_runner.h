/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_RUNNER_H
#define DPU_RUNNER_H

#include <pthread.h>
#include <stdint.h>

#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_runner.h
 * @brief C API for DPU execution operations.
 */

/**
 * @brief Launch a thread on all DPUs of a rank.
 * @param rank the the rank of DPUs
 * @param thread the thread to launch
 * @param should_resume whether the thread PC should be kept intact or set to 0
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_launch_thread_on_rank(struct dpu_rank_t *rank, dpu_thread_t thread, bool should_resume);

/**
 * @brief Launch a thread on a DPU.
 * @param dpu the DPU
 * @param thread the thread to launch
 * @param should_resume whether the thread PC should be kept intact or set to 0
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_launch_thread_on_dpu(struct dpu_t *dpu, dpu_thread_t thread, bool should_resume);

/**
 * @brief Poll the status of all DPUs of a rank, and update the run_context structure inside the rank structure.
 * @param rank the rank of DPUs
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_poll_rank(struct dpu_rank_t *rank);

/**
 * @brief Poll the status of a DPU.
 * @param dpu the DPU
 * @param dpu_is_running whether the DPU is running
 * @param dpu_is_in_fault whether the DPU is in fault
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_poll_dpu(struct dpu_t *dpu, bool *dpu_is_running, bool *dpu_is_in_fault);

/**
 * @brief DPU Rank context to track the DPUs running state.
 */
typedef struct _dpu_run_context_t {
    /** Bitfields giving which DPUs are running. */
    dpu_bitfield_t dpu_running[DPU_MAX_NR_CIS];
    /** Bitfields giving which DPUs are in fault. */
    dpu_bitfield_t dpu_in_fault[DPU_MAX_NR_CIS];
    /** Latest error returned by a poll command. */
    dpu_error_t poll_status;
    /**
     * @brief The number of DPUs which are running.
     * @internal Must be consistent with the dpu_running & dpu_in_fault fields.
     */
    uint8_t nb_dpu_running;
} * dpu_run_context_t;

/**
 * @brief Fetches the rank run context.
 * @param rank the unique identifier of the rank
 * @return The pointer on the rank run context.
 */
dpu_run_context_t
dpu_get_run_context(struct dpu_rank_t *rank);

/**
 * @brief Boot the specified dpu
 * @param dpu The targeted dpu
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_boot_dpu(struct dpu_t *dpu);

/**
 * @brief Boot the specified DPU rank
 * @param rank The targeted rank
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_boot_rank(struct dpu_rank_t *rank);

/**
 * @brief Check the status of the specified DPU rank
 * @param rank The targeted rank
 * @param done Whether the rank is running or not (done means not running)
 * @param fault Whether at least one DPU of the rank is in fault
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_status_rank(struct dpu_rank_t *rank, bool *done, bool *fault);

/**
 * @brief Check the status of the specified DPU
 * @param dpu The targeted dpu
 * @param done Whether the DPU is running or not (done means not running)
 * @param fault Whether the DPU is in fault
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_status_dpu(struct dpu_t *dpu, bool *done, bool *fault);

#endif // DPU_RUNNER_H
