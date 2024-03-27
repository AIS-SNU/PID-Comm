/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_debug.h>
#include <dpu_management.h>

#include <dpu_api_log.h>
#include <dpu_rank.h>
#include <dpu_mask.h>
#include <verbose_control.h>
#include <ufi/ufi.h>
#include <dpu_internals.h>

dpu_error_t
dpu_disable_one_dpu(struct dpu_t *dpu)
{
    LOG_DPU(VERBOSE, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    dpu_error_t status;
    struct dpu_rank_t *rank = dpu->rank;
    dpu_slice_id_t slice_id = dpu->slice_id;
    dpu_member_id_t dpu_id = dpu->dpu_id;

    FF(RANK_FEATURE(rank, disable_dpu)(dpu));

    rank->runtime.control_interface.slice_info[slice_id].enabled_dpus &= ~(1 << dpu_id);
    rank->runtime.control_interface.slice_info[slice_id].all_dpus_are_enabled = false;
    dpu->enabled = false;

end:
    return status;
}

dpu_error_t
drain_pipeline(struct dpu_t *dpu, struct dpu_context_t *context, bool should_add_to_schedule)
{
    LOG_DPU(DEBUG, dpu, "");
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_slice_id_t slice_id = dpu->slice_id;
    dpu_member_id_t member_id = dpu->dpu_id;

    uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
    bool still_draining;
    uint32_t *previous_run_register = NULL;
    uint32_t *run_register = NULL;
    dpu_selected_mask_t mask_one = dpu_mask_one(member_id);

    if ((previous_run_register = malloc(nr_of_threads_per_dpu * sizeof(*previous_run_register))) == NULL) {
        status = DPU_ERR_SYSTEM;
        goto end;
    }
    if ((run_register = malloc(nr_of_threads_per_dpu * sizeof(*run_register))) == NULL) {
        status = DPU_ERR_SYSTEM;
        goto end;
    }

    uint8_t mask = CI_MASK_ONE(slice_id);
    uint8_t result[DPU_MAX_NR_CIS];

    FF(ufi_select_dpu(rank, &mask, member_id));

    // 1. Fetching initial RUN register
    for (dpu_thread_t each_thread = 0; each_thread < nr_of_threads_per_dpu; ++each_thread) {
        FF(ufi_read_run_bit(rank, mask, each_thread, result));
        previous_run_register[each_thread] = result[slice_id];
    }

    // 2. Initial RUN register can be null: draining is done
    still_draining = false;
    for (dpu_thread_t each_thread = 0; each_thread < nr_of_threads_per_dpu; ++each_thread) {
        if ((previous_run_register[each_thread] & mask_one) != 0) {
            still_draining = true;
            break;
        }
    }

    if (still_draining) {
        do {
            iram_addr_t pc_results[DPU_MAX_NR_CIS];
            // 3. Looping until there is no more running thread
            // 3.1 Set replacing instruction to a STOP
            FF(ufi_debug_replace_stop(rank, mask));
            // 3.2 Popping out a thread from the FIFO
            FF(ufi_set_dpu_fault_and_step(rank, mask));
            // 3.3 Fetching potential PC (if a thread has been popped out)
            FF(ufi_debug_pc_read(rank, mask, pc_results));
            iram_addr_t pc = pc_results[slice_id];
            // 3.4 Fetching new RUN register
            for (dpu_thread_t each_thread = 0; each_thread < nr_of_threads_per_dpu; ++each_thread) {
                FF(ufi_read_run_bit(rank, mask, each_thread, result));
                run_register[each_thread] = result[slice_id];
            }

            still_draining = false;
            for (dpu_thread_t each_thread = 0; each_thread < nr_of_threads_per_dpu; ++each_thread) {
                if ((run_register[each_thread] & mask_one) != 0) {
                    still_draining = true;
                } else if (context && (previous_run_register[each_thread] & mask_one) != 0) {
                    context->pcs[each_thread] = pc;
                    if (should_add_to_schedule) {
                        context->scheduling[each_thread] = context->nr_of_running_threads++;
                    }
                    previous_run_register[each_thread] = 0;
                }
            }
        } while (still_draining);

        // 4. Clear dbg_replace_en
        FF(ufi_clear_debug_replace(rank, mask));
    }

end:
    free(run_register);
    free(previous_run_register);
    return status;
}

void
set_pc_in_core_dump_or_restore_registers(dpu_thread_t thread,
    iram_addr_t pc,
    dpuinstruction_t *program,
    iram_size_t program_size,
    uint8_t nr_of_threads)
{
    uint32_t upper_index = (uint32_t)(program_size - (12 * (nr_of_threads - thread - 1)));

    program[upper_index - 1] |= pc;
    program[upper_index - 4] |= pc;
    program[upper_index - 7] |= pc;
    program[upper_index - 10] |= pc;
}

dpu_error_t
map_rank_status_to_api_status(dpu_rank_status_e rank_status)
{
    switch (rank_status) {
        case DPU_RANK_SUCCESS:
            return DPU_OK;
        case DPU_RANK_COMMUNICATION_ERROR:
            return DPU_ERR_DRIVER;
        case DPU_RANK_BACKEND_ERROR:
            return DPU_ERR_DRIVER;
        case DPU_RANK_SYSTEM_ERROR:
            return DPU_ERR_SYSTEM;
        case DPU_RANK_INVALID_PROPERTY_ERROR:
            return DPU_ERR_INVALID_PROFILE;
        default:
            return DPU_ERR_INTERNAL;
    }
}
