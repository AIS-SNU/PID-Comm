/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_CONFIG_H
#define DPU_CONFIG_H

#include <stdint.h>

#include <dpu_types.h>
#include <dpu_error.h>

/**
 * @file dpu_config.h
 * @brief C API to configure DPUs.
 */

/**
 * @brief Reset the given DPU rank.
 * @param rank the DPU rank
 * @return Whether the operation was succesful.
 */
dpu_error_t
dpu_reset_rank(struct dpu_rank_t *rank);

/**
 * @brief Reset the given DPU.
 * @param dpu the DPU
 * @return Whether the operation was succesful.
 */
dpu_error_t
dpu_reset_dpu(struct dpu_t *dpu);

#endif // DPU_CONFIG_H
