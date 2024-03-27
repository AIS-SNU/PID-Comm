/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <ufi/ufi_ci_types.h>
#include <ufi/ufi_dma_wavegen_config.h>
#include <ufi/ufi.h>
#include <dpu_types.h>
#include <ufi_rank_utils.h>

__API_SYMBOL__ const struct dpu_dma_config dma_engine_cas_config = {
	.refresh_access_number = 53,
	.column_read_latency = 7,
	.minimal_access_number = 0,
	.default_time_origin = -4,
	.ldma_to_sdma_time_origin = -10,
	.xdma_time_start_activate = 9,
	.xdma_time_start_access = 18,
	.sdma_time_start_wb_f1 = 0,
};

/* Configuration for DPU_CLOCK_DIV2 */
__API_SYMBOL__ const struct dpu_dma_config dma_engine_clock_div2_config = {
	/* refresh_access_number computed later depending on frequency */
	.column_read_latency = 9,      .minimal_access_number = 0,
	.default_time_origin = -4,     .ldma_to_sdma_time_origin = -10,
	.xdma_time_start_activate = 9, .xdma_time_start_access = 18,
	.sdma_time_start_wb_f1 = 0,
};

__API_SYMBOL__ const struct dpu_wavegen_config waveform_generator_clock_div2_config = {
    .MCBAB    = { .rise = 0x00, .fall = 0x39, .counter_enable = 0x01, .counter_disable = 0x01 },
    .RCYCLKB  = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x01, .counter_disable = 0x03 },
    .WCYCLKB  = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x02, .counter_disable = 0x03 },
    .DWCLKB   = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x01, .counter_disable = 0x02 },
    .DWAEB    = { .rise = 0x31, .fall = 0x08, .counter_enable = 0x02, .counter_disable = 0x04 },
    .DRAPB    = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x01, .counter_disable = 0x03 },
    .DRAOB    = { .rise = 0x31, .fall = 0x39, .counter_enable = 0x03, .counter_disable = 0x04 },
    .DWBSB_EN = { .rise = 0x3C, .fall = 0x3E, .counter_enable = 0x01, .counter_disable = 0x03 },
    .vector_sampling = { .rab_gross = 0x01, .cat_gross = 0x01, .dwbsb_gross = 0x01, .drbsb_gross = 0x01, .drbsb_fine = 0x02 },
    .timing_completion = {.activate = 0x0A, .read = 0x03, .write = 0x03, .precharge = 0x0B,
        /* refresh_start, refresh_activ, refresh_prech, refresh_end computed later depending on frequency */
        },
    /* refresh_and_row_hammer_info computed later depending on frequency */
    .row_hammer_config = 0x06,
    .refresh_ctrl = 0x01,
};

/* Configuration for DPU_CLOCK_DIV3 */
__API_SYMBOL__ const struct dpu_dma_config dma_engine_clock_div3_config = {
	/* refresh_access_number computed later depending on frequency */
	.column_read_latency = 8,      .minimal_access_number = 0,
	.default_time_origin = -4,     .ldma_to_sdma_time_origin = -10,
	.xdma_time_start_activate = 9, .xdma_time_start_access = 18,
	.sdma_time_start_wb_f1 = 0,
};

__API_SYMBOL__ const struct dpu_wavegen_config waveform_generator_clock_div3_config = {
    .MCBAB    = { .rise = 0x00, .fall = 0x39, .counter_enable = 0x02, .counter_disable = 0x02 },
    .RCYCLKB  = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x02, .counter_disable = 0x04 },
    .WCYCLKB  = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x03, .counter_disable = 0x04 },
    .DWCLKB   = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x02, .counter_disable = 0x03 },
    .DWAEB    = { .rise = 0x31, .fall = 0x08, .counter_enable = 0x03, .counter_disable = 0x05 },
    .DRAPB    = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x02, .counter_disable = 0x04 },
    .DRAOB    = { .rise = 0x31, .fall = 0x39, .counter_enable = 0x04, .counter_disable = 0x05 },
    .DWBSB_EN = { .rise = 0x3C, .fall = 0x3E, .counter_enable = 0x02, .counter_disable = 0x04 },
    .vector_sampling = { .rab_gross = 0x01, .cat_gross = 0x01, .dwbsb_gross = 0x01, .drbsb_gross = 0x01, .drbsb_fine = 0x02 },
    .timing_completion = { .activate = 0x0F, .read = 0x05, .write = 0x05, .precharge = 0x0B,
        /* refresh_start, refresh_activ, refresh_prech, refresh_end computed later depending on frequency */
        },
    /* refresh_and_row_hammer_info computed later depending on frequency */
    .row_hammer_config = 0x06,
    .refresh_ctrl = 0x01,
};

/* Configuration for DPU_CLOCK_DIV4 */
__API_SYMBOL__ const struct dpu_dma_config dma_engine_clock_div4_config = {
	/* refresh_access_number computed later depending on frequency */
	.column_read_latency = 7,      .minimal_access_number = 0,
	.default_time_origin = -4,     .ldma_to_sdma_time_origin = -10,
	.xdma_time_start_activate = 9, .xdma_time_start_access = 18,
	.sdma_time_start_wb_f1 = 0,
};

__API_SYMBOL__ const struct dpu_wavegen_config waveform_generator_clock_div4_config = {
    .MCBAB    = { .rise = 0x00, .fall = 0x39, .counter_enable = 0x03, .counter_disable = 0x03 },
    .RCYCLKB  = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x03, .counter_disable = 0x05 },
    .WCYCLKB  = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x04, .counter_disable = 0x05 },
    .DWCLKB   = { .rise = 0x31, .fall = 0x00, .counter_enable = 0x03, .counter_disable = 0x04 },
    .DWAEB    = { .rise = 0x31, .fall = 0x08, .counter_enable = 0x04, .counter_disable = 0x06 },
    .DRAPB    = { .rise = 0x33, .fall = 0x39, .counter_enable = 0x03, .counter_disable = 0x05 },
    .DRAOB    = { .rise = 0x31, .fall = 0x39, .counter_enable = 0x05, .counter_disable = 0x06 },
    .DWBSB_EN = { .rise = 0x3C, .fall = 0x3E, .counter_enable = 0x03, .counter_disable = 0x05 },
    .vector_sampling = { .rab_gross = 0x02, .cat_gross = 0x02, .dwbsb_gross = 0x01, .drbsb_gross = 0x01, .drbsb_fine = 0x02 },
    .timing_completion = { .activate = 0x14, .read = 0x07, .write = 0x07, .precharge = 0x0B,
        /* refresh_start, refresh_activ, refresh_prech, refresh_end computed later depending on frequency */
        },
    /* refresh_and_row_hammer_info computed later depending on frequency */
    .row_hammer_config = 0x06,
    .refresh_ctrl = 0x01,
};

__API_SYMBOL__ u32 dpu_dma_shuffling_box_config(
	struct dpu_rank_t *rank, const struct dpu_bit_config *config)
{
	u32 status;
	u8 mask = ALL_CIS;

	LOG_RANK(
		VERBOSE, rank,
		"DMA shuffling box config: cpu_to_dpu: 0x%04x dpu_to_cpu: 0x%04x nibble_swap: 0x%02x",
		config->cpu2dpu, config->dpu2cpu, config->nibble_swap);

	FF(ufi_select_all(rank, &mask));

	// Configure Jedec Shuffling box
	FF(ufi_write_dma_ctrl(rank, mask, 0x10, (config->dpu2cpu >> 8) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, 0x11, (config->dpu2cpu >> 0) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, 0x12, (config->cpu2dpu >> 8) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, 0x13, (config->cpu2dpu >> 0) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, 0x14, config->nibble_swap));

	// Clear DMA Engine Configuration Path and flush reg_replace_instr of readop2
	FF(ufi_clear_dma_ctrl(rank, mask));
end:
	return status;
}

static u32 dpu_wavegen_reg_config(struct dpu_rank_t *rank, u8 mask, u8 address,
				  const struct dpu_wavegen_reg_config *config)
{
	u32 status;

	FF(ufi_write_dma_ctrl(rank, mask, address + 0, config->rise));
	FF(ufi_write_dma_ctrl(rank, mask, address + 1, config->fall));
	FF(ufi_write_dma_ctrl(rank, mask, address + 2, config->counter_enable));
	FF(ufi_write_dma_ctrl(rank, mask, address + 3,
			      config->counter_disable));

end:
	return status;
}

static u32
dpu_wavegen_timing_config(struct dpu_rank_t *rank, u8 mask, u8 address,
			  const struct dpu_wavegen_timing_config *config)
{
	u32 status;

	FF(ufi_write_dma_ctrl(rank, mask, address + 0, config->activate));
	FF(ufi_write_dma_ctrl(rank, mask, address + 1, config->read));
	FF(ufi_write_dma_ctrl(rank, mask, address + 2, config->write));
	FF(ufi_write_dma_ctrl(rank, mask, address + 3, config->precharge));
	FF(ufi_write_dma_ctrl(rank, mask, address + 4, config->refresh_start));
	FF(ufi_write_dma_ctrl(rank, mask, address + 5, config->refresh_activ));
	FF(ufi_write_dma_ctrl(rank, mask, address + 6, config->refresh_prech));
	FF(ufi_write_dma_ctrl(rank, mask, address + 7, config->refresh_end));

end:
	return status;
}

static u32 dpu_wavegen_vector_sampling_config(
	struct dpu_rank_t *rank, u8 mask, u8 address,
	const struct dpu_wavegen_vector_sampling_config *config)
{
	u32 status;

	FF(ufi_write_dma_ctrl(rank, mask, address + 0, config->rab_gross));
	FF(ufi_write_dma_ctrl(rank, mask, address + 1, config->cat_gross));
	FF(ufi_write_dma_ctrl(rank, mask, address + 2, config->dwbsb_gross));
	FF(ufi_write_dma_ctrl(rank, mask, address + 3, config->drbsb_gross));
	FF(ufi_write_dma_ctrl(rank, mask, address + 4, config->drbsb_fine));

end:
	return status;
}

static u32 dpu_wavegen_rowhammer_and_refresh_config(struct dpu_rank_t *rank,
						    u8 mask, u8 address,
						    u16 rowhammer_info,
						    u8 rowhammer_config)
{
	u32 status;

	FF(ufi_write_dma_ctrl(rank, mask, address + 0,
			      (rowhammer_info >> 0) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, address + 1,
			      (rowhammer_info >> 8) & 0xFF));
	FF(ufi_write_dma_ctrl(rank, mask, address + 2, rowhammer_config));

end:
	return status;
}

__API_SYMBOL__ u32 dpu_wavegen_config(struct dpu_rank_t *rank,
				      const struct dpu_wavegen_config *config)
{
	u32 status;
	u8 mask = ALL_CIS, each_dpu;

	LOG_RANK(VERBOSE, rank, "");

	FF(ufi_select_all(rank, &mask));

	FF(dpu_wavegen_reg_config(rank, mask, 0xC0, &config->MCBAB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xC4, &config->RCYCLKB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xC8, &config->WCYCLKB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xCC, &config->DWCLKB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xD0, &config->DWAEB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xD4, &config->DRAPB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xD8, &config->DRAOB));
	FF(dpu_wavegen_reg_config(rank, mask, 0xDC, &config->DWBSB_EN));

	FF(dpu_wavegen_timing_config(rank, mask, 0xE0,
				     &config->timing_completion));
	FF(dpu_wavegen_vector_sampling_config(rank, mask, 0xE8,
					      &config->vector_sampling));
	FF(dpu_wavegen_rowhammer_and_refresh_config(
		rank, mask, 0xFC, config->refresh_and_row_hammer_info,
		config->row_hammer_config));

	for (each_dpu = 0;
	     each_dpu <
	     GET_DESC_HW(rank)->topology.nr_of_dpus_per_control_interface;
	     ++each_dpu) {
		// Configuration MUST start with dpu_id = 4 because of refresh
		u8 target_dpu;

		if (GET_DESC_HW(rank)
			    ->topology.nr_of_dpus_per_control_interface <= 4)
			target_dpu = each_dpu;
		else
			target_dpu = (each_dpu >= 4) ? (each_dpu - 4) :
							     (each_dpu + 4);

		mask = ALL_CIS;
		FF(ufi_select_dpu(rank, &mask, target_dpu));
		FF(ufi_write_dma_ctrl(rank, mask, 0x83, config->refresh_ctrl));
		FF(ufi_clear_dma_ctrl(rank, mask));
	}

end:
	return status;
}

static inline u32 nr_cycles_for(u64 duration_in_ns_tenfold,
				u32 frequency_in_mhz)
{
	return DIV_ROUND_UP_ULL(duration_in_ns_tenfold * frequency_in_mhz,
				1000);
}

static void
fill_timing_configuration(struct dpu_dma_config *dma_configuration,
			  struct dpu_wavegen_config *wavegen_configuration,
			  u32 fck_frequency, u8 fck_division, u8 refresh_mode)
{
#define REFRESH_START_VALUE (2)
#define REFRESH_END_VALUE (2)

#define REFRESH_ACTIVATE_DURATION_TENFOLD_NS 512 // 51.2 * 10
#define REFRESH_PRECHARGE_DURATION_TENFOLD_NS 269 // 26.9 * 10

#define REFRESH_NR_PULSES_DEFAULT (4)

	u32 tRFC;
	u32 refresh_mode_log2;
	/*
     * Assume that fck_frequency *is* even and greater than 100 so no need to use
     * double here since the following computation will result in integral anyway.
     * In addition, we divide here by 10 so that REFRESH_ACTIVATE_DURATION and
     * REFRESH_PRECHARGE_DURATION are integral too.
     */
	u32 half_tenth_fck_frequency = fck_frequency / (2 * 10);
	u32 refresh_start = REFRESH_START_VALUE;
	u32 refresh_activate =
		nr_cycles_for(REFRESH_ACTIVATE_DURATION_TENFOLD_NS,
			      half_tenth_fck_frequency) -
		1;
	u32 refresh_precharge =
		nr_cycles_for(REFRESH_PRECHARGE_DURATION_TENFOLD_NS,
			      half_tenth_fck_frequency) -
		1;
	u32 refresh_end = REFRESH_END_VALUE;
	u32 refresh_nr_pulses = REFRESH_NR_PULSES_DEFAULT / refresh_mode;
	u32 refresh;
	u32 refresh_dma;
	bool config_ok;

	switch (refresh_mode) {
	case 1:
		tRFC = 345;
		refresh_mode_log2 = 0;
		break;
	case 2:
		tRFC = 255;
		refresh_mode_log2 = 1;
		break;
	default:
		tRFC = 155;
		refresh_mode_log2 = 2;
		break;
	}

	do {
		u32 refresh_cycles_total, dma_cycles;

		refresh = refresh_start + 1 +
			  refresh_nr_pulses * (refresh_activate + 1) +
			  (refresh_nr_pulses - 1) * (refresh_precharge + 1) +
			  nr_cycles_for(14 * 10, half_tenth_fck_frequency);

		refresh_dma = DIV_ROUND_DOWN_ULL(tRFC * fck_frequency,
						 4 * fck_division * 1000) -
			      1;

		refresh_cycles_total =
			1 + refresh_start + 1 +
			refresh_nr_pulses * (refresh_activate + 1) +
			refresh_nr_pulses * (refresh_precharge + 1) +
			refresh_end + 1;

		dma_cycles = (refresh_dma + 1) * 4 * fck_division / 2;

		if (dma_cycles <= refresh_cycles_total) {
			refresh_activate--;
			refresh_precharge--;
			config_ok = false;
		} else {
			config_ok = true;
		}
	} while (!config_ok);

	dma_configuration->refresh_access_number = refresh_dma;
	wavegen_configuration->timing_completion.refresh_start = refresh_start;
	wavegen_configuration->timing_completion.refresh_activ =
		refresh_activate;
	wavegen_configuration->timing_completion.refresh_prech =
		refresh_precharge;
	wavegen_configuration->timing_completion.refresh_end = refresh_end;
	wavegen_configuration->refresh_and_row_hammer_info =
		(((refresh >> 8) & 1) << 15) |
		((refresh_mode_log2 & 0x3) << 8) | (refresh & 0xFF);
}

dpu_clock_division_t from_division_factor_to_dpu_enum(uint8_t factor)
{
	switch (factor) {
	default:
		return DPU_CLOCK_DIV2;
	case 3:
		return DPU_CLOCK_DIV3;
	case 4:
		return DPU_CLOCK_DIV4;
	case 8:
		return DPU_CLOCK_DIV8;
	}
}

__API_SYMBOL__ void
fetch_dma_and_wavegen_configs(u32 fck_frequency, u8 clock_division,
			      u8 refresh_mode, bool ctrl_refresh,
			      struct dpu_dma_config *dma_config,
			      struct dpu_wavegen_config *wavegen_config)
{
	u8 clock_div = from_division_factor_to_dpu_enum(clock_division);
	const struct dpu_dma_config *reference_dma_config;
	const struct dpu_wavegen_config *reference_wavegen_config;

	switch (clock_div) {
	case DPU_CLOCK_DIV4: {
		reference_dma_config = &dma_engine_clock_div4_config;
		reference_wavegen_config =
			&waveform_generator_clock_div4_config;
		break;
	}
	case DPU_CLOCK_DIV3: {
		reference_dma_config = &dma_engine_clock_div3_config;
		reference_wavegen_config =
			&waveform_generator_clock_div3_config;
		break;
	}
	default /*DPU_CLOCK_DIV2 (and the MRAM invalid DPU_CLOCK_DIV8 config)*/: {
		reference_dma_config = &dma_engine_clock_div2_config;
		reference_wavegen_config =
			&waveform_generator_clock_div2_config;
		clock_division = 2;
		break;
	}
	}

	memcpy(dma_config, reference_dma_config, sizeof(*dma_config));
	memcpy(wavegen_config, reference_wavegen_config,
	       sizeof(*wavegen_config));

	fill_timing_configuration(dma_config, wavegen_config, fck_frequency,
				  clock_division, refresh_mode);

	if (!ctrl_refresh) {
		wavegen_config->refresh_ctrl = 0x0;
	}
}

__API_SYMBOL__ u32 dpu_dma_config(struct dpu_rank_t *rank,
				  const struct dpu_dma_config *config)
{
	u64 dma_engine_timing = 0L;
	u32 status;
	u8 mask;

	LOG_RANK(
		VERBOSE, rank,
		"DMA engine config: refresh_access_number: %d column_read_latency: %d minimal_access_number: %d "
		"default_time_origin: %d ldma_to_sdma_time_origin: %d xdma_time_start_activate: %d "
		"xdma_time_start_access: %d sdma_time_start_wb_f1: %d",
		config->refresh_access_number, config->column_read_latency,
		config->minimal_access_number, config->default_time_origin,
		config->ldma_to_sdma_time_origin,
		config->xdma_time_start_activate,
		config->xdma_time_start_access, config->sdma_time_start_wb_f1);

	dma_engine_timing |= (((u64)config->refresh_access_number) & 0x7Ful)
			     << 36u;
	dma_engine_timing |= (((u64)config->column_read_latency) & 0x0Ful)
			     << 32u;
	dma_engine_timing |= (((u64)config->minimal_access_number) & 0x07ul)
			     << 29u;
	dma_engine_timing |= (((u64)config->default_time_origin) & 0x7Ful)
			     << 22u;
	dma_engine_timing |= (((u64)config->ldma_to_sdma_time_origin) & 0x7Ful)
			     << 15u;
	dma_engine_timing |= (((u64)config->xdma_time_start_activate) & 0x1Ful)
			     << 10u;
	dma_engine_timing |= (((u64)config->xdma_time_start_access) & 0x1Ful)
			     << 5u;
	dma_engine_timing |= (((u64)config->sdma_time_start_wb_f1) & 0x1Ful)
			     << 0u;

	mask = ALL_CIS;
	FF(ufi_select_all(rank, &mask));

	// Configure DMA Engine Timing
	FF(ufi_write_dma_ctrl(rank, mask, 0x20,
			      (dma_engine_timing >> 0u) & 0xFFu));
	FF(ufi_write_dma_ctrl(rank, mask, 0x21,
			      (dma_engine_timing >> 8u) & 0xFFu));
	FF(ufi_write_dma_ctrl(rank, mask, 0x22,
			      (dma_engine_timing >> 16u) & 0xFFu));
	FF(ufi_write_dma_ctrl(rank, mask, 0x23,
			      (dma_engine_timing >> 24u) & 0xFFu));
	FF(ufi_write_dma_ctrl(rank, mask, 0x24,
			      (dma_engine_timing >> 32u) & 0xFFu));
	FF(ufi_write_dma_ctrl(rank, mask, 0x25,
			      (dma_engine_timing >> 40u) & 0xFFu));

	// Clear DMA Engine Configuration Path and flush reg_replace_instr of readop2
	FF(ufi_clear_dma_ctrl(rank, mask));

end:
	return status;
}
