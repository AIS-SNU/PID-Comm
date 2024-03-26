/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_DESCRIPTION_H
#define DPU_DESCRIPTION_H

#include <stdbool.h>
#include <stdint.h>

#include <dpu_types.h>
#include <dpu_target.h>
#include <dpu_chip_id.h>

#include <dpu_hw_description.h>

/**
 * @file dpu_description.h
 * @brief C API for DPU rank description.
 */

/**
 * @brief Description of the characteristics of a DPU rank
 */
typedef struct _dpu_description_t {
    /** Hardware characteristics for the DPU rank. */
    struct dpu_hw_description_t hw;
    /** Backend type for the DPU rank. */
    dpu_type_t type;

    /** Configuration for various parameters on the DPU rank. */
    struct {
        /** Whether the MRAM accesses are to be made via DPU program and the WRAM. */
        bool mram_access_by_dpu_only;
        /** Whether the MRAM accesses need to be preceeded by a MUX switch. */
        bool api_must_switch_mram_mux;
        /** Whether the MRAM MUX must be initialized. */
        bool init_mram_mux;

        /** Whether the IRAM repair procedure must be executed. */
        bool do_iram_repair;
        /** Whether the WRAM repair procedure must be executed. */
        bool do_wram_repair;
        /** Whether the VPD information should be ignored when repairing. */
        bool ignore_vpd;

        /** Whether the cycle accurate behavior must be enabled (FPGA only). */
        bool enable_cycle_accurate_behavior;

        /** Whether some defensive checks should be disabled. */
        bool disable_api_safe_checks;

        /** Whether the reset procedure should be disabled when allocating a new rank. */
        bool disable_reset_on_alloc;

        /** Whether the control refresh configuration should be changed (FPGA only). */
        bool ila_control_refresh;
    } configuration;

    /** Reference counter for this description. */
    uint32_t refcount;

    /** Internal structure specific to the backend type. */
    struct {
        /** The internal data. */
        void *data;
        /** Function to free the internal data when freeing this description. */
        void (*free)(void *data);
    } _internals;
} * dpu_description_t;

/**
 * @brief Acquires the ownership of the specified description, incrementing its reference counter.
 * @param description the description to be acquired
 */
void
dpu_acquire_description(dpu_description_t description);

/**
 * @brief Releases the ownership of the specified description, decrementing its reference counter. If nobody owns the
 *        description anymore, the description is freed.
 * @param description the description to be freed
 */
void
dpu_free_description(dpu_description_t description);

#endif // DPU_DESCRIPTION_H
