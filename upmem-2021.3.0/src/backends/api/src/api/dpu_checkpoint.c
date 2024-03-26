/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu.h>

#include <dpu_debug.h>
#include <dpu_log_utils.h>
#include <dpu_api_verbose.h>
#include <dpu_error.h>
#include <dpu_types.h>
#include <dpu_attributes.h>
#include <dpu_memory.h>
#include <dpu_rank.h>

static bool
flags_contain(uint32_t flags, uint32_t flag)
{
    return (flags & flag) != 0;
}

__API_SYMBOL__ dpu_error_t
dpu_checkpoint_save(struct dpu_set_t set, dpu_checkpoint_flags_t flags, struct dpu_context_t *context)
{
    LOG_FN(DEBUG, "%08x, %p", flags, *context);

    switch (set.kind) {
        case DPU_SET_DPU: {
            dpu_error_t status = DPU_OK;

            if ((status = dpu_context_fill_from_rank(context, set.dpu->rank)) != DPU_OK) {
                goto end;
            }

            if (flags_contain(flags, DPU_CHECKPOINT_INTERNAL)) {
                if ((status = dpu_initialize_fault_process_for_dpu(set.dpu, context)) != DPU_OK) {
                    goto end;
                }

                if ((status = dpu_extract_context_for_dpu(set.dpu, context)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_IRAM)) {
                iram_size_t iram_size = set.dpu->rank->description->hw.memories.iram_size;

                if ((context->iram = malloc(iram_size * sizeof(dpuinstruction_t))) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto end;
                }

                if ((status = dpu_copy_from_iram_for_dpu(set.dpu, context->iram, 0, iram_size)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_MRAM)) {
                mram_size_t mram_size = set.dpu->rank->description->hw.memories.mram_size;

                if ((context->mram = malloc(mram_size * sizeof(uint8_t))) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto end;
                }

                if ((status = dpu_copy_from_mram(set.dpu, context->mram, 0, mram_size)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_WRAM)) {
                wram_size_t wram_size = set.dpu->rank->description->hw.memories.wram_size;

                if ((context->wram = malloc(wram_size * sizeof(dpuword_t))) == NULL) {
                    status = DPU_ERR_SYSTEM;
                    goto end;
                }

                if ((status = dpu_copy_from_wram_for_dpu(set.dpu, context->wram, 0, wram_size)) != DPU_OK) {
                    goto end;
                }
            }
        end:
            if (status != DPU_OK) {
                dpu_free_dpu_context(context);
            }
            return status;
        }
        default:
            return DPU_ERR_INVALID_DPU_SET;
    }
}

__API_SYMBOL__ dpu_error_t
dpu_checkpoint_restore(struct dpu_set_t set, dpu_checkpoint_flags_t flags, struct dpu_context_t *context)
{
    LOG_FN(DEBUG, "%08x, %p", flags, context);

    switch (set.kind) {
        case DPU_SET_DPU: {
            dpu_error_t status = DPU_OK;

            if (flags_contain(flags, DPU_CHECKPOINT_INTERNAL)) {
                if ((status = dpu_restore_context_for_dpu(set.dpu, context)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_IRAM)) {
                if ((status = dpu_copy_to_iram_for_dpu(set.dpu, 0, context->iram, context->info.iram_size)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_MRAM)) {
                if ((status = dpu_copy_to_mram(set.dpu, 0, context->mram, context->info.mram_size)) != DPU_OK) {
                    goto end;
                }
            }

            if (flags_contain(flags, DPU_CHECKPOINT_WRAM)) {
                if ((status = dpu_copy_to_wram_for_dpu(set.dpu, 0, context->wram, context->info.wram_size)) != DPU_OK) {
                    goto end;
                }
            }
        end:
            return status;
        }
        default:
            return DPU_ERR_INVALID_DPU_SET;
    }
}

__API_SYMBOL__ dpu_error_t
dpu_checkpoint_free(struct dpu_context_t *context)
{
    LOG_FN(DEBUG, "%p", context);

    dpu_free_dpu_context(context);
    return DPU_OK;
}
