/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __UFI_H__
#define __UFI_H__

#include <ufi/ufi_ci_types.h>
#include <dpu_types.h>

#define ALL_CIS ((1u << DPU_MAX_NR_CIS) - 1u)
#define CI_MASK_ONE(ci) (1u << (ci))

#define DPU_ENABLED_GROUP 0
#define DPU_DISABLED_GROUP 1

u32 ufi_byte_order(struct dpu_rank_t *rank, u8 ci_mask, u64 *results);
u32 ufi_soft_reset(struct dpu_rank_t *rank, u8 ci_mask, u8 clock_division,
		   u8 cycle_accurate);
u32 ufi_bit_config(struct dpu_rank_t *rank, u8 ci_mask,
		   const struct dpu_bit_config *config, u32 *results);
u32 ufi_identity(struct dpu_rank_t *rank, u8 ci_mask, u32 *results);
u32 ufi_thermal_config(struct dpu_rank_t *rank, u8 ci_mask,
		       enum dpu_temperature threshold);

u32 ufi_select_cis(struct dpu_rank_t *rank, u8 *ci_mask);
u32 ufi_select_all(struct dpu_rank_t *rank, u8 *ci_mask);
u32 ufi_select_all_uncached(struct dpu_rank_t *rank, u8 *ci_mask);
u32 ufi_select_all_even_disabled(struct dpu_rank_t *rank, u8 *ci_mask);
u32 ufi_select_group(struct dpu_rank_t *rank, u8 *ci_mask, u8 group);
u32 ufi_select_group_uncached(struct dpu_rank_t *rank, u8 *ci_mask, u8 group);
u32 ufi_select_dpu(struct dpu_rank_t *rank, u8 *ci_mask, u8 dpu);
u32 ufi_select_dpu_uncached(struct dpu_rank_t *rank, u8 *ci_mask, u8 dpu);
u32 ufi_select_dpu_even_disabled(struct dpu_rank_t *rank, u8 *ci_mask, u8 dpu);

u32 ufi_write_group(struct dpu_rank_t *rank, u8 ci_mask, u8 group);

u32 ufi_carousel_config(struct dpu_rank_t *rank, u8 ci_mask,
			const struct dpu_carousel_config *config);

u32 ufi_clear_debug_replace(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_poison(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_bkp(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_dma(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_mem(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_dpu(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_fault_intercept(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_clear_run_bit(struct dpu_rank_t *rank, u8 ci_mask, u8 run_bit,
		      u8 *previous);

u32 ufi_iram_repair_config(struct dpu_rank_t *rank, u8 ci_mask,
			   struct dpu_repair_config **config);
u32 ufi_wram_repair_config(struct dpu_rank_t *rank, u8 ci_mask, u8 bank,
			   struct dpu_repair_config **config);

u32 ufi_get_pc_mode(struct dpu_rank_t *rank, u8 ci_mask,
		    enum dpu_pc_mode *pc_mode);
u32 ufi_set_pc_mode(struct dpu_rank_t *rank, u8 ci_mask,
		    const enum dpu_pc_mode *pc_mode);
u32 ufi_set_stack_direction(struct dpu_rank_t *rank, u8 ci_mask,
			    const bool *direction, u8 *previous);

u32 ufi_write_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask, u8 address,
		       u8 data);
u32 ufi_read_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask, u8 *data);
u32 ufi_clear_dma_ctrl(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_set_mram_mux(struct dpu_rank_t *rank, u8 ci_mask,
		     dpu_ci_bitfield_t ci_mux_pos);

u32 ufi_iram_write(struct dpu_rank_t *rank, u8 ci_mask, u64 **src, u16 offset,
		   u16 len);
u32 ufi_iram_read(struct dpu_rank_t *rank, u8 ci_mask, u64 **dst, u16 offset,
		  u16 len);

u32 ufi_wram_write(struct dpu_rank_t *rank, u8 ci_mask, u32 **src, u16 offset,
		   u16 len);
u32 ufi_wram_read(struct dpu_rank_t *rank, u8 ci_mask, u32 **dst, u16 offset,
		  u16 len);

u32 ufi_thread_boot(struct dpu_rank_t *rank, u8 ci_mask, u8 thread,
		    u8 *previous);
u32 ufi_thread_resume(struct dpu_rank_t *rank, u8 ci_mask, u8 thread,
		      u8 *previous);
u32 ufi_read_run_bit(struct dpu_rank_t *rank, u8 ci_mask, u8 run_bit,
		     u8 *running);

u32 ufi_read_dpu_run(struct dpu_rank_t *rank, u8 ci_mask, u8 *run);
u32 ufi_read_dpu_fault(struct dpu_rank_t *rank, u8 ci_mask, u8 *fault);

u32 ufi_set_dpu_fault_and_step(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_set_bkp_fault(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_set_poison_fault(struct dpu_rank_t *rank, u8 ci_mask);

u32 ufi_read_bkp_fault(struct dpu_rank_t *rank, u8 ci_mask, u8 *fault);
u32 ufi_read_poison_fault(struct dpu_rank_t *rank, u8 ci_mask, u8 *fault);

u32 ufi_read_and_clear_dma_fault(struct dpu_rank_t *rank, u8 ci_mask,
				 u8 *fault);
u32 ufi_read_and_clear_mem_fault(struct dpu_rank_t *rank, u8 ci_mask,
				 u8 *fault);
u32 ufi_read_bkp_fault_thread_index(struct dpu_rank_t *rank, u8 ci_mask,
				    u8 *thread);
u32 ufi_read_dma_fault_thread_index(struct dpu_rank_t *rank, u8 ci_mask,
				    u8 *thread);
u32 ufi_read_mem_fault_thread_index(struct dpu_rank_t *rank, u8 ci_mask,
				    u8 *thread);

u32 ufi_debug_replace_stop(struct dpu_rank_t *rank, u8 ci_mask);
u32 ufi_debug_pc_read(struct dpu_rank_t *rank, u8 ci_mask, u16 *pcs);
u32 ufi_debug_pc_sample(struct dpu_rank_t *rank, u8 ci_mask);

#endif /* __UFI_H__ */
