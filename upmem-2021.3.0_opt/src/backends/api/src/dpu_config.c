/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <string.h>

#include <dpu_config.h>
#include <dpu_custom.h>
#include <dpu_error.h>
#include <dpu_description.h>
#include <dpu_management.h>
#include <dpu_types.h>

#include <dpu_api_log.h>
#include <dpu_attributes.h>
#include <dpu_internals.h>
#include <dpu_mask.h>
#include <dpu_predef_programs.h>
#include <dpu_rank.h>
#include <verbose_control.h>
#include <dpu_instruction_encoder.h>

__API_SYMBOL__ dpu_error_t
dpu_reset_rank(struct dpu_rank_t *rank)
{
    LOG_RANK(DEBUG, rank, "");

    dpu_error_t status;

    dpu_lock_rank(rank);

    FF(dpu_custom_for_rank(rank, DPU_COMMAND_EVENT_START, (dpu_custom_command_args_t)DPU_EVENT_RESET));
    FF(dpu_custom_for_rank(rank, DPU_COMMAND_ALL_SOFT_RESET, NULL));

    FF(RANK_FEATURE(rank, reset_rank)(rank));

    FF(dpu_custom_for_rank(rank, DPU_COMMAND_EVENT_END, (dpu_custom_command_args_t)DPU_EVENT_RESET));

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_reset_dpu(struct dpu_t *dpu)
{
    LOG_DPU(DEBUG, dpu, "");
    struct dpu_rank_t *rank = dpu->rank;

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, reset_dpu)(dpu);
    dpu_unlock_rank(rank);

    return status;
}

uint8_t
dpu_get_host_mux_mram_state(struct dpu_rank_t *rank, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id)
{
    return (rank->runtime.control_interface.slice_info[slice_id].host_mux_mram_state >> dpu_id) & 1;
}

void
dpu_set_host_mux_mram_state(struct dpu_rank_t *rank, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id, bool set)
{
    if (set)
        rank->runtime.control_interface.slice_info[slice_id].host_mux_mram_state |= (1 << dpu_id);
    else
        rank->runtime.control_interface.slice_info[slice_id].host_mux_mram_state &= ~(1 << dpu_id);
}
