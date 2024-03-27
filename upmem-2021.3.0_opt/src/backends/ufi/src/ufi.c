/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <stddef.h>
#include <ufi_rank_utils.h>
#include <dpu_hw_description.h>

#include <ufi/ufi_bit_config.h>
#include <ufi/ufi_ci.h>
#include <ufi/ufi_ci_commands.h>
#include <ufi/ufi.h>

static u8 next_ci(u8 ci_mask, u8 current_ci, u8 nr_cis)
{
	do {
		current_ci++;
	} while ((current_ci < nr_cis) && !CI_MASK_ON(ci_mask, current_ci));

	return current_ci;
}

#define for_each_ci(i, nr_cis, mask)                                           \
	for (i = next_ci(mask, -1, nr_cis); i < nr_cis;                        \
	     i = next_ci(mask, i, nr_cis))

#define IRAM_TIMING_SAFE_VALUE 0x02
#define IRAM_TIMING_NORMAL_VALUE 0x04
#define IRAM_TIMING_AGGRESSIVE_VALUE 0x05

#define RFRAM_TIMING_SAFE_VALUE 0x01
#define RFRAM_TIMING_NORMAL_VALUE 0x04
#define RFRAM_TIMING_AGGRESSIVE_VALUE 0x05

#define WRAM_TIMING_SAFE_VALUE 0x02
#define WRAM_TIMING_NORMAL_VALUE 0x05
#define WRAM_TIMING_AGGRESSIVE_VALUE 0x06

static u32 UFI_exec_write_structure(struct dpu_rank_t *rank, u8 ci_mask,
				    u64 structure);
static u32 UFI_exec_write_structures(struct dpu_rank_t *rank, u8 ci_mask,
				     u64 *structures);
static u32 UFI_exec_void_frame(struct dpu_rank_t *rank, u8 ci_mask,
			       u64 structure, u64 frame);
static u32 UFI_exec_void_frames(struct dpu_rank_t *rank, u8 ci_mask,
				u64 *structures, u64 *frames);
static u32 UFI_exec_8bit_frame(struct dpu_rank_t *rank, u8 ci_mask,
			       u64 structure, u64 frame, u8 *results);
static u32 UFI_exec_32bit_frame(struct dpu_rank_t *rank, u8 ci_mask,
				u64 structure, u64 frame, u32 *results);

__API_SYMBOL__ u32 ufi_byte_order(struct dpu_rank_t *rank, u8 ci_mask,
				  u64 *results)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	ci_prepare_mask(cmds, ci_mask, CI_BYTE_ORDER);
	FF(ci_exec_byte_discovery(rank, cmds, results));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_soft_reset(struct dpu_rank_t *rank, u8 ci_mask,
				  u8 clock_division, u8 cycle_accurate)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	ci_prepare_mask(cmds, ci_mask,
			CI_SOFTWARE_RESET(clock_division, cycle_accurate));
	FF(ci_exec_reset_cmd(rank, cmds));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_bit_config(struct dpu_rank_t *rank, u8 ci_mask,
				  const struct dpu_bit_config *config,
				  u32 *results)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	struct dpu_bit_config *bit_config =
		&(GET_DESC_HW(rank)->dpu.pcb_transformation);

	u64 cmd;
	if (config == NULL) {
		cmd = CI_BIT_ORDER(0, 0, 0, 0);
	} else {
		/* Important: no shuffling boxes, so we have to software-shuffle correctly for the parameters of BIT_ORDER
         * to be understood...We want DPU to read a, so we send m(a):
         * CPU: a       -> mm(a)        DPU
         * CPU: m(a)    -> mm(m(a)) = a DPU
         * But no need to shuffle nibble swap since it is either 0 or 0xFFFFFFFF and nothing to do with bit_order_result.
         */
		cmd = CI_BIT_ORDER(
			config->nibble_swap, config->stutter,
			dpu_bit_config_dpu2cpu(bit_config, config->cpu2dpu),
			dpu_bit_config_dpu2cpu(bit_config, config->dpu2cpu));
	}

	ci_prepare_mask(cmds, ci_mask, cmd);

	if (results == NULL) {
		FF(ci_exec_void_cmd(rank, cmds));
	} else {
		FF(ci_exec_32bit_cmd(rank, cmds, results));
	}

end:
	return status;
}

/* Symbol used by lldb to call the 'ufi_identity' function when detaching from a DPU.
 * It allows the debuggee to consume the color restore by the 'lldb-server-dpu'.
 */
__attribute__((used)) u32 lldb_dummy_results[DPU_MAX_NR_CIS];

__API_SYMBOL__ u32 ufi_identity(struct dpu_rank_t *rank, u8 ci_mask,
				u32 *results)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_bit_config *bit_config =
		&(GET_DESC_HW(rank)->dpu.pcb_transformation);
	u8 each_ci;

	ci_prepare_mask(cmds, ci_mask, CI_IDENTITY);
	FF(ci_exec_32bit_cmd(rank, cmds, results));

	/* No shuffling boxes, so the result must be software-shuffled */
	for_each_ci (each_ci, nr_cis, ci_mask) {
		results[each_ci] =
			dpu_bit_config_cpu2dpu(bit_config, results[each_ci]);
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_thermal_config(struct dpu_rank_t *rank, u8 ci_mask,
				      enum dpu_temperature threshold)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	ci_prepare_mask(cmds, ci_mask, CI_THERMAL_CONFIG(threshold));
	FF(ci_exec_void_cmd(rank, cmds));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_cis(struct dpu_rank_t *rank, u8 *ci_mask)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;
	u8 mask = *ci_mask;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		if (!ci_info->enabled_dpus)
			mask &= ~CI_MASK_ONE(each_ci);
	}

	*ci_mask = mask;

	return status;
}

__API_SYMBOL__ u32 ufi_select_all(struct dpu_rank_t *rank, u8 *ci_mask)
{
	u32 status = DPU_OK;
	u64 structs[DPU_MAX_NR_CIS];
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	bool do_select = false;
	u8 each_ci;

	/* Initialize structs with default value (CI_EMPTY) */
	ci_prepare_mask(structs, 0, 0);

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		if (ci_info->all_dpus_are_enabled) {
			if (ci_info->slice_target.type !=
			    DPU_SLICE_TARGET_ALL) {
				structs[each_ci] = CI_SELECT_ALL_STRUCT;
				cmds[each_ci] = CI_SELECT_ALL_FRAME;
				ci_info->slice_target.type =
					DPU_SLICE_TARGET_ALL;
				do_select = true;
			}
		} else {
			if ((ci_info->slice_target.type !=
			     DPU_SLICE_TARGET_GROUP) ||
			    (ci_info->slice_target.group_id !=
			     DPU_ENABLED_GROUP)) {
				if (ci_info->enabled_dpus != 0) {
					structs[each_ci] =
						CI_SELECT_GROUP_STRUCT;
					cmds[each_ci] = CI_SELECT_GROUP_FRAME(
						DPU_ENABLED_GROUP);
					ci_info->slice_target.type =
						DPU_SLICE_TARGET_GROUP;
					ci_info->slice_target.group_id =
						DPU_ENABLED_GROUP;
					do_select = true;
				} else {
					mask &= ~CI_MASK_ONE(each_ci);
				}
			}
		}
	}

	if (do_select) {
		FF(UFI_exec_void_frames(rank, mask, structs, cmds));
	}

	*ci_mask = mask;

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_all_uncached(struct dpu_rank_t *rank, u8 *ci_mask)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		ci_info->slice_target.type = DPU_SLICE_TARGET_ALL;
	}
	FF(UFI_exec_void_frame(rank, mask, CI_SELECT_ALL_STRUCT,
			       CI_SELECT_ALL_FRAME));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_all_even_disabled(struct dpu_rank_t *rank,
						u8 *ci_mask)
{
	u32 status = DPU_OK;
	u64 structs[DPU_MAX_NR_CIS];
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	bool do_select = false;
	u8 each_ci;

	/* Initialize structs with default value (CI_EMPTY) */
	ci_prepare_mask(structs, 0, 0);

	/* Note: if there are disabled DPUs, the next ufi_select_all will
     * change the selection to SELECT_GROUP, so no need to clean after this
     * function is called.
     */
	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		if (ci_info->enabled_dpus != 0) {
			if (ci_info->slice_target.type !=
			    DPU_SLICE_TARGET_ALL) {
				structs[each_ci] = CI_SELECT_ALL_STRUCT;
				cmds[each_ci] = CI_SELECT_ALL_FRAME;
				ci_info->slice_target.type =
					DPU_SLICE_TARGET_ALL;
				do_select = true;
			}
		} else
			mask &= ~CI_MASK_ONE(each_ci);
	}

	if (do_select) {
		FF(UFI_exec_void_frames(rank, mask, structs, cmds));
	}

	*ci_mask = mask;

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_group(struct dpu_rank_t *rank, u8 *ci_mask,
				    u8 group)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	bool do_select = false;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		/*
		 * By construction, to set the group of a DPU, we need to select
		 * the DPU/all enabled DPUs, which means the group can only contain enabled DPUs.
		 * But we might select a group that does not contain any DPU, which should not
		 * happen if all the DPUs of a CI were disabled, so first check that the
		 * group is not empty.
		 */
		if (ci_info->dpus_per_group[group]) {
			if ((ci_info->slice_target.type !=
			     DPU_SLICE_TARGET_GROUP) ||
			    (ci_info->slice_target.group_id != group)) {
				ci_info->slice_target.type =
					DPU_SLICE_TARGET_GROUP;
				ci_info->slice_target.group_id = group;
				do_select = true;
			}
		} else {
			mask &= ~CI_MASK_ONE(each_ci);
		}
	}

	if (do_select) {
		FF(UFI_exec_void_frame(rank, mask, CI_SELECT_GROUP_STRUCT,
				       CI_SELECT_GROUP_FRAME(group)));
	}

	*ci_mask = mask;

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_group_uncached(struct dpu_rank_t *rank,
					     u8 *ci_mask, u8 group)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		ci_info->slice_target.type = DPU_SLICE_TARGET_GROUP;
		ci_info->slice_target.group_id = group;
	}

	FF(UFI_exec_void_frame(rank, mask, CI_SELECT_GROUP_STRUCT,
			       CI_SELECT_GROUP_FRAME(group)));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_dpu(struct dpu_rank_t *rank, u8 *ci_mask, u8 dpu)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	bool do_select = false;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		if (ci_info->enabled_dpus & (1 << dpu)) {
			if ((ci_info->slice_target.type !=
			     DPU_SLICE_TARGET_DPU) ||
			    (ci_info->slice_target.dpu_id != dpu)) {
				ci_info->slice_target.type =
					DPU_SLICE_TARGET_DPU;
				ci_info->slice_target.dpu_id = dpu;
				do_select = true;
			}
		} else {
			mask &= ~CI_MASK_ONE(each_ci);
		}
	}

	if (do_select) {
		FF(UFI_exec_void_frame(rank, mask, CI_SELECT_DPU_STRUCT,
				       CI_SELECT_DPU_FRAME(dpu)));
	}

	*ci_mask = mask;

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_dpu_uncached(struct dpu_rank_t *rank, u8 *ci_mask,
					   u8 dpu)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		ci_info->slice_target.type = DPU_SLICE_TARGET_DPU;
		ci_info->slice_target.dpu_id = dpu;
	}

	FF(UFI_exec_void_frame(rank, mask, CI_SELECT_DPU_STRUCT,
			       CI_SELECT_DPU_FRAME(dpu)));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_select_dpu_even_disabled(struct dpu_rank_t *rank,
						u8 *ci_mask, u8 dpu)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	struct dpu_configuration_slice_info_t *info =
		GET_CI_CONTEXT(rank)->slice_info;

	u8 mask = *ci_mask;
	bool do_select = false;
	u8 each_ci;

	for_each_ci (each_ci, nr_cis, mask) {
		struct dpu_configuration_slice_info_t *ci_info = &info[each_ci];

		if (ci_info->enabled_dpus) {
			if ((ci_info->slice_target.type !=
			     DPU_SLICE_TARGET_DPU) ||
			    (ci_info->slice_target.dpu_id != dpu)) {
				ci_info->slice_target.type =
					DPU_SLICE_TARGET_DPU;
				ci_info->slice_target.dpu_id = dpu;
				do_select = true;
			}
		} else {
			mask &= ~CI_MASK_ONE(each_ci);
		}
	}

	if (do_select) {
		FF(UFI_exec_void_frame(rank, mask, CI_SELECT_DPU_STRUCT,
				       CI_SELECT_DPU_FRAME(dpu)));
	}

	*ci_mask = mask;

end:
	return status;
}

__API_SYMBOL__ u32 ufi_write_group(struct dpu_rank_t *rank, u8 ci_mask,
				   u8 group)
{
	u32 status;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci, each_group;

	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_configuration_slice_info_t *info =
			&(GET_CI_CONTEXT(rank)->slice_info[each_ci]);
		u8 dpu_mask;

		FF(ci_fill_selected_dpu_mask(rank, each_ci, &dpu_mask));

		for (each_group = 0; each_group < DPU_MAX_NR_GROUPS;
		     ++each_group) {
			info->dpus_per_group[each_group] &= ~dpu_mask;
		}

		info->dpus_per_group[group] |= dpu_mask;
	}

	FF(UFI_exec_void_frame(rank, ci_mask, CI_WRITE_GROUP_STRUCT,
			       CI_WRITE_GROUP_FRAME(group)));
end:
	return status;
}

__API_SYMBOL__ u32 ufi_carousel_config(struct dpu_rank_t *rank, u8 ci_mask,
				       const struct dpu_carousel_config *config)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	ci_prepare_mask(cmds, ci_mask,
			CI_CMD_DURATION_FUTUR(config->cmd_duration));
	FF(ci_exec_void_cmd(rank, cmds));
	FF(UFI_exec_void_frame(
		rank, ci_mask, CI_CMD_BUS_CONFIG_DURATION_STRUCT,
		CI_CMD_BUS_CONFIG_DURATION_FRAME(config->cmd_duration)));
	FF(UFI_exec_void_frame(
		rank, ci_mask, CI_CMD_BUS_CONFIG_SAMPLING_STRUCT,
		CI_CMD_BUS_CONFIG_SAMPLING_FRAME(config->cmd_sampling)));
	FF(UFI_exec_void_frame(rank, ci_mask, CI_CMD_BUS_SYNC_STRUCT,
			       CI_CMD_BUS_SYNC_FRAME));

	ci_prepare_mask(cmds, ci_mask,
			CI_RES_DURATION_FUTUR(config->res_duration));
	FF(ci_exec_void_cmd(rank, cmds));
	ci_prepare_mask(cmds, ci_mask,
			CI_RES_SAMPLING_FUTUR(config->res_sampling));
	FF(ci_exec_void_cmd(rank, cmds));
	FF(UFI_exec_void_frame(
		rank, ci_mask, CI_RES_BUS_CONFIG_DURATION_STRUCT,
		CI_RES_BUS_CONFIG_DURATION_FRAME(config->res_duration)));
	FF(UFI_exec_void_frame(rank, ci_mask, CI_RES_BUS_SYNC_STRUCT,
			       CI_RES_BUS_SYNC_FRAME));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_clear_debug_replace(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask,
				   CI_DEBUG_STD_REPLACE_CLEAR_STRUCT,
				   CI_DEBUG_STD_REPLACE_CLEAR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_poison(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_POISON_FAULT_CLEAR_STRUCT,
				   CI_POISON_FAULT_CLEAR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_bkp(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_BKP_FAULT_CLEAR_STRUCT,
				   CI_BKP_FAULT_CLEAR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_dma(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask,
				   CI_DMA_FAULT_READ_AND_CLR_STRUCT,
				   CI_DMA_FAULT_READ_AND_CLR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_mem(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask,
				   CI_MEM_FAULT_READ_AND_CLR_STRUCT,
				   CI_MEM_FAULT_READ_AND_CLR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_dpu(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_DPU_FAULT_STATE_CLR_STRUCT,
				   CI_DPU_FAULT_STATE_CLR_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_fault_intercept(struct dpu_rank_t *rank,
					     u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_BKP_FAULT_READ_STRUCT,
				   CI_BKP_FAULT_READ_FRAME);
}

__API_SYMBOL__ u32 ufi_clear_run_bit(struct dpu_rank_t *rank, u8 ci_mask,
				     u8 run_bit, u8 *previous)
{
	return previous == NULL ?
			     UFI_exec_void_frame(rank, ci_mask,
					   CI_THREAD_CLR_RUN_STRUCT,
					   CI_THREAD_CLR_RUN_FRAME(run_bit)) :
			     UFI_exec_8bit_frame(rank, ci_mask,
					   CI_THREAD_CLR_RUN_STRUCT,
					   CI_THREAD_CLR_RUN_FRAME(run_bit),
					   previous);
}

__API_SYMBOL__ u32 ufi_iram_repair_config(struct dpu_rank_t *rank, u8 ci_mask,
					  struct dpu_repair_config **config)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	FF(UFI_exec_write_structure(rank, ci_mask,
				    CI_IREPAIR_CONFIG_AB_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_IREPAIR_CONFIG_AB_FRAME(ci_config->AB_msbs,
							   ci_config->A_lsbs,
							   ci_config->B_lsbs);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	FF(UFI_exec_write_structure(rank, ci_mask,
				    CI_IREPAIR_CONFIG_CD_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_IREPAIR_CONFIG_CD_FRAME(ci_config->CD_msbs,
							   ci_config->C_lsbs,
							   ci_config->D_lsbs);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	FF(UFI_exec_write_structure(rank, ci_mask,
				    CI_IREPAIR_CONFIG_OE_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] =
			CI_IREPAIR_CONFIG_OE_FRAME(ci_config->odd_index,
						   ci_config->even_index,
						   IRAM_TIMING_SAFE_VALUE);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	FF(UFI_exec_void_frame(
		rank, ci_mask, CI_REGISTER_FILE_TIMING_CONFIG_STRUCT,
		CI_REGISTER_FILE_TIMING_CONFIG_FRAME(RFRAM_TIMING_SAFE_VALUE)));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_wram_repair_config(struct dpu_rank_t *rank, u8 ci_mask,
					  u8 bank,
					  struct dpu_repair_config **config)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	u8 b0 = 0x60 | (bank << 2);
	u8 b4 = 0x60;
	u8 b5 = 0x40;

	FF(UFI_exec_write_structure(rank, ci_mask, CI_DMA_CTRL_WRITE_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_DMA_CTRL_WRITE_FRAME(
			b0, 0x60, 0x60 | (ci_config->even_index >> 1),
			0x60 | (ci_config->odd_index >> 1), b4, b5);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_DMA_CTRL_WRITE_FRAME(
			b0, 0x61, 0x60 | (ci_config->AB_msbs >> 4),
			0x60 | (ci_config->AB_msbs & 0xf), b4, b5);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_DMA_CTRL_WRITE_FRAME(
			b0, 0x62, 0x60 | ci_config->A_lsbs,
			0x60 | ci_config->B_lsbs, b4, b5);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_DMA_CTRL_WRITE_FRAME(
			b0, 0x63, 0x60 | (ci_config->CD_msbs >> 4),
			0x60 | (ci_config->CD_msbs & 0xf), b4, b5);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		struct dpu_repair_config *ci_config = config[each_ci];
		cmds[each_ci] = CI_DMA_CTRL_WRITE_FRAME(
			b0, 0x64, 0x60 | ci_config->C_lsbs,
			0x60 | ci_config->D_lsbs, b4, b5);
	}
	FF(ci_exec_void_cmd(rank, cmds));

	FF(UFI_exec_void_frame(rank, ci_mask, CI_DMA_CTRL_WRITE_STRUCT,
			       CI_DMA_CTRL_WRITE_FRAME(
				       b0, 0x65, 0x60,
				       0x60 | WRAM_TIMING_SAFE_VALUE, b4, b5)));
	FF(ufi_clear_dma_ctrl(rank, ci_mask));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_get_pc_mode(struct dpu_rank_t *rank, u8 ci_mask,
				   enum dpu_pc_mode *pc_mode)
{
	u32 status;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 modes[DPU_MAX_NR_CIS];
	u8 each_ci;

	FF(UFI_exec_8bit_frame(rank, ci_mask, CI_PC_MODE_READ_STRUCT,
			       CI_PC_MODE_READ_FRAME, modes));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		pc_mode[each_ci] = modes[each_ci];
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_set_pc_mode(struct dpu_rank_t *rank, u8 ci_mask,
				   const enum dpu_pc_mode *pc_mode)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	FF(UFI_exec_write_structure(rank, ci_mask, CI_PC_MODE_WRITE_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		cmds[each_ci] = CI_PC_MODE_WRITE_FRAME(pc_mode[each_ci]);
	}
	FF(ci_exec_void_cmd(rank, cmds));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_set_stack_direction(struct dpu_rank_t *rank, u8 ci_mask,
					   const bool *direction, u8 *previous)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	// We can do that because CI_STACK_UP_READ_AND_SET_STRUCT == CI_STACK_UP_READ_AND_CLR_STRUCT
	FF(UFI_exec_write_structure(rank, ci_mask,
				    CI_STACK_UP_READ_AND_SET_STRUCT));
	for_each_ci (each_ci, nr_cis, ci_mask) {
		cmds[each_ci] = direction[each_ci] ?
					      CI_STACK_UP_READ_AND_SET_FRAME :
					      CI_STACK_UP_READ_AND_CLR_FRAME;
	}

	if (previous == NULL) {
		FF(ci_exec_void_cmd(rank, cmds));
	} else {
		FF(ci_exec_8bit_cmd(rank, cmds, previous));
	}

end:
	return status;
}

static u64 ci_dma_ctrl_write_frame(u8 address, u8 data)
{
	u8 b0 = 0x60 | (((address) >> 4) & 0x0F);
	u8 b1 = 0x60 | (((address) >> 0) & 0x0F);
	u8 b2 = 0x60 | (((data) >> 4) & 0x0F);
	u8 b3 = 0x60 | (((data) >> 0) & 0x0F);
	u8 b4 = 0x60;
	u8 b5 = 0x20;

	return CI_DMA_CTRL_WRITE_FRAME(b0, b1, b2, b3, b4, b5);
}

__API_SYMBOL__ u32 ufi_write_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask,
				      u8 address, u8 data)
{
	u32 status;

	FF(UFI_exec_void_frame(rank, ci_mask, CI_DMA_CTRL_WRITE_STRUCT,
			       ci_dma_ctrl_write_frame(address, data)));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_write_dma_ctrl_datas(struct dpu_rank_t *rank, u8 ci_mask,
					    u8 address, u8 *datas)
{
	u32 status;
	u64 *frames = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	FF(UFI_exec_write_structure(rank, ci_mask, CI_DMA_CTRL_WRITE_STRUCT));

	for_each_ci (each_ci, nr_cis, ci_mask) {
		frames[each_ci] =
			ci_dma_ctrl_write_frame(address, datas[each_ci]);
	}

	FF(ci_exec_void_cmd(rank, frames));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_read_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask,
				     u8 *data)
{
	return UFI_exec_8bit_frame(rank, ci_mask, CI_DMA_CTRL_READ_STRUCT,
				   CI_DMA_CTRL_READ_FRAME, data);
}

__API_SYMBOL__ u32 ufi_clear_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_DMA_CTRL_CLEAR_STRUCT,
				   CI_DMA_CTRL_CLEAR_FRAME);
}

__API_SYMBOL__ u32 ufi_iram_write(struct dpu_rank_t *rank, u8 ci_mask,
				  u64 **src, u16 offset, u16 len)
{
	u32 status = DPU_OK;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u16 each_address;
	u8 each_ci;

	for (each_address = 0; each_address < len; ++each_address) {
		u16 addr = offset + each_address;

		FF(UFI_exec_write_structure(
			rank, ci_mask, CI_IRAM_WRITE_INSTRUCTION_STRUCT(addr)));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			cmds[each_ci] = CI_IRAM_WRITE_INSTRUCTION_FRAME(
				src[each_ci][each_address], addr);
		}

		FF(ci_exec_wait_mask_cmd(rank, cmds));
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_iram_read(struct dpu_rank_t *rank, u8 ci_mask, u64 **dst,
				 u16 offset, u16 len)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 bytes[DPU_MAX_NR_CIS];
	u16 each_address;
	u8 each_ci;

	for (each_address = 0; each_address < len; ++each_address) {
		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE0_STRUCT,
			CI_IRAM_READ_BYTE0_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[0] =
				bytes[each_ci];
		}

		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE1_STRUCT,
			CI_IRAM_READ_BYTE1_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[1] =
				bytes[each_ci];
		}

		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE2_STRUCT,
			CI_IRAM_READ_BYTE2_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[2] =
				bytes[each_ci];
		}

		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE3_STRUCT,
			CI_IRAM_READ_BYTE3_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[3] =
				bytes[each_ci];
		}

		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE4_STRUCT,
			CI_IRAM_READ_BYTE4_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[4] =
				bytes[each_ci];
		}

		FF(UFI_exec_8bit_frame(
			rank, ci_mask, CI_IRAM_READ_BYTE5_STRUCT,
			CI_IRAM_READ_BYTE5_FRAME(offset + each_address),
			bytes));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			((u8 *)(&dst[each_ci][each_address]))[5] =
				bytes[each_ci];
			((u8 *)(&dst[each_ci][each_address]))[6] = 0;
			((u8 *)(&dst[each_ci][each_address]))[7] = 0;
		}
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_wram_write(struct dpu_rank_t *rank, u8 ci_mask,
				  u32 **src, u16 offset, u16 len)
{
	u32 status = DPU_OK;
	u64 *cmds = GET_CMDS(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u16 each_address;
	u8 each_ci;

	for (each_address = 0; each_address < len; ++each_address) {
		u16 addr = offset + each_address;

		FF(UFI_exec_write_structure(rank, ci_mask,
					    CI_WRAM_WRITE_WORD_STRUCT(addr)));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			cmds[each_ci] = CI_WRAM_WRITE_WORD_FRAME(
				src[each_ci][each_address], addr);
		}

		FF(ci_exec_wait_mask_cmd(rank, cmds));
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_wram_read(struct dpu_rank_t *rank, u8 ci_mask, u32 **dst,
				 u16 offset, u16 len)
{
	u32 status = DPU_OK;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u32 words[DPU_MAX_NR_CIS];
	u16 each_address;
	u8 each_ci;

	for (each_address = 0; each_address < len; ++each_address) {
		FF(UFI_exec_32bit_frame(
			rank, ci_mask, CI_WRAM_READ_WORD_STRUCT,
			CI_WRAM_READ_WORD_FRAME(offset + each_address), words));

		for_each_ci (each_ci, nr_cis, ci_mask) {
			dst[each_ci][each_address] = words[each_ci];
		}
	}

end:
	return status;
}

/* This function should *not* be used directly as it does not manage mux,
 * use dpu_thread_boot_safe instead.
 */
__API_SYMBOL__ u32 ufi_thread_boot(struct dpu_rank_t *rank, u8 ci_mask,
				   u8 thread, u8 *previous)
{
	return previous == NULL ?
			     UFI_exec_void_frame(rank, ci_mask, CI_THREAD_BOOT_STRUCT,
					   CI_THREAD_BOOT_FRAME(thread)) :
			     UFI_exec_8bit_frame(rank, ci_mask, CI_THREAD_BOOT_STRUCT,
					   CI_THREAD_BOOT_FRAME(thread),
					   previous);
}

__API_SYMBOL__ u32 ufi_thread_resume(struct dpu_rank_t *rank, u8 ci_mask,
				     u8 thread, u8 *previous)
{
	return previous == NULL ?
			     UFI_exec_void_frame(rank, ci_mask,
					   CI_THREAD_RESUME_STRUCT,
					   CI_THREAD_RESUME_FRAME(thread)) :
			     UFI_exec_8bit_frame(rank, ci_mask,
					   CI_THREAD_RESUME_STRUCT,
					   CI_THREAD_RESUME_FRAME(thread),
					   previous);
}

__API_SYMBOL__ u32 ufi_read_run_bit(struct dpu_rank_t *rank, u8 ci_mask,
				    u8 run_bit, u8 *running)
{
	return UFI_exec_8bit_frame(rank, ci_mask, CI_THREAD_READ_RUN_STRUCT,
				   CI_THREAD_READ_RUN_FRAME(run_bit), running);
}

__API_SYMBOL__ u32 ufi_read_dpu_run(struct dpu_rank_t *rank, u8 ci_mask,
				    u8 *run)
{
	return UFI_exec_8bit_frame(rank, ci_mask, CI_DPU_RUN_STATE_READ_STRUCT,
				   CI_DPU_RUN_STATE_READ_FRAME, run);
}

__API_SYMBOL__ u32 ufi_read_dpu_fault(struct dpu_rank_t *rank, u8 ci_mask,
				      u8 *fault)
{
	return UFI_exec_8bit_frame(rank, ci_mask,
				   CI_DPU_FAULT_STATE_READ_STRUCT,
				   CI_DPU_FAULT_STATE_READ_FRAME, fault);
}

__API_SYMBOL__ u32 ufi_set_dpu_fault_and_step(struct dpu_rank_t *rank,
					      u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask,
				   CI_DPU_FAULT_STATE_SET_AND_STEP_STRUCT,
				   CI_DPU_FAULT_STATE_SET_AND_STEP_FRAME);
}

__API_SYMBOL__ u32 ufi_set_bkp_fault(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_BKP_FAULT_SET_STRUCT,
				   CI_BKP_FAULT_SET_FRAME);
}

__API_SYMBOL__ u32 ufi_set_poison_fault(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_POISON_FAULT_SET_STRUCT,
				   CI_POISON_FAULT_SET_FRAME);
}

__API_SYMBOL__ u32 ufi_read_bkp_fault(struct dpu_rank_t *rank, u8 ci_mask,
				      u8 *fault)
{
	return fault == NULL ?
			     UFI_exec_void_frame(rank, ci_mask,
					   CI_BKP_FAULT_READ_STRUCT,
					   CI_BKP_FAULT_READ_FRAME) :
			     UFI_exec_8bit_frame(rank, ci_mask,
					   CI_BKP_FAULT_READ_STRUCT,
					   CI_BKP_FAULT_READ_FRAME, fault);
}

__API_SYMBOL__ u32 ufi_read_poison_fault(struct dpu_rank_t *rank, u8 ci_mask,
					 u8 *fault)
{
	return UFI_exec_8bit_frame(rank, ci_mask, CI_POISON_FAULT_READ_STRUCT,
				   CI_POISON_FAULT_READ_FRAME, fault);
}

__API_SYMBOL__ u32 ufi_read_and_clear_dma_fault(struct dpu_rank_t *rank,
						u8 ci_mask, u8 *fault)
{
	return fault == NULL ?
			     UFI_exec_void_frame(rank, ci_mask,
					   CI_DMA_FAULT_READ_AND_CLR_STRUCT,
					   CI_DMA_FAULT_READ_AND_CLR_FRAME) :
			     UFI_exec_8bit_frame(rank, ci_mask,
					   CI_DMA_FAULT_READ_AND_CLR_STRUCT,
					   CI_DMA_FAULT_READ_AND_CLR_FRAME,
					   fault);
}

__API_SYMBOL__ u32 ufi_read_and_clear_mem_fault(struct dpu_rank_t *rank,
						u8 ci_mask, u8 *fault)
{
	return fault == NULL ?
			     UFI_exec_void_frame(rank, ci_mask,
					   CI_MEM_FAULT_READ_AND_CLR_STRUCT,
					   CI_MEM_FAULT_READ_AND_CLR_FRAME) :
			     UFI_exec_8bit_frame(rank, ci_mask,
					   CI_MEM_FAULT_READ_AND_CLR_STRUCT,
					   CI_MEM_FAULT_READ_AND_CLR_FRAME,
					   fault);
}

__API_SYMBOL__ u32 ufi_read_bkp_fault_thread_index(struct dpu_rank_t *rank,
						   u8 ci_mask, u8 *thread)
{
	return UFI_exec_8bit_frame(rank, ci_mask,
				   CI_BKP_FAULT_THREAD_INDEX_READ_STRUCT,
				   CI_BKP_FAULT_THREAD_INDEX_READ_FRAME,
				   thread);
}

__API_SYMBOL__ u32 ufi_read_dma_fault_thread_index(struct dpu_rank_t *rank,
						   u8 ci_mask, u8 *thread)
{
	return UFI_exec_8bit_frame(rank, ci_mask,
				   CI_DMA_FAULT_THREAD_INDEX_READ_STRUCT,
				   CI_DMA_FAULT_THREAD_INDEX_READ_FRAME,
				   thread);
}

__API_SYMBOL__ u32 ufi_read_mem_fault_thread_index(struct dpu_rank_t *rank,
						   u8 ci_mask, u8 *thread)
{
	return UFI_exec_8bit_frame(rank, ci_mask,
				   CI_MEM_FAULT_THREAD_INDEX_READ_STRUCT,
				   CI_MEM_FAULT_THREAD_INDEX_READ_FRAME,
				   thread);
}

__API_SYMBOL__ u32 ufi_set_mram_mux(struct dpu_rank_t *rank, u8 ci_mask,
				    dpu_ci_bitfield_t ci_mux_pos)
{
	u32 status;
	u8 mux_value[DPU_MAX_NR_CIS];
	u8 each_ci;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;

	for_each_ci (each_ci, nr_cis, ci_mask) {
		mux_value[each_ci] = (ci_mux_pos & (1 << each_ci)) ? 0 : 1;
	}

	FF(ufi_write_dma_ctrl_datas(rank, ci_mask, 0x80, mux_value));
	FF(ufi_write_dma_ctrl(rank, ci_mask, 0x81, 0));
	FF(ufi_write_dma_ctrl_datas(rank, ci_mask, 0x82, mux_value));
	FF(ufi_write_dma_ctrl_datas(rank, ci_mask, 0x84, mux_value));

	FF(ufi_clear_dma_ctrl(rank, ci_mask));

end:
	return status;
}

__API_SYMBOL__ u32 ufi_debug_replace_stop(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask,
				   CI_DEBUG_STD_REPLACE_STOP_ENABLED_STRUCT,
				   CI_DEBUG_STD_REPLACE_STOP_ENABLED_FRAME);
}

__API_SYMBOL__ u32 ufi_debug_pc_read(struct dpu_rank_t *rank, u8 ci_mask,
				     u16 *pcs)
{
	u32 status;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u32 iram_size = GET_DESC_HW(rank)->memories.iram_size;

	u8 lsb[DPU_MAX_NR_CIS];
	u8 msb[DPU_MAX_NR_CIS];

	u8 each_ci;
	u8 nr_bits_in_lsb;

	FF(UFI_exec_8bit_frame(rank, ci_mask, CI_PC_LSB_READ_STRUCT,
			       CI_PC_LSB_READ_FRAME, lsb));
	FF(UFI_exec_8bit_frame(rank, ci_mask, CI_PC_MSB_READ_STRUCT,
			       CI_PC_MSB_READ_FRAME, msb));

	switch (iram_size) {
	case 1 << 12:
		nr_bits_in_lsb = 6;
		break;
	case 1 << 13:
		nr_bits_in_lsb = 7;
		break;
	case 1 << 14:
		nr_bits_in_lsb = 7;
		break;
	case 1 << 15:
		nr_bits_in_lsb = 8;
		break;
	case 1 << 16:
		nr_bits_in_lsb = 8;
		break;
	default:
		nr_bits_in_lsb = 8;
		break;
	}

	for_each_ci (each_ci, nr_cis, ci_mask) {
		pcs[each_ci] = (lsb[each_ci] & ((1 << nr_bits_in_lsb) - 1)) |
			       (msb[each_ci] << nr_bits_in_lsb);
	}

end:
	return status;
}

__API_SYMBOL__ u32 ufi_debug_pc_sample(struct dpu_rank_t *rank, u8 ci_mask)
{
	return UFI_exec_void_frame(rank, ci_mask, CI_DEBUG_STD_SAMPLE_PC_STRUCT,
				   CI_DEBUG_STD_SAMPLE_PC_FRAME);
}

static u32 UFI_exec_write_structure(struct dpu_rank_t *rank, u8 ci_mask,
				    u64 structure)
{
	u32 status = DPU_OK;
	u64 *cmds = GET_CMDS(rank);
	struct dpu_control_interface_context *ci = GET_CI_CONTEXT(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	bool do_write_structure = false;
	for_each_ci (each_ci, nr_cis, ci_mask) {
		if (ci->slice_info[each_ci].structure_value != structure) {
			ci->slice_info[each_ci].structure_value = structure;
			do_write_structure = true;
		}
	}

	if (do_write_structure) {
		ci_prepare_mask(cmds, ci_mask, structure);
		FF(ci_exec_void_cmd(rank, cmds));
	}

end:
	return status;
}

static u32 UFI_exec_write_structures(struct dpu_rank_t *rank, u8 ci_mask,
				     u64 *structures)
{
	u32 status = DPU_OK;
	struct dpu_control_interface_context *ci = GET_CI_CONTEXT(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	bool do_write_structure = false;
	for_each_ci (each_ci, nr_cis, ci_mask) {
		if ((structures[each_ci] != CI_EMPTY) &&
		    (ci->slice_info[each_ci].structure_value !=
		     structures[each_ci])) {
			ci->slice_info[each_ci].structure_value =
				structures[each_ci];
			do_write_structure = true;
		}
	}

	if (do_write_structure) {
		FF(ci_exec_void_cmd(rank, structures));
	}

end:
	return status;
}

static u32 UFI_exec_void_frame(struct dpu_rank_t *rank, u8 ci_mask,
			       u64 structure, u64 frame)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	FF(UFI_exec_write_structure(rank, ci_mask, structure));
	ci_prepare_mask(cmds, ci_mask, frame);
	FF(ci_exec_void_cmd(rank, cmds));

	return DPU_OK;
end:
	return status;
}

static u32 UFI_exec_void_frames(struct dpu_rank_t *rank, u8 ci_mask,
				u64 *structures, u64 *frames)
{
	u32 status;
	FF(UFI_exec_write_structures(rank, ci_mask, structures));
	FF(ci_exec_void_cmd(rank, frames));

end:
	return status;
}

static u32 UFI_exec_8bit_frame(struct dpu_rank_t *rank, u8 ci_mask,
			       u64 structure, u64 frame, u8 *results)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	FF(UFI_exec_write_structure(rank, ci_mask, structure));
	ci_prepare_mask(cmds, ci_mask, frame);
	FF(ci_exec_8bit_cmd(rank, cmds, results));

end:
	return status;
}

static u32 UFI_exec_32bit_frame(struct dpu_rank_t *rank, u8 ci_mask,
				u64 structure, u64 frame, u32 *results)
{
	u32 status;
	u64 *cmds = GET_CMDS(rank);

	FF(UFI_exec_write_structure(rank, ci_mask, structure));
	ci_prepare_mask(cmds, ci_mask, frame);
	FF(ci_exec_32bit_cmd(rank, cmds, results));

end:
	return status;
}
