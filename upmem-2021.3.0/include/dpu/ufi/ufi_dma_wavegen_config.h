/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __DMA_WAVEGEN_CONFIG_H__
#define __DMA_WAVEGEN_CONFIG_H__

#include <ufi/ufi_ci_types.h>
#include <dpu_types.h>

struct dpu_dma_config {
	uint8_t refresh_access_number;
	uint8_t column_read_latency;
	uint8_t minimal_access_number;
	uint8_t default_time_origin;
	uint8_t ldma_to_sdma_time_origin;
	uint8_t xdma_time_start_activate;
	uint8_t xdma_time_start_access;
	uint8_t sdma_time_start_wb_f1;
};

struct dpu_wavegen_reg_config {
	uint8_t rise;
	uint8_t fall;
	uint8_t counter_enable;
	uint8_t counter_disable;
};

struct dpu_wavegen_timing_config {
	uint8_t activate;
	uint8_t read;
	uint8_t write;
	uint8_t precharge;
	uint8_t refresh_start;
	uint8_t refresh_activ;
	uint8_t refresh_prech;
	uint8_t refresh_end;
};

struct dpu_wavegen_vector_sampling_config {
	uint8_t rab_gross;
	uint8_t cat_gross;
	uint8_t dwbsb_gross;
	uint8_t drbsb_gross;
	uint8_t drbsb_fine;
};

struct dpu_wavegen_config {
	struct dpu_wavegen_reg_config MCBAB;
	struct dpu_wavegen_reg_config RCYCLKB;
	struct dpu_wavegen_reg_config WCYCLKB;
	struct dpu_wavegen_reg_config DWCLKB;
	struct dpu_wavegen_reg_config DWAEB;
	struct dpu_wavegen_reg_config DRAPB;
	struct dpu_wavegen_reg_config DRAOB;
	struct dpu_wavegen_reg_config DWBSB_EN;
	struct dpu_wavegen_timing_config timing_completion;
	struct dpu_wavegen_vector_sampling_config vector_sampling;
	uint16_t refresh_and_row_hammer_info; // {...[8], 4'h0 rah_auto, ref_mode[1:0]} pmcrft counter[7:0]
	uint8_t row_hammer_config;
	uint8_t refresh_ctrl;
};

extern const struct dpu_dma_config dma_engine_clock_div2_config;
extern const struct dpu_dma_config dma_engine_clock_div3_config;
extern const struct dpu_dma_config dma_engine_clock_div4_config;
extern const struct dpu_dma_config dma_engine_cas_config;
extern const struct dpu_wavegen_config waveform_generator_clock_div2_config;
extern const struct dpu_wavegen_config waveform_generator_clock_div3_config;
extern const struct dpu_wavegen_config waveform_generator_clock_div4_config;

u32 dpu_dma_shuffling_box_config(struct dpu_rank_t *rank,
				 const struct dpu_bit_config *config);
u32 dpu_wavegen_config(struct dpu_rank_t *rank,
		       const struct dpu_wavegen_config *config);
void fetch_dma_and_wavegen_configs(u32 fck_frequency, u8 clock_division,
				   u8 refresh_mode, bool ctrl_refresh,
				   struct dpu_dma_config *dma_config,
				   struct dpu_wavegen_config *wavegen_config);
u32 dpu_dma_config(struct dpu_rank_t *rank,
		   const struct dpu_dma_config *config);

#endif // __DMA_WAVEGEN_CONFIG_H__
