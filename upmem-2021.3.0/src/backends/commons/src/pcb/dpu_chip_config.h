/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_CHIP_CONFIG_H
#define DPU_CHIP_CONFIG_H

#include <stddef.h>
#include <dpu_description.h>

extern const dpu_description_t dpu_chip_descriptions[NEXT_DPU_CHIP_ID];

static inline dpu_description_t
default_description_for_chip(dpu_chip_id_e chip_id)
{
    return (chip_id < NEXT_DPU_CHIP_ID) ? dpu_chip_descriptions[chip_id] : NULL;
}

#endif // DPU_CHIP_CONFIG_H
