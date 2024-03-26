/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_types.h>

dpu_error_t ci_start_thread_rank(struct dpu_rank_t *rank, dpu_thread_t thread,
				 bool resume, uint8_t *previous);

dpu_error_t ci_start_thread_dpu(struct dpu_t *dpu, dpu_thread_t thread,
				bool resume, uint8_t *previous);

dpu_error_t ci_poll_rank(struct dpu_rank_t *rank, dpu_bitfield_t *run,
			 dpu_bitfield_t *fault);

dpu_error_t ci_sample_pc_dpu(struct dpu_t *dpu, iram_addr_t *pc);
