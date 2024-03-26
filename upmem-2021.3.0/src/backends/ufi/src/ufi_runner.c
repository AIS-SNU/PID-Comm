/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

#include <dpu_error.h>
#include <dpu_rank.h>
#include <dpu_types.h>

#include <dpu_internals.h>

#include <ufi/ufi.h>
#include <ufi/ufi_config.h>
#include <ufi_rank_utils.h>

__API_SYMBOL__ dpu_error_t ci_start_thread_rank(struct dpu_rank_t *rank,
						dpu_thread_t thread,
						bool resume, uint8_t *previous)
{
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t ci_mask = ALL_CIS;
	dpu_error_t status;
	dpu_member_id_t each_dpu;
	dpu_slice_id_t each_slice;

	FF(ufi_select_all(rank, &ci_mask));

	if (ci_mask == ALL_CIS) {
		FF(dpu_switch_mux_for_rank(rank, false));
	} else {
		for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
			uint8_t mux_mask = 0;

			for (each_slice = 0; each_slice < nr_cis;
			     ++each_slice) {
				if (!CI_MASK_ON(ci_mask, each_slice)) {
					/* Either the DPU of this slice is being booted, then you switch the mux at DPU-side, which
                     * amounts to clearing the DPU's bit (which amounts to doing nothing here) or you take care
                     * of not changing the mux direction.
                     */
					mux_mask |= dpu_get_host_mux_mram_state(
							    rank, each_slice,
							    each_dpu)
						    << each_slice;
				}
			}
			FF(dpu_switch_mux_for_dpu_line(rank, each_dpu,
						       mux_mask));
		}
	}

	FF(ufi_select_all(rank, &ci_mask));

	if (resume) {
		FF(ufi_thread_resume(rank, ci_mask, thread, previous));
	} else {
		FF(ufi_thread_boot(rank, ci_mask, thread, previous));
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_start_thread_dpu(struct dpu_t *dpu,
					       dpu_thread_t thread, bool resume,
					       uint8_t *previous)
{
	struct dpu_rank_t *rank = dpu->rank;
	uint8_t ci_mask = CI_MASK_ONE(dpu->slice_id);
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t mux_mask = 0;
	dpu_error_t status;
	dpu_slice_id_t each_slice;

	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		if (!CI_MASK_ON(ci_mask, each_slice)) {
			/* Either the DPU of this slice is being booted, then you switch the mux at DPU-side, which
             * amounts to clearing the DPU's bit (which amounts to doing nothing here) or you take care
             * of not changing the mux direction.
             */
			mux_mask |= dpu_get_host_mux_mram_state(
					    rank, each_slice, dpu->dpu_id)
				    << each_slice;
		}
	}

	FF(dpu_switch_mux_for_dpu_line(rank, dpu->dpu_id, mux_mask));

	FF(ufi_select_dpu(rank, &ci_mask, dpu->dpu_id));
	if (resume) {
		FF(ufi_thread_resume(rank, ci_mask, thread, previous));
	} else {
		FF(ufi_thread_boot(rank, ci_mask, thread, previous));
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_poll_rank(struct dpu_rank_t *rank,
					dpu_bitfield_t *run,
					dpu_bitfield_t *fault)
{
	uint8_t mask = ALL_CIS;
	dpu_error_t status;

	FF(ufi_select_all(rank, &mask));
	FF(ufi_read_dpu_run(rank, mask, run));
	FF(ufi_read_dpu_fault(rank, mask, fault));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_sample_pc_dpu(struct dpu_t *dpu, iram_addr_t *pc)
{
	iram_addr_t pcs[DPU_MAX_NR_CIS];

	struct dpu_rank_t *rank = dpu->rank;
	uint8_t ci_mask = CI_MASK_ONE(dpu->slice_id);
	dpu_error_t status;

	FF(ufi_select_dpu(rank, &ci_mask, dpu->dpu_id));
	FF(ufi_debug_pc_sample(rank, ci_mask));
	FF(ufi_debug_pc_read(rank, ci_mask, pcs));

	*pc = pcs[dpu->slice_id];

end:
	return status;
}
