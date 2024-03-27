/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __DPU_TARGET_MACROS_H
#define __DPU_TARGET_MACROS_H

#include <limits.h>
#include <dpu_types.h>
#include <dpu_target.h>

/**
 * @file dpu_target_macros.h
 * @brief Macros to help set or get the DPU target from the rank id
 */

#define DPU_TARGET_LOG2_SIZE (4)
#define DPU_TARGET_SHIFT (CHAR_BIT * sizeof(dpu_rank_id_t) - DPU_TARGET_LOG2_SIZE)
#define DPU_TARGET_MASK ((1 << DPU_TARGET_SHIFT) - 1)

#ifdef __cplusplus
#ifndef _Static_assert
#define _Static_assert static_assert
#endif
#endif

_Static_assert((1 << DPU_TARGET_LOG2_SIZE) > NB_OF_DPU_TYPES, "Update DPU_TARGET_SHIFT");

#endif /* __DPU_TARGET_MACROS_H */
