/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <stdio.h>

#include <dpu_error.h>
#include <dpu_attributes.h>
#include <dpu_api_verbose.h>
#include <dpu_log_utils.h>

#include <dpu_rank.h>

static const char *
dpu_thread_job_type_to_string(enum dpu_thread_job_type job_type)
{
    switch (job_type) {
        case DPU_THREAD_JOB_LAUNCH_RANK:
            return "launch_rank";
        case DPU_THREAD_JOB_LAUNCH_DPU:
            return "launch_dpu";
        case DPU_THREAD_JOB_SYNC:
            return "synchronization";
        case DPU_THREAD_JOB_COPY_WRAM_TO_MATRIX:
            return "copy_wram_to_matrix";
        case DPU_THREAD_JOB_COPY_IRAM_TO_MATRIX:
            return "copy_iram_to_matrix";
        case DPU_THREAD_JOB_COPY_MRAM_TO_MATRIX:
            return "copy_mram_to_matrix";
        case DPU_THREAD_JOB_COPY_WRAM_FROM_MATRIX:
            return "copy_wram_from_matrix";
        case DPU_THREAD_JOB_COPY_IRAM_FROM_MATRIX:
            return "copy_iram_from_matrix";
        case DPU_THREAD_JOB_COPY_MRAM_FROM_MATRIX:
            return "copy_mram_from_matrix";
        case DPU_THREAD_JOB_COPY_WRAM_TO_RANK:
            return "copy_wram_to_rank";
        case DPU_THREAD_JOB_COPY_IRAM_TO_RANK:
            return "copy_iram_to_rank";
        case DPU_THREAD_JOB_COPY_MRAM_TO_RANK:
            return "copy_mram_to_rank";
        case DPU_THREAD_JOB_CALLBACK:
            return "callback";
        default:
            return "unknown job type";
    }
}

static const char *
dpu_error_to_string_switch(dpu_error_t status)
{
    switch (status) {
        case DPU_OK:
            return "success";
        case DPU_ERR_SYSTEM:
            return "system error";
        case DPU_ERR_DPU_ALREADY_RUNNING:
            return "dpu is already running";
        case DPU_ERR_INVALID_LAUNCH_POLICY:
            return "invalid launch policy";
        case DPU_ERR_DPU_FAULT:
            return "dpu is in fault";
        case DPU_ERR_INTERNAL:
            return "internal error";
        case DPU_ERR_DRIVER:
            return "driver error";
        case DPU_ERR_INVALID_THREAD_ID:
            return "invalid thread id";
        case DPU_ERR_INVALID_NOTIFY_ID:
            return "invalid notify id";
        case DPU_ERR_INVALID_WRAM_ACCESS:
            return "invalid wram access";
        case DPU_ERR_INVALID_IRAM_ACCESS:
            return "invalid iram access";
        case DPU_ERR_MRAM_BUSY:
            return "mram is busy";
        case DPU_ERR_INVALID_MRAM_ACCESS:
            return "invalid mram access";
        case DPU_ERR_CORRUPTED_MEMORY:
            return "corrupted memory";
        case DPU_ERR_DPU_DISABLED:
            return "dpu is disabled";
        case DPU_ERR_TIMEOUT:
            return "result timeout";
        case DPU_ERR_INVALID_PROFILE:
            return "invalid profile";
        case DPU_ERR_ALLOCATION:
            return "dpu allocation error";
        case DPU_ERR_UNKNOWN_SYMBOL:
            return "undefined symbol";
        case DPU_ERR_LOG_FORMAT:
            return "logging format error";
        case DPU_ERR_LOG_CONTEXT_MISSING:
            return "logging context missing";
        case DPU_ERR_LOG_BUFFER_TOO_SMALL:
            return "logging buffer too small";
        case DPU_ERR_INVALID_DPU_SET:
            return "invalid action for this dpu set";
        case DPU_ERR_INVALID_SYMBOL_ACCESS:
            return "invalid memory symbol access";
        case DPU_ERR_TRANSFER_ALREADY_SET:
            return "dpu transfer is already set";
        case DPU_ERR_NO_PROGRAM_LOADED:
            return "no program has been loaded in the dpu set";
        case DPU_ERR_DIFFERENT_DPU_PROGRAMS:
            return "cannot access symbol on dpus with different programs";
        case DPU_ERR_INVALID_MEMORY_TRANSFER:
            return "invalid memory transfer";
        case DPU_ERR_ELF_INVALID_FILE:
            return "invalid ELF file";
        case DPU_ERR_ELF_NO_SUCH_FILE:
            return "no such ELF file";
        case DPU_ERR_ELF_NO_SUCH_SECTION:
            return "no such ELF section";
        case DPU_ERR_VPD_INVALID_FILE:
            return "invalid VPD file";
        case DPU_ERR_VPD_NO_REPAIR:
            return "repairs have not been generated";
        case DPU_ERR_NO_THREAD_PER_RANK:
            return "no thread to perform multiple-rank operation";
        case DPU_ERR_INVALID_BUFFER_SIZE:
            return "invalid buffer size";
        case DPU_ERR_NONBLOCKING_SYNC_CALLBACK:
            return "nonblocking and synchronous callback is invalid";
        case DPU_ERR_ASYNC_JOBS:
            return "asynchronous operation error";
        default:
            return "unknown error";
    }
}

__API_SYMBOL__ const char *
dpu_error_to_string(dpu_error_t status)
{
    char *string;
    if (status & DPU_ERR_ASYNC_JOBS) {
        status &= (~DPU_ERR_ASYNC_JOBS);
        enum dpu_thread_job_type job_type = status >> DPU_ERROR_ASYNC_JOB_TYPE_SHIFT;
        dpu_error_t error = status & ((1 << DPU_ERROR_ASYNC_JOB_TYPE_SHIFT) - 1);
        if (asprintf(&string,
                "%s (%s): %s (0x%x)",
                dpu_error_to_string_switch(DPU_ERR_ASYNC_JOBS),
                dpu_thread_job_type_to_string(job_type),
                dpu_error_to_string_switch(error),
                status)
            <= 0) {
            LOG_FN(INFO, "asprintf failed");
            return NULL;
        }
    } else {
        if (asprintf(&string, "%s", dpu_error_to_string_switch(status)) <= 0) {
            LOG_FN(INFO, "asprintf failed");
            return NULL;
        }
    }
    return string;
}
