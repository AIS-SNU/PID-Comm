/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_instruction_encoder.h>
#include <dpu_internals.h>
#include <dpu_management.h>
#include <dpu_mask.h>
#include <dpu_predef_programs.h>
#include <dpu_rank.h>
#include <dpu_types.h>

#include <ufi_rank_utils.h>
#include <ufi/ufi.h>
#include <ufi/ufi_config.h>
#include <ufi/ufi_bit_config.h>
#include <ufi/ufi_dma_wavegen_config.h>

#define REFRESH_MODE_VALUE 2

#define EXPECTED_BYTE_ORDER_RESULT_AFTER_CONFIGURATION 0x000103FF0F8FCFEFULL

/* Bit set when the DPU has the control of the bank */
#define MUX_DPU_BANK_CTRL (1 << 0)
/* Bit set when the DPU can write to the bank */
#define MUX_DPU_WRITE_CTRL (1 << 1)
/* Bit set when the host or the DPU wrote to the bank without permission */
#define MUX_COLLISION_ERR (1 << 7)

#define WAVEGEN_MUX_HOST_EXPECTED 0x00
#define WAVEGEN_MUX_DPU_EXPECTED (MUX_DPU_BANK_CTRL | MUX_DPU_WRITE_CTRL)

static const char *clock_division_to_string(dpu_clock_division_t clock_division)
{
	switch (clock_division) {
	case DPU_CLOCK_DIV8:
		return "CLOCK DIV 8";
	case DPU_CLOCK_DIV4:
		return "CLOCK DIV 4";
	case DPU_CLOCK_DIV3:
		return "CLOCK DIV 3";
	case DPU_CLOCK_DIV2:
		return "CLOCK DIV 2";
	default:
		return "CLOCK DIV UNKNOWN";
	}
}

static enum dpu_temperature from_celsius_to_dpu_enum(uint8_t temperature)
{
	if (temperature < 50)
		return DPU_TEMPERATURE_LESS_THAN_50;
	if (temperature < 60)
		return DPU_TEMPERATURE_BETWEEN_50_AND_60;
	if (temperature < 70)
		return DPU_TEMPERATURE_BETWEEN_60_AND_70;
	if (temperature < 80)
		return DPU_TEMPERATURE_BETWEEN_70_AND_80;
	if (temperature < 90)
		return DPU_TEMPERATURE_BETWEEN_80_AND_90;
	if (temperature < 100)
		return DPU_TEMPERATURE_BETWEEN_90_AND_100;
	if (temperature < 110)
		return DPU_TEMPERATURE_BETWEEN_100_AND_110;

	return DPU_TEMPERATURE_GREATER_THAN_110;
}

static const char *pc_mode_to_string(enum dpu_pc_mode pc_mode)
{
	switch (pc_mode) {
	case DPU_PC_MODE_12:
		return "12";
	case DPU_PC_MODE_13:
		return "13";
	case DPU_PC_MODE_14:
		return "14";
	case DPU_PC_MODE_15:
		return "15";
	case DPU_PC_MODE_16:
		return "16";
	default:
		return "UNKNOWN";
	}
}

static inline const char *stack_direction_to_string(bool stack_is_up)
{
	return stack_is_up ? "up" : "down";
}

static void save_enabled_dpus(struct dpu_rank_t *rank,
			      bool *all_dpus_are_enabled_save,
			      dpu_selected_mask_t *enabled_dpus_save,
			      bool update_save)
{
	dpu_description_t desc = rank->description;
	uint8_t nr_cis = desc->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus = desc->hw.topology.nr_of_dpus_per_control_interface;
	dpu_selected_mask_t all_dpus_mask = dpu_mask_all(nr_dpus);
	dpu_slice_id_t each_ci;
	dpu_member_id_t each_dpu;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		struct dpu_configuration_slice_info_t *ci_info =
			&rank->runtime.control_interface.slice_info[each_ci];

		all_dpus_are_enabled_save[each_ci] =
			!update_save ? ci_info->all_dpus_are_enabled :
					     all_dpus_are_enabled_save[each_ci] &
					       ci_info->all_dpus_are_enabled;
		enabled_dpus_save[each_ci] =
			!update_save ? ci_info->enabled_dpus :
					     enabled_dpus_save[each_ci] &
					       ci_info->enabled_dpus;

		/* Do not even talk to CIs where ALL dpus are disabled. */
		if (!ci_info->enabled_dpus)
			continue;

		ci_info->all_dpus_are_enabled = true;
		ci_info->enabled_dpus = all_dpus_mask;

		for (each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
			DPU_GET_UNSAFE(rank, each_ci, each_dpu)->enabled = true;
		}
	}
}

static bool byte_order_values_are_compatible(uint64_t reference, uint64_t found)
{
	unsigned int each_byte;
	for (each_byte = 0; each_byte < sizeof(uint64_t); ++each_byte) {
		if (__builtin_popcount((
			    uint8_t)((reference >> (8 * each_byte)) & 0xFFl)) !=
		    __builtin_popcount(
			    (uint8_t)((found >> (8 * each_byte)) & 0xFFl))) {
			return false;
		}
	}

	return true;
}

static dpu_error_t dpu_byte_order(struct dpu_rank_t *rank)
{
	uint64_t *byte_order_results = rank->data;
	uint8_t mask = ALL_CIS;
	dpu_error_t status = DPU_OK;
	dpu_slice_id_t slice_id;

	LOG_RANK(VERBOSE, rank, "");

	/*
	 * BYTE_ORDER command must be issued before SOFT_RESET *but* if the HW
	 * is really in trouble, it will fail to answer to BYTE_ORDER whereas a
	 * simple SOFT_RESET would suffice to fix it. On HW, do not issue any
	 * BYTE_ORDER command, the driver will expose its value that will be
	 * retrieved at rank allocation: the driver should not fail since nothing
	 * can have messed with CI before itself.
	 */
	if (rank->type == HW) {
		uint8_t nr_cis =
			rank->description->hw.topology.nr_of_control_interfaces;

		for (slice_id = 0; slice_id < nr_cis; ++slice_id)
			byte_order_results[slice_id] =
				rank->runtime.control_interface
					.slice_info[slice_id]
					.byte_order;
	} else {
		FF(ufi_select_cis(rank, &mask));
		FF(ufi_byte_order(rank, mask, byte_order_results));
	}

	for (slice_id = 0;
	     slice_id < rank->description->hw.topology.nr_of_control_interfaces;
	     ++slice_id) {
		if (!CI_MASK_ON(mask, slice_id))
			continue;

		if (!byte_order_values_are_compatible(
			    byte_order_results[slice_id],
			    EXPECTED_BYTE_ORDER_RESULT_AFTER_CONFIGURATION)) {
			LOG_CI(WARNING, rank, slice_id,
			       "ERROR: invalid byte order (reference: 0x%016lx; found: 0x%016lx)",
			       EXPECTED_BYTE_ORDER_RESULT_AFTER_CONFIGURATION,
			       byte_order_results[slice_id]);
			status = DPU_ERR_DRIVER;
		}
	}

end:
	return status;
}

static dpu_error_t dpu_soft_reset(struct dpu_rank_t *rank,
				  dpu_clock_division_t clock_division)
{
	dpu_error_t status;
	uint8_t cycle_accurate =
		rank->description->configuration.enable_cycle_accurate_behavior ?
			      1 :
			      0;
	uint8_t mask = ALL_CIS;

	LOG_RANK(VERBOSE, rank, "%s", clock_division_to_string(clock_division));

	FF(ufi_select_cis(rank, &mask));
	ufi_soft_reset(rank, mask, clock_division, cycle_accurate);

end:
	return status;
}

static dpu_error_t dpu_bit_config(struct dpu_rank_t *rank,
				  struct dpu_bit_config *config)
{
	uint32_t bit_config_results[DPU_MAX_NR_CIS];
	uint8_t mask = ALL_CIS;
	dpu_error_t status;
	dpu_slice_id_t slice_id;
	uint32_t bit_config_result;

	LOG_RANK(VERBOSE, rank, "");

	FF(ufi_select_cis(rank, &mask));
	FF(ufi_bit_config(rank, mask, NULL, bit_config_results));

	bit_config_result = bit_config_results[__builtin_ctz(mask)];

	/* Let's verify that all CIs have the bit config result as the first CI. */
	for (slice_id = 0;
	     slice_id < rank->description->hw.topology.nr_of_control_interfaces;
	     ++slice_id) {
		if (!CI_MASK_ON(mask, slice_id))
			continue;

		if (bit_config_results[slice_id] != bit_config_result) {
			LOG_RANK(
				WARNING, rank,
				"inconsistent bit configuration between the different CIs (0x%08x != 0x%08x)",
				bit_config_results[slice_id],
				bit_config_result);
			status = DPU_ERR_INTERNAL;
			goto end;
		}
	}

	dpu_bit_config_compute(bit_config_result, config);

	config->stutter = 0;

	LOG_RANK(
		DEBUG, rank,
		"bit_order: 0x%08x nibble_swap: 0x%02x cpu_to_dpu: 0x%04x dpu_to_cpu: 0x%04x",
		bit_config_result, config->nibble_swap, config->cpu2dpu,
		config->dpu2cpu);

end:
	return status;
}

static dpu_error_t dpu_identity(struct dpu_rank_t *rank)
{
	uint32_t identity_results[DPU_MAX_NR_CIS];
	uint8_t mask = ALL_CIS;
	dpu_error_t status;
	dpu_slice_id_t slice_id;
	uint32_t identity_result;

	LOG_RANK(VERBOSE, rank, "");

	FF(ufi_select_cis(rank, &mask));
	FF(ufi_identity(rank, mask, identity_results));

	identity_result = identity_results[__builtin_ctz(mask)];
	for (slice_id = 0;
	     slice_id < rank->description->hw.topology.nr_of_control_interfaces;
	     ++slice_id) {
		if (!CI_MASK_ON(mask, slice_id))
			continue;

		if (identity_results[slice_id] != identity_result) {
			LOG_RANK(
				WARNING, rank,
				"inconsistent identity between the different CIs (0x%08x != 0x%08x)",
				identity_results[slice_id], identity_result);
			status = DPU_ERR_INTERNAL;
			goto end;
		}
	}

	if (identity_result != rank->description->hw.signature.chip_id) {
		LOG_RANK(
			WARNING, rank,
			"ERROR: invalid identity (expected: 0x%08x; found: 0x%08x)",
			rank->description->hw.signature.chip_id,
			identity_result);
		status = DPU_ERR_INTERNAL;
	}

end:
	return status;
}

static dpu_error_t
dpu_ci_shuffling_box_config(struct dpu_rank_t *rank,
			    const struct dpu_bit_config *config)
{
	dpu_error_t status;
	uint8_t mask = ALL_CIS;

	LOG_RANK(VERBOSE, rank, "c2d: 0x%04x, d2c: 0x%04x, nibble_swap: 0x%02x",
		 config->cpu2dpu, config->dpu2cpu, config->nibble_swap);

	FF(ufi_select_cis(rank, &mask));
	FF(ufi_bit_config(rank, mask, config, NULL));

end:
	return status;
}

static dpu_error_t dpu_thermal_config(struct dpu_rank_t *rank,
				      uint8_t thermal_config)
{
	dpu_error_t status;
	enum dpu_temperature temperature =
		from_celsius_to_dpu_enum(thermal_config);
	uint8_t mask = ALL_CIS;

	LOG_RANK(VERBOSE, rank, "%d°C (value: 0x%04x)", thermal_config,
		 temperature);

	FF(ufi_select_cis(rank, &mask));
	ufi_thermal_config(rank, mask, temperature);
end:
	return status;
}

static dpu_error_t dpu_carousel_config(struct dpu_rank_t *rank,
				       const struct dpu_carousel_config *config)
{
	dpu_error_t status;
	uint8_t mask = ALL_CIS;

	LOG_RANK(
		VERBOSE, rank,
		"cmd_duration: %d cmd_sampling: %d res_duration: %d res_sampling: %d",
		config->cmd_duration, config->cmd_sampling,
		config->res_duration, config->res_sampling);

	FF(ufi_select_all(rank, &mask));
	FF(ufi_carousel_config(rank, mask, config));

end:
	return status;
}

static inline uint8_t count_nr_of_faulty_bits(uint64_t data)
{
	return __builtin_popcountll(data);
}

static inline uint8_t find_index_of_first_faulty_bit(uint64_t data)
{
	return __builtin_ctz(data);
}

static inline uint8_t find_index_of_last_faulty_bit(uint64_t data)
{
	return 63 - __builtin_clz(data);
}

static bool
extract_memory_repair_configuration(struct dpu_t *dpu,
				    struct dpu_memory_repair_t *repair_info,
				    struct dpu_repair_config *repair_config)
{
	uint32_t each_corrupted_addr_index;
	uint32_t each_faulty_address;
	uint32_t nr_of_corrupted_addr = repair_info->nr_of_corrupted_addresses;
	dpuinstruction_t faulty_bits;
	uint8_t nr_of_faulty_bits;
	uint8_t first_index;
	uint8_t last_index;

	repair_config->AB_msbs = 0xFF;
	repair_config->CD_msbs = 0xFF;
	repair_config->A_lsbs = 0xFF;
	repair_config->B_lsbs = 0xFF;
	repair_config->C_lsbs = 0xFF;
	repair_config->D_lsbs = 0xFF;
	repair_config->even_index = 0xFF;
	repair_config->odd_index = 0xFF;

	LOG_DPU(DEBUG, dpu, "repair info: number of corrupted addresses: %d",
		nr_of_corrupted_addr);

	for (each_corrupted_addr_index = 0;
	     each_corrupted_addr_index < nr_of_corrupted_addr;
	     ++each_corrupted_addr_index) {
		LOG_DPU(DEBUG, dpu,
			"repair info: #%d address: 0x%04x faulty_bits: 0x%016lx",
			each_corrupted_addr_index,
			repair_info
				->corrupted_addresses[each_corrupted_addr_index]
				.address,
			repair_info
				->corrupted_addresses[each_corrupted_addr_index]
				.faulty_bits);
	}

	if (nr_of_corrupted_addr > NB_MAX_REPAIR_ADDR) {
		LOG_DPU(DEBUG, dpu,
			"ERROR: too many corrupted addresses (%d > %d)",
			nr_of_corrupted_addr, NB_MAX_REPAIR_ADDR);
		return false;
	}

	for (each_corrupted_addr_index = 0;
	     each_corrupted_addr_index < nr_of_corrupted_addr;
	     ++each_corrupted_addr_index) {
		uint32_t address =
			repair_info
				->corrupted_addresses[each_corrupted_addr_index]
				.address;
		uint8_t msbs = (uint8_t)(address >> 4);
		uint8_t lsbs = (uint8_t)(address & 0xF);

		if (repair_config->A_lsbs == 0xFF) {
			repair_config->AB_msbs = msbs;
			repair_config->A_lsbs = lsbs;
		} else if ((repair_config->B_lsbs == 0xFF) &&
			   (repair_config->AB_msbs == msbs)) {
			repair_config->B_lsbs = lsbs;
		} else if (repair_config->C_lsbs == 0xFF) {
			repair_config->CD_msbs = msbs;
			repair_config->C_lsbs = lsbs;
		} else if ((repair_config->D_lsbs == 0xFF) &&
			   (repair_config->CD_msbs == msbs)) {
			repair_config->D_lsbs = lsbs;
		} else {
			LOG_DPU(DEBUG, dpu,
				"ERROR: corrupted addresses are too far apart");
			return false;
		}
	}

	faulty_bits = 0L;

	for (each_faulty_address = 0;
	     each_faulty_address < nr_of_corrupted_addr;
	     ++each_faulty_address) {
		faulty_bits |=
			repair_info->corrupted_addresses[each_faulty_address]
				.faulty_bits;
	}

	nr_of_faulty_bits = count_nr_of_faulty_bits(faulty_bits);

	if (nr_of_faulty_bits > 2) {
		LOG_DPU(DEBUG, dpu, "ERROR: too many corrupted bits (%d > %d)",
			nr_of_faulty_bits, 2);
		return false;
	}

	switch (count_nr_of_faulty_bits(faulty_bits)) {
	default:
		return false;
	case 2:
		last_index = find_index_of_last_faulty_bit(faulty_bits);

		if ((last_index & 1) == 1) {
			repair_config->odd_index = last_index;
		} else {
			repair_config->even_index = last_index;
		}
		// FALLTHROUGH
	case 1:
		first_index = find_index_of_first_faulty_bit(faulty_bits);

		if ((first_index & 1) == 1) {
			if (repair_config->odd_index != 0xFF) {
				LOG_DPU(DEBUG, dpu,
					"ERROR: both corrupted bit indices are odd");
				return false;
			}

			repair_config->odd_index = first_index;
		} else {
			if (repair_config->even_index != 0xFF) {
				LOG_DPU(DEBUG, dpu,
					"ERROR: both corrupted bit indices are even");
				return false;
			}

			repair_config->even_index = first_index;
		}
		// FALLTHROUGH
	case 0:
		break;
	}

	// We can repair the memory! Let's choose default values then return the configuration.
	if (repair_config->A_lsbs == 0xFF) {
		repair_config->A_lsbs = 0xF;
	}
	if (repair_config->B_lsbs == 0xFF) {
		repair_config->B_lsbs = 0xF;
	}
	if (repair_config->C_lsbs == 0xFF) {
		repair_config->C_lsbs = 0xF;
	}
	if (repair_config->D_lsbs == 0xFF) {
		repair_config->D_lsbs = 0xF;
	}

	if (repair_config->even_index == 0xFF) {
		repair_config->even_index = 0;
	}

	if (repair_config->odd_index == 0xFF) {
		repair_config->odd_index = 1;
	}

	LOG_DPU(DEBUG, dpu,
		"valid repair config: AB_MSBs: 0x%02x A_LSBs: 0x%01x B_LSBs: 0x%01x CD_MSBs: 0x%02x C_LSBs: 0x%01x D_LSBs: 0x%01x",
		repair_config->AB_msbs, repair_config->A_lsbs,
		repair_config->B_lsbs, repair_config->CD_msbs,
		repair_config->C_lsbs, repair_config->D_lsbs);

	return true;
}

static dpu_error_t try_to_repair_iram(struct dpu_rank_t *rank)
{
	dpu_error_t status = DPU_OK;

	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;

	struct dpu_repair_config configs[DPU_MAX_NR_CIS];
	struct dpu_repair_config *config_array[DPU_MAX_NR_CIS];

	struct dpu_configuration_slice_info_t *ci_infos =
		rank->runtime.control_interface.slice_info;
	bool all_disabled = true;

	dpu_slice_id_t each_ci;
	dpu_member_id_t each_dpu;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		config_array[each_ci] = &configs[each_ci];
	}

	for (each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
		uint8_t ci_mask = ALL_CIS;

		for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
			struct dpu_t *dpu =
				DPU_GET_UNSAFE(rank, each_ci, each_dpu);

			if (!extract_memory_repair_configuration(
				    dpu, &dpu->repair.iram_repair,
				    config_array[each_ci])) {
				LOG_DPU(DEBUG, dpu,
					"ERROR: cannot repair IRAM");
				dpu->repair.iram_repair.fail_to_repair = true;
				dpu->enabled = false;
				ci_infos[each_ci].enabled_dpus &=
					~(1 << each_dpu);
				ci_infos[each_ci].all_dpus_are_enabled = false;
			}
		}

		FF(ufi_select_dpu(rank, &ci_mask, each_dpu));
		FF(ufi_iram_repair_config(rank, ci_mask, config_array));
	}

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (ci_infos[each_ci].enabled_dpus) {
			all_disabled = false;
			break;
		}
	}

	if (all_disabled) {
		LOG_RANK(
			WARNING, rank,
			"No enabled dpus in this rank due to memory corruption.");
		return DPU_ERR_CORRUPTED_MEMORY;
	}

end:
	return status;
}

static dpu_error_t try_to_repair_wram(struct dpu_rank_t *rank)
{
	dpu_error_t status = DPU_OK;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	struct dpu_repair_config configs[DPU_MAX_NR_CIS];
	struct dpu_repair_config *config_array[DPU_MAX_NR_CIS];
	struct dpu_configuration_slice_info_t *ci_infos =
		rank->runtime.control_interface.slice_info;
	bool all_disabled = true;
	dpu_slice_id_t each_ci;
	dpu_member_id_t each_dpu;

	LOG_RANK(VERBOSE, rank, "");

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		config_array[each_ci] = &configs[each_ci];
	}

	for (each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
		uint8_t each_wram_bank;

		for (each_wram_bank = 0; each_wram_bank < NR_OF_WRAM_BANKS;
		     ++each_wram_bank) {
			uint8_t ci_mask = ALL_CIS;

			for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
				struct dpu_t *dpu =
					DPU_GET_UNSAFE(rank, each_ci, each_dpu);

				if (!extract_memory_repair_configuration(
					    dpu,
					    &dpu->repair
						     .wram_repair[each_wram_bank],
					    config_array[each_ci])) {
					LOG_DPU(DEBUG, dpu,
						"ERROR: cannot repair WRAM bank #%d",
						each_wram_bank);
					dpu->repair.wram_repair[each_wram_bank]
						.fail_to_repair = true;
					dpu->enabled = false;
					ci_infos[each_ci].enabled_dpus &=
						~(1 << each_dpu);
					ci_infos[each_ci].all_dpus_are_enabled =
						false;
				}
			}

			FF(ufi_select_dpu(rank, &ci_mask, each_dpu));
			FF(ufi_wram_repair_config(rank, ci_mask, each_wram_bank,
						  config_array));
		}
	}

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (ci_infos[each_ci].enabled_dpus) {
			all_disabled = false;
			break;
		}
	}

	if (all_disabled) {
		LOG_RANK(
			WARNING, rank,
			"No enabled dpus in this rank due to memory corruption.");
		return DPU_ERR_CORRUPTED_MEMORY;
	}

end:
	return status;
}
static dpu_error_t dpu_iram_repair_config(struct dpu_rank_t *rank)
{
	dpu_error_t status;

	if (rank->description->configuration.do_iram_repair) {
		LOG_RANK(VERBOSE, rank, "IRAM repair enabled");
		// SRAM defects were filled at rank allocation
		FF(try_to_repair_iram(rank));
	} else {
		uint8_t mask = ALL_CIS;
		// Default configuration
		struct dpu_repair_config config = { 0, 0, 0, 0, 0, 0, 0, 1 };
		struct dpu_repair_config *config_array[DPU_MAX_NR_CIS] = {
			[0 ... DPU_MAX_NR_CIS - 1] = &config
		};

		LOG_RANK(VERBOSE, rank, "IRAM repair disabled");

		FF(ufi_select_all(rank, &mask));
		FF(ufi_iram_repair_config(rank, mask, config_array));
	}

end:
	return status;
}

static dpu_error_t dpu_wram_repair_config(struct dpu_rank_t *rank)
{
	dpu_error_t status;

	LOG_RANK(VERBOSE, rank, "");

	if (rank->description->configuration.do_wram_repair) {
		LOG_RANK(VERBOSE, rank, "WRAM repair enabled");
		// SRAM defects were filled at rank allocation
		FF(try_to_repair_wram(rank));
	} else {
		uint8_t each_wram_bank;
		uint8_t mask = ALL_CIS;
		// Default configuration
		struct dpu_repair_config config = { 0, 0, 0, 0, 0, 0, 0, 1 };
		struct dpu_repair_config *config_array[DPU_MAX_NR_CIS] = {
			[0 ... DPU_MAX_NR_CIS - 1] = &config
		};

		LOG_RANK(VERBOSE, rank, "WRAM repair disabled");
		FF(ufi_select_all(rank, &mask));
		for (each_wram_bank = 0; each_wram_bank < NR_OF_WRAM_BANKS;
		     ++each_wram_bank) {
			FF(ufi_wram_repair_config(rank, mask, each_wram_bank,
						  config_array));
		}
	}

end:
	return status;
}

static dpu_error_t dpu_clear_debug(struct dpu_rank_t *rank)
{
	dpu_error_t status;
	uint8_t mask = ALL_CIS;

	LOG_RANK(VERBOSE, rank, "");
	FF(ufi_select_all(rank, &mask));

	FF(ufi_clear_debug_replace(rank, mask));
	FF(ufi_clear_fault_poison(rank, mask));
	FF(ufi_clear_fault_bkp(rank, mask));
	FF(ufi_clear_fault_dma(rank, mask));
	FF(ufi_clear_fault_mem(rank, mask));
	FF(ufi_clear_fault_dpu(rank, mask));
	FF(ufi_clear_fault_intercept(rank, mask));

end:
	return status;
}

static dpu_error_t dpu_clear_run_bits(struct dpu_rank_t *rank)
{
	dpu_error_t status;
	uint32_t each_bit;
	uint8_t mask = ALL_CIS;
	uint32_t nr_run_bits = rank->description->hw.dpu.nr_of_threads +
			       rank->description->hw.dpu.nr_of_notify_bits;

	LOG_RANK(VERBOSE, rank, "");
	FF(ufi_select_all(rank, &mask));

	for (each_bit = 0; each_bit < nr_run_bits; ++each_bit) {
		FF(ufi_clear_run_bit(rank, mask, each_bit, NULL));
	}

end:
	return status;
}

static dpu_error_t dpu_set_pc_mode(struct dpu_rank_t *rank,
				   enum dpu_pc_mode pc_mode)
{
	dpu_error_t status;
	enum dpu_pc_mode pc_modes[DPU_MAX_NR_CIS] = { [0 ... DPU_MAX_NR_CIS -
						       1] = pc_mode };
	uint8_t mask = ALL_CIS;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	dpu_member_id_t dpu_id;
	dpu_slice_id_t slice_id;

	LOG_RANK(VERBOSE, rank, "%s", pc_mode_to_string(pc_mode));
	FF(ufi_select_all(rank, &mask));
	FF(ufi_set_pc_mode(rank, mask, pc_modes));

	for (dpu_id = 0; dpu_id < nr_dpus_per_ci; ++dpu_id) {
		mask = ALL_CIS;
		FF(ufi_select_dpu(rank, &mask, dpu_id));
		FF(ufi_get_pc_mode(rank, mask, pc_modes));

		for (slice_id = 0; slice_id < nr_cis; ++slice_id) {
			struct dpu_t *dpu =
				DPU_GET_UNSAFE(rank, slice_id, dpu_id);
			if (dpu->enabled) {
				if (pc_modes[dpu_id] != pc_mode) {
					LOG_DPU(WARNING, dpu,
						"ERROR: invalid PC mode (expected: %d, found: %d)",
						pc_mode, pc_modes[dpu_id]);
					status = DPU_ERR_INTERNAL;
					break;
				}
			}
		}
	}

end:
	return status;
}

static dpu_error_t dpu_set_stack_direction(struct dpu_rank_t *rank,
					   bool stack_is_up)
{
	dpu_error_t status;
	uint8_t previous_directions[DPU_MAX_NR_CIS];
	bool stack_directions[DPU_MAX_NR_CIS] = { [0 ... DPU_MAX_NR_CIS - 1] =
							  stack_is_up };
	uint8_t mask = ALL_CIS;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	dpu_member_id_t dpu_id;
	dpu_slice_id_t slice_id;

	LOG_RANK(VERBOSE, rank, "%s", stack_direction_to_string(stack_is_up));
	FF(ufi_select_all(rank, &mask));
	FF(ufi_set_stack_direction(rank, mask, stack_directions, NULL));
	FF(ufi_set_stack_direction(rank, mask, stack_directions,
				   previous_directions));

	for (slice_id = 0; slice_id < nr_cis; ++slice_id) {
		for (dpu_id = 0; dpu_id < nr_dpus_per_ci; ++dpu_id) {
			struct dpu_t *dpu =
				DPU_GET_UNSAFE(rank, slice_id, dpu_id);
			if (dpu->enabled) {
				bool stack_if_effectively_up =
					dpu_mask_is_selected(
						previous_directions[slice_id],
						dpu_id);
				if (stack_if_effectively_up != stack_is_up) {
					LOG_DPU(WARNING, dpu,
						"ERROR: invalid stack mode (expected: %s, found: %s)",
						stack_direction_to_string(
							stack_is_up),
						stack_direction_to_string(
							stack_if_effectively_up));
					status = DPU_ERR_INTERNAL;
				}
			}
		}
	}

end:
	return status;
}

static dpu_error_t dpu_reset_internal_state(struct dpu_rank_t *rank)
{
	dpu_error_t status;
	iram_size_t internal_state_reset_size;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t nr_threads = rank->description->hw.dpu.nr_of_threads;
	dpuinstruction_t *internal_state_reset =
		fetch_internal_reset_program(&internal_state_reset_size);
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS] = {
		[0 ... DPU_MAX_NR_CIS - 1] = internal_state_reset
	};
	uint8_t mask = ALL_CIS;
	uint8_t mask_all = (1 << nr_dpus) - 1;
	uint8_t state[DPU_MAX_NR_CIS];
	bool running;
	dpu_thread_t each_thread;

	LOG_RANK(VERBOSE, rank, "");

	if (internal_state_reset == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	FF(ufi_select_all(rank, &mask));
	FF(ufi_iram_write(rank, mask, iram_array, 0,
			  internal_state_reset_size));

	for (each_thread = 0; each_thread < nr_threads; ++each_thread) {
		/* Do not use dpu_thread_boot_safe_for_rank functions here as it would
         * increase reset duration for nothing.
         */
		FF(ufi_thread_boot(rank, mask, each_thread, NULL));
	}

	do {
		dpu_slice_id_t each_ci;

		FF(ufi_read_dpu_run(rank, mask, state));

		running = false;
		for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
			if (!CI_MASK_ON(mask, each_ci))
				continue;
			running = running || ((state[each_ci] & mask_all) != 0);
		}
	} while (running);

end:
	free(internal_state_reset);
	return status;
}

static dpu_error_t dpu_init_groups(struct dpu_rank_t *rank,
				   const bool *all_dpus_are_enabled_save,
				   const dpu_selected_mask_t *enabled_dpus_save)
{
	dpu_error_t status = DPU_OK;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t ci_mask = 0;
	dpu_slice_id_t each_ci;
	dpu_member_id_t each_dpu;

	LOG_RANK(VERBOSE, rank, "");

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		if (all_dpus_are_enabled_save[each_ci]) {
			ci_mask |= CI_MASK_ONE(each_ci);
		} else if (enabled_dpus_save[each_ci]) {
			for (each_dpu = 0; each_dpu < nr_dpus_per_ci;
			     ++each_dpu) {
				uint8_t single_ci_mask = CI_MASK_ONE(each_ci);
				uint8_t group = ((enabled_dpus_save[each_ci] &
						  (1 << each_dpu)) != 0) ?
							      DPU_ENABLED_GROUP :
							      DPU_DISABLED_GROUP;
				FF(ufi_select_dpu(rank, &single_ci_mask,
						  each_dpu));
				FF(ufi_write_group(rank, single_ci_mask,
						   group));
			}
		}
	}

	if (ci_mask != 0) {
		FF(ufi_select_all(rank, &ci_mask));
		FF(ufi_write_group(rank, ci_mask, DPU_ENABLED_GROUP));
	}

	/* Set the rank context with the saved context */
	rank->nr_dpus_enabled = 0;
	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		struct dpu_configuration_slice_info_t *ci_info =
			&rank->runtime.control_interface.slice_info[each_ci];

		ci_info->all_dpus_are_enabled =
			all_dpus_are_enabled_save[each_ci];
		ci_info->enabled_dpus = enabled_dpus_save[each_ci];
		rank->nr_dpus_enabled +=
			__builtin_popcount(ci_info->enabled_dpus);

		for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
			DPU_GET_UNSAFE(rank, each_ci, each_dpu)->enabled =
				(ci_info->enabled_dpus & (1 << each_dpu)) != 0;
		}
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_reset_rank(struct dpu_rank_t *rank)
{
	dpu_error_t status;
	dpu_description_t desc = rank->description;
	struct dpu_bit_config *bit_config = &desc->hw.dpu.pcb_transformation;

	struct dpu_dma_config std_dma_config;
	struct dpu_wavegen_config wavegen_config;
	const struct dpu_dma_config *dma_config =
		(rank->type == CYCLE_ACCURATE_SIMULATOR) ?
			      &dma_engine_cas_config :
			      &std_dma_config;

	bool all_dpus_are_enabled_save[DPU_MAX_NR_CIS];
	dpu_selected_mask_t enabled_dpus_save[DPU_MAX_NR_CIS];

	fetch_dma_and_wavegen_configs(desc->hw.timings.fck_frequency_in_mhz,
				      desc->hw.timings.clock_division,
				      REFRESH_MODE_VALUE,
				      !desc->configuration.ila_control_refresh,
				      &std_dma_config, &wavegen_config);

	/* All DPUs are enabled during the reset */
	save_enabled_dpus(rank, all_dpus_are_enabled_save, enabled_dpus_save,
			  false);

	FF(dpu_byte_order(rank));
	FF(dpu_soft_reset(rank, DPU_CLOCK_DIV8));
	FF(dpu_bit_config(rank, bit_config));
	FF(dpu_ci_shuffling_box_config(rank, bit_config));
	FF(dpu_soft_reset(rank, from_division_factor_to_dpu_enum(
					desc->hw.timings.clock_division)));
	FF(dpu_ci_shuffling_box_config(rank, bit_config));
	FF(dpu_identity(rank));
	FF(dpu_thermal_config(rank, desc->hw.timings.std_temperature));

	FF(dpu_carousel_config(rank, &desc->hw.timings.carousel));

	FF(dpu_iram_repair_config(rank));
	save_enabled_dpus(rank, all_dpus_are_enabled_save, enabled_dpus_save,
			  true);

	FF(dpu_wram_repair_config(rank));
	save_enabled_dpus(rank, all_dpus_are_enabled_save, enabled_dpus_save,
			  true);

	FF(dpu_dma_config(rank, dma_config));
	FF(dpu_dma_shuffling_box_config(rank, bit_config));
	FF(dpu_wavegen_config(rank, &wavegen_config));

	FF(dpu_clear_debug(rank));
	FF(dpu_clear_run_bits(rank));

	FF(dpu_set_pc_mode(rank, DPU_PC_MODE_16));
	FF(dpu_set_stack_direction(rank, true));
	FF(dpu_reset_internal_state(rank));

	FF(dpu_switch_mux_for_rank(
		rank, desc->configuration.api_must_switch_mram_mux));
	FF(dpu_init_groups(rank, all_dpus_are_enabled_save, enabled_dpus_save));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_reset_dpu(struct dpu_t *dpu)
{
	dpu_error_t status;
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;
	dpu_selected_mask_t mask_one = dpu_mask_one(member_id);

	/* There is no explicit reset command for a DPU.
     * We just stop the DPU, if it is running, and set the PCs to 0.
     */

	struct dpu_context_t context;
	struct _dpu_description_t *description = dpu_get_description(rank);

	uint8_t nr_of_dpu_threads = description->hw.dpu.nr_of_threads;
	dpu_thread_t each_thread;
	dpuinstruction_t stop_instruction = STOPci(BOOT_CC_TRUE, 0);

	uint8_t mask = CI_MASK_ONE(slice_id);
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS];
	uint8_t result[DPU_MAX_NR_CIS];

	context.scheduling = NULL;
	context.pcs = NULL;

	if ((context.scheduling = malloc(nr_of_dpu_threads *
					 sizeof(*(context.scheduling)))) ==
	    NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	if ((context.pcs = malloc(nr_of_dpu_threads *
				  sizeof(*(context.pcs)))) == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	for (each_thread = 0; each_thread < nr_of_dpu_threads; ++each_thread) {
		context.scheduling[each_thread] = 0xFF;
	}

	context.nr_of_running_threads = 0;
	context.bkp_fault = false;
	context.dma_fault = false;
	context.mem_fault = false;

	FF(dpu_custom_for_dpu(dpu, DPU_COMMAND_DPU_SOFT_RESET, NULL));

	FF(dpu_initialize_fault_process_for_dpu(dpu, &context));

	FF(ufi_select_dpu(rank, &mask, member_id));

	iram_array[slice_id] = &stop_instruction;
	FF(ufi_iram_write(rank, mask, iram_array, 0, 1));

	for (each_thread = 0; each_thread < description->hw.dpu.nr_of_threads;
	     ++each_thread) {
		FF(dpu_thread_boot_safe_for_dpu(dpu, each_thread, NULL, false));
	}

	do {
		FF(ufi_read_dpu_run(rank, mask, result));
	} while ((result[slice_id] & mask_one) != 0);

end:
	free(context.pcs);
	free(context.scheduling);
	return status;
}

__API_SYMBOL__ dpu_error_t ci_disable_dpu(struct dpu_t *dpu)
{
	dpu_error_t status;
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t dpu_id = dpu->dpu_id;

	uint8_t ci_mask = CI_MASK_ONE(slice_id);
	FF(ufi_select_dpu(rank, &ci_mask, dpu_id));
	FF(ufi_write_group(rank, ci_mask, DPU_DISABLED_GROUP));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t dpu_wavegen_read_status(struct dpu_t *dpu,
						   uint8_t address,
						   uint8_t *value)
{
	dpu_error_t status = DPU_OK;

	struct dpu_rank_t *rank = dpu->rank;
	uint8_t mask = CI_MASK_ONE(dpu->slice_id);
	uint8_t results[DPU_MAX_NR_CIS];

	LOG_DPU(DEBUG, dpu, "%d", address);

	if (!dpu->enabled) {
		return DPU_ERR_DPU_DISABLED;
	}

	dpu_lock_rank(rank);

	FF(ufi_select_dpu(rank, &mask, dpu->dpu_id));
	FF(ufi_write_dma_ctrl(rank, mask, 0xFF, address & 3));
	FF(ufi_clear_dma_ctrl(rank, mask));
	FF(ufi_read_dma_ctrl(rank, mask, results));

	*value = results[dpu->slice_id];

end:
	dpu_unlock_rank(rank);
	return status;
}

#define TIMEOUT_MUX_STATUS 100
#define CMD_GET_MUX_CTRL 0x02
static dpu_error_t dpu_check_wavegen_mux_status_for_dpu(struct dpu_rank_t *rank,
							uint8_t dpu_id,
							uint8_t *expected)
{
	dpu_error_t status;
	uint8_t dpu_dma_ctrl;
	uint8_t result_array[DPU_MAX_NR_CIS];
	uint32_t timeout = TIMEOUT_MUX_STATUS;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t ci_mask = ALL_CIS;
	bool should_retry;

	LOG_RANK(VERBOSE, rank, "[DPU %d]", dpu_id);

	FF(ufi_select_dpu_even_disabled(rank, &ci_mask, dpu_id));

	// Check Mux control through dma_rdat_ctrl of fetch1
	// 1 - Select WaveGen Read register @0xFF and set it @0x02  (mux and collision ctrl)
	// 2 - Flush readop2 (Pipeline to DMA cfg data path)
	// 3 - Read dpu_dma_ctrl
	FF(ufi_write_dma_ctrl(rank, ci_mask, 0xFF, CMD_GET_MUX_CTRL));
	FF(ufi_clear_dma_ctrl(rank, ci_mask));

	do {
		dpu_slice_id_t each_slice;
		should_retry = false;

		FF(ufi_read_dma_ctrl(rank, ci_mask, result_array));
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			if (!CI_MASK_ON(ci_mask, each_slice))
				continue;

			dpu_dma_ctrl = result_array[each_slice];

			// Expected 0x3 for DPU4 since it is the only one to be refreshed
			// Expected 0x0 for others DPU since no refresh has been issued
			LOG_RANK(
				VERBOSE, rank,
				"[DPU %hhu:%hhu] XMA Init = 0x%02x (expected = 0x%02x)",
				each_slice, dpu_id, dpu_dma_ctrl,
				expected[each_slice]);

			if ((dpu_dma_ctrl & 0x7F) != expected[each_slice]) {
				should_retry = true;
				break;
			}
		}

		timeout--;
	} while (timeout && should_retry); // Do not check Collision Error bit

	if (!timeout) {
		LOG_RANK(WARNING, rank,
			 "Timeout waiting for result to be correct");
		return rank->description->configuration.disable_api_safe_checks ?
				     DPU_OK :
				     DPU_ERR_TIMEOUT;
	}

end:
	return status;
}

static dpu_error_t
dpu_check_wavegen_mux_status_for_rank(struct dpu_rank_t *rank, uint8_t expected)
{
	dpu_error_t status;
	uint8_t dpu_dma_ctrl;
	uint8_t result_array[DPU_MAX_NR_CIS];
	uint32_t timeout;
	uint8_t ci_mask = ALL_CIS, mask;
	uint8_t nr_dpus =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	bool should_retry;
	dpu_member_id_t each_dpu;

	LOG_RANK(VERBOSE, rank, "");

	/* ci_mask retains the real disabled CIs, whereas mask does not take
     * care of disabled dpus (and then CIs) since it should switch mux of
     * disabled dpus: but not in the case a CI is completely deactivated.
     */

	// Check Mux control through dma_rdat_ctrl of fetch1
	// 1 - Select WaveGen Read register @0xFF and set it @0x02  (mux and collision ctrl)
	// 2 - Flush readop2 (Pipeline to DMA cfg data path)
	// 3 - Read dpu_dma_ctrl
	ci_mask = ALL_CIS;
	FF(ufi_select_all_even_disabled(rank, &ci_mask));
	FF(ufi_write_dma_ctrl(rank, ci_mask, 0xFF, CMD_GET_MUX_CTRL));
	FF(ufi_clear_dma_ctrl(rank, ci_mask));

	for (each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
		timeout = TIMEOUT_MUX_STATUS;

		do {
			dpu_slice_id_t each_slice;
			should_retry = false;

			mask = ALL_CIS;
			FF(ufi_select_dpu_even_disabled(rank, &mask, each_dpu));
			FF(ufi_read_dma_ctrl(rank, mask, result_array));

			for (each_slice = 0; each_slice < nr_cis;
			     ++each_slice) {
				if (!CI_MASK_ON(ci_mask, each_slice))
					continue;

				dpu_dma_ctrl = result_array[each_slice];

				if ((dpu_dma_ctrl & 0x7F) != expected) {
					LOG_RANK(VERBOSE, rank,
						 "DPU (%d, %d) failed",
						 each_slice, each_dpu);
					should_retry = true;
				}
			}

			timeout--;
		} while (timeout &&
			 should_retry); // Do not check Collision Error bit

		if (!timeout) {
			LOG_RANK(WARNING, rank,
				 "Timeout waiting for result to be correct");
			return rank->description->configuration
					       .disable_api_safe_checks ?
					     DPU_OK :
					     DPU_ERR_TIMEOUT;
		}
	}

end:
	return status;
}

static dpu_error_t host_handle_access_for_dpu(struct dpu_rank_t *rank,
					      uint8_t dpu_id,
					      dpu_ci_bitfield_t ci_mux_pos)
{
	dpu_error_t status;

	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t ci_mask = ALL_CIS, expected[DPU_MAX_NR_CIS];
	dpu_slice_id_t slice_id;

	FF(ufi_select_dpu_even_disabled(rank, &ci_mask, dpu_id));
	FF(ufi_set_mram_mux(rank, ci_mask, ci_mux_pos));

	for (slice_id = 0; slice_id < nr_cis; ++slice_id)
		expected[slice_id] = (ci_mux_pos & (1 << slice_id)) ?
						   WAVEGEN_MUX_HOST_EXPECTED :
						   WAVEGEN_MUX_DPU_EXPECTED;

	FF(dpu_check_wavegen_mux_status_for_dpu(rank, dpu_id, expected));

end:
	return status;
}

static dpu_error_t host_handle_access_for_rank(struct dpu_rank_t *rank,
					       bool set_mux_for_host)
{
	dpu_error_t status;

	uint8_t mask = ALL_CIS;

	FF(ufi_select_all_even_disabled(rank, &mask));
	FF(ufi_set_mram_mux(rank, mask, set_mux_for_host ? 0xFF : 0x0));

	FF(dpu_check_wavegen_mux_status_for_rank(
		rank, set_mux_for_host ? WAVEGEN_MUX_HOST_EXPECTED :
					       WAVEGEN_MUX_DPU_EXPECTED));

end:
	return status;
}

/*
 * Do care about pairs of dpus: switch mux according to mask for both lines.
 */
__API_SYMBOL__ dpu_error_t dpu_switch_mux_for_dpu_line(struct dpu_rank_t *rank,
						       uint8_t dpu_id,
						       uint8_t mask)
{
	dpu_error_t status = DPU_OK;
	dpu_member_id_t dpu_pair_base_id = (dpu_member_id_t)(dpu_id & ~1);
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;

	/* Check if switching mux is needed */
	bool switch_base_line = false;
	bool switch_friend_line = false;

	uint8_t mux_mram_mask = 0;
	dpu_slice_id_t each_slice;
	uint32_t nb_dpu_running = 0;
	dpu_bitfield_t run_mask = dpu_mask_empty();

	LOG_RANK(VERBOSE, rank, "[DPU %d]", dpu_id);

	dpu_lock_rank(rank);

	for (each_slice = 0; each_slice < nr_cis; ++each_slice)
		mux_mram_mask |= dpu_get_host_mux_mram_state(rank, each_slice,
							     dpu_pair_base_id)
				 << each_slice;

	switch_base_line = !(mux_mram_mask == mask);

	if ((dpu_pair_base_id + 1) <
	    rank->description->hw.topology.nr_of_dpus_per_control_interface) {
		mux_mram_mask = 0;
		for (each_slice = 0; each_slice < nr_cis; ++each_slice)
			mux_mram_mask |=
				dpu_get_host_mux_mram_state(
					rank, each_slice, dpu_pair_base_id + 1)
				<< each_slice;

		switch_friend_line = !(mux_mram_mask == mask);
	}

	if (!switch_base_line && !switch_friend_line) {
		LOG_RANK(VERBOSE, rank,
			 "Mux is in the right direction, nothing to do.");
		goto end;
	}

	if (switch_base_line) {
		run_mask = dpu_mask_select(run_mask, dpu_pair_base_id);
	}
	if (switch_friend_line) {
		run_mask = dpu_mask_select(run_mask, dpu_pair_base_id + 1);
	}
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		dpu_bitfield_t run_bits = dpu_mask_intersection(
			rank->runtime.run_context.dpu_running[each_slice],
			run_mask);
		nb_dpu_running += dpu_mask_count(run_bits);
	}
	if (nb_dpu_running > 0) {
		LOG_RANK(
			WARNING, rank,
			"Host can not get access to the MRAM because %u DPU%s running.",
			nb_dpu_running, nb_dpu_running > 1 ? "s are" : " is");
		status = DPU_ERR_MRAM_BUSY;
		goto end;
	}

	/* Update mux state:
     * We must switch the mux at host side for the current dpu but switch the mux at dpu side
     * for other dpus of the same line.
     * We record the state before actually placing the mux in this state. If we did record the
     * state after setting the mux, we could be interrupted between the setting of the mux and
     * the recording of the state, and then the debugger would miss a mux state.
     */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		if (switch_base_line)
			dpu_set_host_mux_mram_state(rank, each_slice,
						    dpu_pair_base_id,
						    mask & (1 << each_slice));
		if (switch_friend_line)
			dpu_set_host_mux_mram_state(rank, each_slice,
						    dpu_pair_base_id + 1,
						    mask & (1 << each_slice));
	}

	if (!rank->description->configuration.api_must_switch_mram_mux)
		goto end;

	if (switch_base_line)
		FF(host_handle_access_for_dpu(rank, dpu_pair_base_id, mask));
	if (switch_friend_line)
		FF(host_handle_access_for_dpu(rank, dpu_pair_base_id + 1,
					      mask));

end:
	dpu_unlock_rank(rank);

	return status;
}

__API_SYMBOL__ dpu_error_t dpu_switch_mux_for_rank(struct dpu_rank_t *rank,
						   bool set_mux_for_host)
{
	dpu_error_t status = DPU_OK;

	dpu_description_t desc = rank->description;
	uint8_t nr_cis = desc->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		desc->hw.topology.nr_of_dpus_per_control_interface;
	bool switch_mux = false;
	dpu_slice_id_t each_slice;

	LOG_RANK(VERBOSE, rank, "");

	dpu_lock_rank(rank);
	if (rank->runtime.run_context.nb_dpu_running > 0) {
		LOG_RANK(
			WARNING, rank,
			"Host can not get access to the MRAM because %u DPU%s running.",
			rank->runtime.run_context.nb_dpu_running,
			rank->runtime.run_context.nb_dpu_running > 1 ? "s are" :
									     " is");
		status = DPU_ERR_MRAM_BUSY;
		goto end;
	}

	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		if ((set_mux_for_host &&
		     __builtin_popcount(rank->runtime.control_interface
						.slice_info[each_slice]
						.host_mux_mram_state) <
			     nr_dpus_per_ci) ||
		    (!set_mux_for_host &&
		     rank->runtime.control_interface.slice_info[each_slice]
			     .host_mux_mram_state)) {
			LOG_RANK(
				VERBOSE, rank,
				"At least CI %d has mux in the wrong direction (0x%x), must switch rank.",
				each_slice,
				rank->runtime.control_interface
					.slice_info[each_slice]
					.host_mux_mram_state);
			switch_mux = true;
			break;
		}
	}

	if (!switch_mux) {
		LOG_RANK(VERBOSE, rank,
			 "Mux is in the right direction, nothing to do.");
		goto end;
	}

	/* We record the state before actually placing the mux in this state. If we
     * did record the state after setting the mux, we could be interrupted between
     * the setting of the mux and the recording of the state, and then the debugger
     * would miss a mux state.
     */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice)
		rank->runtime.control_interface.slice_info[each_slice]
			.host_mux_mram_state =
			set_mux_for_host ? (1 << nr_dpus_per_ci) - 1 : 0x0;

	if (!rank->description->configuration.api_must_switch_mram_mux &&
	    !rank->description->configuration.init_mram_mux) {
		status = DPU_OK;
		goto end;
	}

	FF(host_handle_access_for_rank(rank, set_mux_for_host));

end:
	dpu_unlock_rank(rank);

	return status;
}
