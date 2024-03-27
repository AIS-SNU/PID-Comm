/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <dpu_types.h>
#include <ufi/ufi_ci.h>
#include <ufi/ufi_ci_types.h>
#include <ufi/ufi_ci_commands.h>
#include <ufi_rank_utils.h>

#define NB_RETRY_FOR_VALID_RESULT 100

static void invert_color(struct dpu_rank_t *rank, u8 ci_mask);
static u8 compute_ci_mask(struct dpu_rank_t *rank, const u64 *commands);
static u32 compute_masks(struct dpu_rank_t *rank, const u64 *commands,
			 u64 *masks, u64 *expected, u8 *cis,
			 bool add_select_mask, bool *is_done);
static u32 exec_cmd(struct dpu_rank_t *rank, u64 *commands,
		    bool add_select_mask);
static bool determine_if_byte_discoveries_are_finished(struct dpu_rank_t *rank,
						       const u64 *data,
						       u8 ci_mask);
static bool
determine_if_commands_are_finished(struct dpu_rank_t *rank, const u64 *data,
				   const u64 *expected, const u64 *result_masks,
				   u8 expected_color, bool *is_done);

static void log_temperature(struct dpu_rank_t *rank, u64 *results);

__API_SYMBOL__ u32 ci_commit_commands(struct dpu_rank_t *rank, u64 *commands)
{
	struct dpu_rank_handler *handler = GET_HANDLER(rank);
	u32 ret;

	LOGV_PACKET(rank, commands, WRITE_DIR);

	ret = debug_record_last_cmd(rank, WRITE_DIR, commands);
	if (ret != DPU_OK)
		return ret;

	if (handler->commit_commands(rank, commands) != DPU_RANK_SUCCESS) {
		return DPU_ERR_DRIVER;
	}

	return DPU_OK;
}

__API_SYMBOL__ u32 ci_update_commands(struct dpu_rank_t *rank, u64 *commands)
{
	struct dpu_rank_handler *handler = GET_HANDLER(rank);
	u32 ret;

	if (handler->update_commands(rank, commands) != DPU_RANK_SUCCESS) {
		return DPU_ERR_DRIVER;
	}

	ret = debug_record_last_cmd(rank, READ_DIR, commands);
	if (ret != DPU_OK)
		return ret;

	return DPU_OK;
}

__API_SYMBOL__ void ci_prepare_mask(u64 *buffer, u8 mask, u64 data)
{
	u8 each_ci;

	for (each_ci = 0; each_ci < DPU_MAX_NR_CIS; ++each_ci) {
		if (CI_MASK_ON(mask, each_ci)) {
			buffer[each_ci] = data;
		} else {
			buffer[each_ci] = CI_EMPTY;
		}
	}
}

__API_SYMBOL__ u32 ci_fill_selected_dpu_mask(struct dpu_rank_t *rank, u8 ci,
					     u8 *mask)
{
	u8 nr_dpus =
		GET_DESC_HW(rank)->topology.nr_of_dpus_per_control_interface;
	struct dpu_configuration_slice_info_t *info =
		&(GET_CI_CONTEXT(rank)->slice_info[ci]);

	u8 dpu_mask;
	switch (info->slice_target.type) {
	case DPU_SLICE_TARGET_ALL:
		dpu_mask = (1 << nr_dpus) - 1;
		break;
	case DPU_SLICE_TARGET_DPU:
		dpu_mask = 1 << info->slice_target.dpu_id;
		break;
	case DPU_SLICE_TARGET_GROUP:
		dpu_mask = info->dpus_per_group[info->slice_target.group_id];
		break;
	default:
		return DPU_ERR_INTERNAL;
	}

	*mask = dpu_mask;

	return DPU_OK;
}

__API_SYMBOL__ u32 ci_exec_void_cmd(struct dpu_rank_t *rank, u64 *commands)
{
	return exec_cmd(rank, commands, false);
}

__API_SYMBOL__ u32 ci_exec_wait_mask_cmd(struct dpu_rank_t *rank, u64 *commands)
{
	return exec_cmd(rank, commands, true);
}

__API_SYMBOL__ u32 ci_exec_8bit_cmd(struct dpu_rank_t *rank, u64 *commands,
				    u8 *results)
{
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;
	u32 status;

	if ((status = exec_cmd(rank, commands, false)) != DPU_OK) {
		return status;
	}

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (commands[each_ci] != CI_EMPTY) {
			results[each_ci] = rank->data[each_ci];
		}
	}

	return DPU_OK;
}

__API_SYMBOL__ u32 ci_exec_32bit_cmd(struct dpu_rank_t *rank, u64 *commands,
				     u32 *results)
{
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;
	u32 status;

	if ((status = exec_cmd(rank, commands, false)) != DPU_OK) {
		return status;
	}

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (commands[each_ci] != CI_EMPTY) {
			results[each_ci] = rank->data[each_ci];
		}
	}

	return DPU_OK;
}

static u32 wait_for_byte_discovery(struct dpu_rank_t *rank, u64 *results,
				   u8 ci_mask)
{
	u32 nr_retries = NB_RETRY_FOR_VALID_RESULT;
	bool in_progress, timeout;
	u32 status;

	do {
		if ((status = ci_update_commands(rank, results)) != DPU_OK) {
			return status;
		}

		in_progress = !determine_if_byte_discoveries_are_finished(
			rank, results, ci_mask);
		timeout = (nr_retries--) == 0;
	} while (in_progress && !timeout);

	if (in_progress) {
		LOGV_PACKET(rank, results, READ_DIR);
		return DPU_ERR_TIMEOUT;
	}

	return DPU_OK;
}

__API_SYMBOL__ u32 ci_exec_byte_discovery(struct dpu_rank_t *rank,
					  u64 *commands, u64 *results)
{
	u8 ci_mask = compute_ci_mask(rank, commands);
	u32 status;

	invert_color(rank, ci_mask);

	if ((status = ci_commit_commands(rank, commands)) != DPU_OK) {
		return status;
	}

	if ((status = wait_for_byte_discovery(rank, results, ci_mask)) !=
	    DPU_OK) {
		return status;
	}

	/* We make sure that we have the correct results by reading again (we may have timing issues). */
	if ((status = wait_for_byte_discovery(rank, results, ci_mask)) !=
	    DPU_OK) {
		return status;
	}

	LOGV_PACKET(rank, results, READ_DIR);

	return DPU_OK;
}

__API_SYMBOL__ u32 ci_exec_reset_cmd(struct dpu_rank_t *rank, u64 *commands)
{
	u32 status;
	struct dpu_control_interface_context *context = GET_CI_CONTEXT(rank);
	u8 nr_dpus =
		GET_DESC_HW(rank)->topology.nr_of_dpus_per_control_interface;

	u8 clock_division = GET_DESC_HW(rank)->timings.clock_division;
	u32 reset_duration = GET_DESC_HW(rank)->timings.reset_wait_duration *
			     clock_division / 2;
	u64 data[DPU_MAX_NR_CIS];
	u32 each_iter;
	u8 each_ci, each_group;

	u8 reset_mask = compute_ci_mask(rank, commands);

	invert_color(rank, reset_mask);

	if ((status = ci_commit_commands(rank, commands)) != DPU_OK) {
		return status;
	}

	// Software Reset has been sent. We need to wait "some time" before we can continue (~ 20 clock cycles)
	// For simulator backends, we need to run the simulation. We suppose that these function calls will be long enough
	// to put a real hardware backend in a stable state.

	for (each_iter = 0; each_iter < reset_duration; ++each_iter) {
		if ((status = ci_update_commands(rank, data)) != DPU_OK) {
			return status;
		}
	}

	/* Reset the color of the control interfaces */
	context->color &= ~reset_mask;
	context->fault_collide &= ~reset_mask;
	context->fault_decode &= ~reset_mask;

	/* Reset the group & select information */
	for (each_ci = 0; each_ci < DPU_MAX_NR_CIS; ++each_ci) {
		context->slice_info[each_ci].slice_target.type =
			DPU_SLICE_TARGET_NONE;
		context->slice_info[each_ci].dpus_per_group[0] =
			(1 << nr_dpus) - 1;

		for (each_group = 1; each_group < DPU_MAX_NR_GROUPS;
		     ++each_group) {
			context->slice_info[each_ci].dpus_per_group[each_group] =
				0;
		}
	}

	return DPU_OK;
}

static u32 exec_cmd(struct dpu_rank_t *rank, u64 *commands,
		    bool add_select_mask)
{
	u8 expected_color;
	u64 result_masks[DPU_MAX_NR_CIS] = {};
	u64 expected[DPU_MAX_NR_CIS] = {};
	u64 *data = rank->data;
	bool is_done[DPU_MAX_NR_CIS] = {};
	u8 ci_mask;
	u32 status;
	bool in_progress, timeout;
	u32 nr_retries = NB_RETRY_FOR_VALID_RESULT;

	if ((status = compute_masks(rank, commands, result_masks, expected,
				    &ci_mask, add_select_mask, is_done)) !=
	    DPU_OK) {
		return status;
	}

	expected_color = GET_CI_CONTEXT(rank)->color & ci_mask;
	invert_color(rank, ci_mask);

	if ((status = ci_commit_commands(rank, commands)) != DPU_OK) {
		return status;
	}

	do {
		if ((status = ci_update_commands(rank, data)) != DPU_OK) {
			return status;
		}

		in_progress = !determine_if_commands_are_finished(
			rank, data, expected, result_masks, expected_color,
			is_done);
		timeout = (nr_retries--) == 0;
	} while (in_progress && !timeout);

	if (in_progress) {
		/* Either we are in full log:
		 * and then log at least one packet as it has important info to debug.
		 */
		LOGV_PACKET(rank, data, READ_DIR);

		/* Or we are in debug command mode:
		 * and then dump the entire buffer of commands.
		 */
		LOGD_LAST_COMMANDS(rank);

		return DPU_ERR_TIMEOUT;
	}

	/* All results are ready here, and still present when reading the control interfaces.
     * We make sure that we have the correct results by reading again (we may have timing issues).
     * todo(#85): this can be somewhat costly. We should try to integrate this additional read in a lower layer.
     */
	if ((status = ci_update_commands(rank, data)) != DPU_OK) {
		return status;
	}

	LOGV_PACKET(rank, data, READ_DIR);

	log_temperature(rank, data);

	return DPU_OK;
}

static void invert_color(struct dpu_rank_t *rank, u8 ci_mask)
{
	GET_CI_CONTEXT(rank)->color ^= ci_mask;
}

static u8 compute_ci_mask(struct dpu_rank_t *rank, const u64 *commands)
{
	u8 ci_mask = 0;
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (commands[each_ci] != CI_EMPTY) {
			ci_mask |= (1 << each_ci);
		}
	}

	return ci_mask;
}

static u32 compute_masks(struct dpu_rank_t *rank, const u64 *commands,
			 u64 *masks, u64 *expected, u8 *cis,
			 bool add_select_mask, bool *is_done)
{
	u8 ci_mask = 0;

	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 nr_dpus =
		GET_DESC_HW(rank)->topology.nr_of_dpus_per_control_interface;

	u8 each_ci;

	u8 dpu_mask;
	u32 status;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (commands[each_ci] != CI_EMPTY) {
			ci_mask |= (1 << each_ci);
			masks[each_ci] |= 0xFF0000FF00000000l;
			expected[each_ci] |= 0x000000FF00000000l;

			if (add_select_mask) {
				masks[each_ci] |= (1 << nr_dpus) - 1;

				if ((status = ci_fill_selected_dpu_mask(
					     rank, each_ci, &dpu_mask)) !=
				    DPU_OK) {
					return status;
				}

				expected[each_ci] |= dpu_mask;
			}
		} else
			is_done[each_ci] = true;
	}

	*cis = ci_mask;

	return DPU_OK;
}

static bool determine_if_byte_discoveries_are_finished(struct dpu_rank_t *rank,
						       const u64 *data,
						       u8 ci_mask)
{
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci, each_byte;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		bool finished = false;

		if (!CI_MASK_ON(ci_mask, each_ci))
			continue;

		for (each_byte = 0; each_byte < sizeof(u64); ++each_byte) {
			if (((data[each_ci] >> (each_byte << 3)) & 0xFF) == 0) {
				finished = true;
				break;
			}
		}

		if (!finished) {
			return false;
		}
	}

	return true;
}

static bool determine_if_commands_are_finished(struct dpu_rank_t *rank,
					       const u64 *data,
					       const u64 *expected,
					       const u64 *result_masks,
					       u8 expected_color, bool *is_done)
{
	struct dpu_control_interface_context *context = GET_CI_CONTEXT(rank);
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u8 each_ci;
	u8 color, nb_bits_set, ci_mask, ci_color;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (!is_done[each_ci]) {
			u64 result = data[each_ci];

			/* The second case can happen when the debugger has restored the result */
			if ((result & result_masks[each_ci]) !=
				    expected[each_ci] &&
			    (result & CI_NOP) != CI_NOP) {
				return false;
			}

			/* From here, the result seems ok, we just need to check that the color is ok. */
			color = ((result & 0x00FF000000000000ULL) >> 48) & 0xFF;
			nb_bits_set = __builtin_popcount(color);
			ci_mask = 1 << each_ci;
			ci_color = expected_color & ci_mask;

			if (ci_color != 0) {
				if (nb_bits_set <= 3) {
					return false;
				}
			} else {
				if (nb_bits_set >= 5) {
					return false;
				}
			}

			is_done[each_ci] = true;

			/* We are in fault path, store this information */
			if (ci_color != 0) {
				nb_bits_set = 8 - nb_bits_set;
			}

			switch (nb_bits_set) {
			case 0:
				break;
			case 1:
				LOG_CI(VERBOSE, rank, each_ci,
				       "command decoding error detected");
				context->fault_decode |= ci_mask;
				break;
			case 2:
				LOG_CI(VERBOSE, rank, each_ci,
				       "command collision detected");
				context->fault_collide |= ci_mask;
				break;
			case 3:
				LOG_CI(VERBOSE, rank, each_ci,
				       "command collision detected");
				context->fault_collide |= ci_mask;
				LOG_CI(VERBOSE, rank, each_ci,
				       "command decoding error detected");
				context->fault_decode |= ci_mask;
				break;
			default:
				LOG_CI(DEBUG, rank, each_ci,
				       "Number of bits set (%u) is inconsistent."
				       "Mark the result as not ready.",
				       nb_bits_set);
				return false;
			}

			is_done[each_ci] = true;
		}
	}

	return true;
}

static void log_temperature(struct dpu_rank_t *rank, u64 *results)
{
	if (LOG_TEMPERATURE_ENABLED() && LOG_TEMPERATURE_TRIGGERED(rank)) {
		LOG_TEMPERATURE(rank, results);
	}
}

u32 ci_get_color(struct dpu_rank_t *rank, u32 *ret_data)
{
	u8 nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	u64 data[DPU_MAX_NR_CIS];
	u32 timeout = TIMEOUT_COLOR;
	u32 status = DPU_OK;
	bool dummy_command_is_needed = false;
	bool result_is_stable;
	dpu_slice_id_t each_slice;

	/* Read the control interface as long as [63: 56] != 0: this loop is necessary in case of FPGA where
	 * a latency due to implementation makes the initial result not appear right away in case of valid command (ie
	 * not 0x00 and not 0xFF (NOP)).
	 * A NOP command does not change the color and [63: 56],
	 * A valid command changes the color and clears [63: 56],
	 * An invalid command (ie something the CI does not understand) changes the color since it sets CMD_FAULT_DECODE and clears
	 * [63: 56],
	 *
	 * In addition here, we must wait for the result to be valid because we need to restore it before handing control
	 * back to the host application: since we don't know the last command, we cannot be sure of the type of response
	 * the host application expects (cf WRAM/IRAM write). According to Fabrice, there might be at most 28 DPU cycles
	 * between the moment we see [39: 32] == 0xFF and particular commands get their whole result in [31: 0]
	 * => so once [39: 32] == 0xFF, we re-read the result to make sure it is ok.
	 */
	do {
		FF(ci_update_commands(rank, data));

		timeout--;

		result_is_stable = true;
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			u8 cmd_type = (u8)(((data[each_slice] &
					     0xFF00000000000000ULL) >>
					    56) &
					   0xFF);
			if (cmd_type == 0xFF) {
				dummy_command_is_needed = true;
			} else {
				u8 valid = (u8)(((data[each_slice] &
						  0xFF00000000ULL) >>
						 32) &
						0xFF);
				result_is_stable = result_is_stable &&
						   (cmd_type == 0) &&
						   (valid == 0xFF);
			}
		}
	} while (timeout && !result_is_stable);

	if (!timeout) {
		LOG_RANK(WARNING, rank,
			 "Timeout waiting for result to be correct");
		status = DPU_ERR_TIMEOUT;
		goto end;
	}

	FF(ci_update_commands(rank, data));

	if (ret_data) {
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			ret_data[each_slice] = (u32)(data[each_slice]);
		}
	}

	if (dummy_command_is_needed) {
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			if (((data[each_slice] >> 56) & 0xFF) == 0xFF) {
				LOG_CI(VERBOSE, rank, each_slice,
				       "Nop command, must execute dummy command to get the right color...");
				/* To avoid stuttering here, just send twice a byte order command with [63: 56] shuffled
				 * and send an identity command and wait for the result to get the color.
				 */
				// TODO
				/* And simply commit an identity command to get the right color: we must
				 * bypass UFI that requires the color. The timeout is needed for the same
				 * reason as above.
				 */
				data[each_slice] = CI_IDENTITY;
			} else {
				data[each_slice] = CI_EMPTY;
			}
		}

		FF(ci_commit_commands(rank, data));

		timeout = TIMEOUT_COLOR;
		do {
			FF(ci_update_commands(rank, data));

			timeout--;

			result_is_stable = true;
			for (each_slice = 0; each_slice < nr_cis;
			     ++each_slice) {
				result_is_stable =
					result_is_stable &&
					((data[each_slice] >> 56) == 0);
			}
		} while (timeout && !result_is_stable);

		if (!timeout) {
			LOG_RANK(WARNING, rank,
				 "Timeout waiting for result to be correct");
			status = DPU_ERR_TIMEOUT;
			goto end;
		}
	}

	/* Here, we have the color */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		u8 nb_bits_set = __builtin_popcount(
			((data[each_slice] & 0x00FF000000000000ULL) >> 48) &
			0xFF);
		u8 expected_color = (u8)((nb_bits_set <= 3) ? 1 : 0);

		if (!expected_color) {
			GET_CI_CONTEXT(rank)->color &= ~(1UL << each_slice);
		} else {
			GET_CI_CONTEXT(rank)->color |= (1UL << each_slice);
		}
	}

end:
	return status;
}
