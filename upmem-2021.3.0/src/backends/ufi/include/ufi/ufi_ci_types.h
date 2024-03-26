/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __CI_TYPES_H__
#define __CI_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct dpu_rank_t;

#define CI_MASK_ON(mask, ci) (((mask) & (1u << (ci))) != 0)

#endif /* __CI_TYPES_H__ */
