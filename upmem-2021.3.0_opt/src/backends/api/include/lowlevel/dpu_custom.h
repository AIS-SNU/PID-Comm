/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_CUSTOM_H
#define DPU_CUSTOM_H

#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_custom.h
 * @brief C API for custom operations on DPUs.
 */

/**
 * @brief Custom DPU operations that may have different behaviors on different targets.
 */
typedef enum _dpu_custom_command_t {
    /** Reset for a single DPU. */
    DPU_COMMAND_DPU_SOFT_RESET,
    /** Reset for a slice. */
    DPU_COMMAND_ALL_SOFT_RESET,
    /** Allows configuration right before booting DPU. */
    DPU_COMMAND_DPU_PREEXECUTION,
    /** Allows configuration right before booting DPUs. */
    DPU_COMMAND_ALL_PREEXECUTION,
    /** Allows configuration right after the end of DPU execution. */
    DPU_COMMAND_DPU_POSTEXECUTION,
    /** Allows configuration right after the end of DPUs execution. */
    DPU_COMMAND_ALL_POSTEXECUTION,
    /** Add custom label for the current timestamp. */
    DPU_COMMAND_CUSTOM_LABEL,
    /** Detailed backend postmortem. */
    DPU_COMMAND_POSTMORTEM,
    /** Report additional backend status. */
    DPU_COMMAND_SYSTEM_REPORT,
    /** Provides the DPU binary path to the backend. */
    DPU_COMMAND_BINARY_PATH,
    /** Notifies that an event has started. */
    DPU_COMMAND_EVENT_START,
    /** Notifies that an event has ended. */
    DPU_COMMAND_EVENT_END,
} dpu_custom_command_t;

/**
 * @brief Generic argument type for a DPU custom operation.
 */
typedef void *dpu_custom_command_args_t;

/**
 * @param rank the unique identifier of the rank
 * @param command Custom command identifier to send
 * @param args Pointer to arguments for the command
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_custom_for_rank(struct dpu_rank_t *rank, dpu_custom_command_t command, dpu_custom_command_args_t args);

/**
 * @param dpu the unique identifier of the DPU
 * @param command Custom command identifier to send
 * @param args Pointer to arguments for the command
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_custom_for_dpu(struct dpu_t *dpu, dpu_custom_command_t command, dpu_custom_command_args_t args);

#endif // DPU_CUSTOM_H
