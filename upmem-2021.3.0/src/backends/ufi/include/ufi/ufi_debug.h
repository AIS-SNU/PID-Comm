/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_types.h>

dpu_error_t ci_debug_init_dpu(struct dpu_t *dpu, struct dpu_context_t *context);

dpu_error_t ci_debug_teardown_dpu(struct dpu_t *dpu,
				  struct dpu_context_t *context);

dpu_error_t ci_debug_step_dpu(struct dpu_t *dpu, struct dpu_context_t *context,
			      dpu_thread_t thread);

dpu_error_t ci_debug_read_pcs_dpu(struct dpu_t *dpu,
				  struct dpu_context_t *context);

dpu_error_t ci_debug_extract_context_dpu(struct dpu_t *dpu,
					 struct dpu_context_t *context);

dpu_error_t ci_debug_restore_context_dpu(struct dpu_t *dpu,
					 struct dpu_context_t *context);

dpu_error_t ci_debug_save_context_rank(struct dpu_rank_t *rank);

dpu_error_t ci_debug_restore_mux_context_rank(struct dpu_rank_t *rank);

dpu_error_t ci_debug_restore_context_rank(struct dpu_rank_t *rank);
