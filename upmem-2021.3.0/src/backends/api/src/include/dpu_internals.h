/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_INTERNALS_H
#define DPU_INTERNALS_H

#include <stdbool.h>
#include <stdint.h>

#include <dpu_error.h>
#include <dpu_types.h>
#include <dpu_debug.h>
#include <dpu_config.h>

#include <dpu_rank.h>

#define FF(s)                                                                                                                    \
    do {                                                                                                                         \
        if ((status = (s)) != DPU_OK) {                                                                                          \
            goto end;                                                                                                            \
        }                                                                                                                        \
    } while (0)

#define verify_thread_id(t, r)                                                                                                   \
    do {                                                                                                                         \
        if ((t) >= (r)->description->hw.dpu.nr_of_threads) {                                                                     \
            LOG_RANK(WARNING, r, "ERROR: invalid thread id (%d >= %d)", t, (r)->description->hw.dpu.nr_of_threads);              \
            return DPU_ERR_INVALID_THREAD_ID;                                                                                    \
        }                                                                                                                        \
    } while (0)

#define verify_notify_id(t, r)                                                                                                   \
    do {                                                                                                                         \
        if ((t) >= (r)->description->hw.dpu.nr_of_notify_bits) {                                                                 \
            LOG_RANK(WARNING, r, "ERROR: invalid notify id (%d >= %d)", t, (r)->description->hw.dpu.nr_of_notify_bits);          \
            return DPU_ERR_INVALID_NOTIFY_ID;                                                                                    \
        }                                                                                                                        \
    } while (0)

#define verify_wram_access(o, s, r)                                                                                              \
    do {                                                                                                                         \
        if (!(s)) {                                                                                                              \
            LOG_RANK(WARNING, r, "WARNING: wram access of size 0 at offset %d", o);                                              \
            return DPU_OK;                                                                                                       \
        }                                                                                                                        \
        if (((o) >= (r)->description->hw.memories.wram_size) || (((o) + (s)) > (r)->description->hw.memories.wram_size)) {       \
            LOG_RANK(WARNING,                                                                                                    \
                r,                                                                                                               \
                "ERROR: invalid wram access ((%d >= %d) || (%d > %d))",                                                          \
                o,                                                                                                               \
                (r)->description->hw.memories.wram_size,                                                                         \
                (o) + (s),                                                                                                       \
                (r)->description->hw.memories.wram_size);                                                                        \
            return DPU_ERR_INVALID_WRAM_ACCESS;                                                                                  \
        }                                                                                                                        \
    } while (0)

#define verify_iram_access(o, s, r)                                                                                              \
    do {                                                                                                                         \
        if (!(s)) {                                                                                                              \
            LOG_RANK(WARNING, r, "WARNING: iram access of size 0 at offset %d", o);                                              \
            return DPU_OK;                                                                                                       \
        }                                                                                                                        \
        if (((o) >= (r)->description->hw.memories.iram_size) || (((o) + (s)) > (r)->description->hw.memories.iram_size)) {       \
            LOG_RANK(WARNING,                                                                                                    \
                r,                                                                                                               \
                "ERROR: invalid iram access ((%d >= %d) || (%d > %d))",                                                          \
                o,                                                                                                               \
                (r)->description->hw.memories.iram_size,                                                                         \
                (o) + (s),                                                                                                       \
                (r)->description->hw.memories.iram_size);                                                                        \
            return DPU_ERR_INVALID_IRAM_ACCESS;                                                                                  \
        }                                                                                                                        \
    } while (0)

#define verify_mram_access_offset_and_size(o, s, r)                                                                              \
    do {                                                                                                                         \
        if (((s) % 8 != 0) || ((o) % 8 != 0)) {                                                                                  \
            LOG_RANK(WARNING, (r), "ERROR: invalid mram acess (size and offset need to be 8-byte aligned): (%d, %d)", (o), (s)); \
            return DPU_ERR_INVALID_MRAM_ACCESS;                                                                                  \
        }                                                                                                                        \
        if (((o) >= (r)->description->hw.memories.mram_size) || (((o) + (s)) > (r)->description->hw.memories.mram_size)) {       \
            LOG_RANK(WARNING,                                                                                                    \
                r,                                                                                                               \
                "ERROR: invalid mram access ((%d >= %d) || (%d > %d))",                                                          \
                o,                                                                                                               \
                (r)->description->hw.memories.mram_size,                                                                         \
                (o) + (s),                                                                                                       \
                (r)->description->hw.memories.mram_size);                                                                        \
            return DPU_ERR_INVALID_MRAM_ACCESS;                                                                                  \
        }                                                                                                                        \
    } while (0)

dpu_error_t
drain_pipeline(struct dpu_t *dpu, struct dpu_context_t *context, bool should_add_to_schedule);

void
set_pc_in_core_dump_or_restore_registers(dpu_thread_t thread,
    iram_addr_t pc,
    dpuinstruction_t *program,
    iram_size_t program_size,
    uint8_t nr_of_threads);

dpu_clock_division_t
from_division_factor_to_dpu_enum(uint8_t factor);

dpu_error_t
map_rank_status_to_api_status(dpu_rank_status_e rank_status);

dpu_error_t
dpu_thread_boot_safe_for_dpu(struct dpu_t *dpu, uint8_t thread, uint8_t *previous, bool resume);
dpu_error_t
dpu_thread_boot_safe_for_rank(struct dpu_rank_t *rank, uint8_t thread, uint8_t *previous, bool resume);

uint8_t
dpu_get_host_mux_mram_state(struct dpu_rank_t *rank, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id);
void
dpu_set_host_mux_mram_state(struct dpu_rank_t *rank, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id, bool set);

dpu_error_t
dpu_disable_one_dpu(struct dpu_t *dpu);

#endif /* DPU_INTERNALS_H */
