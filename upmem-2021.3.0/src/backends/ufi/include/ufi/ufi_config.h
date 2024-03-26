/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_types.h>

dpu_error_t ci_reset_rank(struct dpu_rank_t *rank);

dpu_error_t ci_reset_dpu(struct dpu_t *dpu);

dpu_error_t ci_disable_dpu(struct dpu_t *dpu);

dpu_error_t dpu_wavegen_read_status(struct dpu_t *dpu, uint8_t address,
				    uint8_t *value);

dpu_error_t dpu_switch_mux_for_dpu_line(struct dpu_rank_t *rank, uint8_t dpu_id,
					uint8_t mask);

dpu_error_t dpu_switch_mux_for_rank(struct dpu_rank_t *rank,
				    bool set_mux_for_host);
