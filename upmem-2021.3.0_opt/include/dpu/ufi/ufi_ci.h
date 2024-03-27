/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __CI_H__
#define __CI_H__

#include <ufi/ufi_ci_types.h>

#define TIMEOUT_COLOR (100000)

void ci_prepare_mask(u64 *buffer, u8 mask, u64 data);

u32 ci_fill_selected_dpu_mask(struct dpu_rank_t *rank, u8 ci, u8 *mask);

u32 ci_commit_commands(struct dpu_rank_t *rank, u64 *commands);
u32 ci_update_commands(struct dpu_rank_t *rank, u64 *commands);

u32 ci_exec_byte_discovery(struct dpu_rank_t *rank, u64 *commands,
			   u64 *results);
u32 ci_exec_reset_cmd(struct dpu_rank_t *rank, u64 *commands);
u32 ci_exec_8bit_cmd(struct dpu_rank_t *rank, u64 *commands, u8 *results);
u32 ci_exec_32bit_cmd(struct dpu_rank_t *rank, u64 *commands, u32 *results);
u32 ci_exec_void_cmd(struct dpu_rank_t *rank, u64 *commands);
u32 ci_exec_wait_mask_cmd(struct dpu_rank_t *rank, u64 *commands);

u32 ci_get_color(struct dpu_rank_t *rank, uint32_t *ret_data);

#endif /* __CI_H__ */
