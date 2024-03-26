/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __BIT_CONFIG_H__
#define __BIT_CONFIG_H__

#include <ufi/ufi_ci_types.h>
#include <dpu_types.h>

void dpu_bit_config_compute(u32 bit_order_result,
			    struct dpu_bit_config *config);

u32 dpu_bit_config_dpu2cpu(struct dpu_bit_config *config, u32 value);
u32 dpu_bit_config_cpu2dpu(struct dpu_bit_config *config, u32 value);

#endif // __BIT_CONFIG_H__
