/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_attributes.h>
#include <dpu_custom.h>
#include <dpu_api_log.h>
#include <dpu_internals.h>
#include <dpu_management.h>
#include <dpu_rank_handler.h>
#include <dpu_rank.h>

__API_SYMBOL__ dpu_error_t
dpu_custom_for_rank(struct dpu_rank_t *rank, dpu_custom_command_t command, dpu_custom_command_args_t args)
{
    LOG_RANK(VERBOSE, rank, "%d", command);

    dpu_lock_rank(rank);
    dpu_rank_status_e status
        = rank->handler_context->handler->custom_operation(rank, (dpu_slice_id_t)-1, (dpu_member_id_t)-1, command, args);
    dpu_unlock_rank(rank);

    return map_rank_status_to_api_status(status);
}

__API_SYMBOL__ dpu_error_t
dpu_custom_for_dpu(struct dpu_t *dpu, dpu_custom_command_t command, dpu_custom_command_args_t args)
{
    LOG_DPU(VERBOSE, dpu, "%d", command);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_slice_id_t slice_id = dpu->slice_id;
    dpu_member_id_t member_id = dpu->dpu_id;

    dpu_lock_rank(rank);
    dpu_rank_status_e status = rank->handler_context->handler->custom_operation(rank, slice_id, member_id, command, args);
    dpu_unlock_rank(rank);

    return map_rank_status_to_api_status(status);
}