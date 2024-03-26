/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu.h>
#include "dpu_attributes.h"
#include <dpu_error.h>
#include <dpu_instruction_encoder.h>
#include <dpu_internals.h>
#include <dpu_management.h>
#include <dpu_mask.h>
#include <dpu_memory.h>
#include <dpu_predef_programs.h>
#include <dpu_rank.h>
#include <dpu_types.h>

#include <stdint.h>
#include <ufi_rank_utils.h>
#include <ufi/ufi.h>
#include <ufi/ufi_config.h>
#include <ufi/ufi_ci_commands.h>
#include <ufi/ufi_ci.h>
#include <ufi/ufi_memory.h>

#define DPU_FAULT_UNKNOWN_ID (0xffffff)

typedef dpuinstruction_t *(*fetch_program_t)(iram_size_t *);
typedef dpu_error_t (*routine_t)(dpu_slice_id_t, dpu_member_id_t,
				 struct dpu_rank_t *, dpuword_t *,
				 struct dpu_context_t *, wram_size_t, uint32_t,
				 uint8_t, uint8_t, wram_size_t);

static dpu_error_t extract_bkp_fault_id(struct dpu_t *dpu, iram_addr_t pc,
					uint32_t *bkp_fault_id)
{
	dpuinstruction_t fault_instruction_mask = FAULTi(0);
	uint32_t immediate = 0;
	dpu_error_t status;
	dpuinstruction_t instruction;
	uint32_t each_bit;

	LOG_DPU(VERBOSE, dpu, "");

	if ((status = dpu_copy_from_iram_for_dpu(dpu, &instruction, pc, 1)) !=
	    DPU_OK) {
		goto end;
	}

	if ((instruction & fault_instruction_mask) != fault_instruction_mask) {
		LOG_DPU(DEBUG, dpu,
			"unexpected instruction when looking for FAULT (found: 0x%012lx)",
			instruction);
		*bkp_fault_id = DPU_FAULT_UNKNOWN_ID;
		goto end;
	}

	// We need to reconstruct the immediate
	for (each_bit = 0; each_bit < 32; ++each_bit) {
		uint32_t bit_mask = 1 << each_bit;
		dpuinstruction_t bit_location =
			FAULTi(bit_mask) ^ fault_instruction_mask;
		dpuinstruction_t bit_in_instruction =
			instruction & bit_location;
		if (bit_in_instruction != 0) {
			immediate |= 1 << each_bit;
		}
	}
	*bkp_fault_id = immediate;

end:
	return status;
}

static dpu_error_t decrement_thread_pc(struct dpu_t *dpu, dpu_thread_t thread,
				       iram_addr_t *pc)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpu_selected_mask_t mask_one = dpu_mask_one(member_id);
	dpu_error_t status;
	dpuinstruction_t instruction;
	dpuinstruction_t modified_stop_j_instruction;
	uint32_t dpu_is_running;
	uint8_t result[DPU_MAX_NR_CIS];
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS];

	LOG_DPU(VERBOSE, dpu, "");

	FF(dpu_custom_for_dpu(
		dpu, DPU_COMMAND_EVENT_START,
		(dpu_custom_command_args_t)DPU_EVENT_DEBUG_ACTION));

	if (*pc == 0) {
		*pc = rank->description->hw.memories.iram_size - 1;
	} else {
		(*pc)--;
	}

	modified_stop_j_instruction = STOPci(BOOT_CC_TRUE, *pc);

	FF(ufi_select_dpu(rank, &mask, member_id));

	iram_array[slice_id] = &instruction;
	FF(ufi_iram_read(rank, mask, iram_array, 0, 1));
	iram_array[slice_id] = &modified_stop_j_instruction;
	FF(ufi_iram_write(rank, mask, iram_array, 0, 1));
	FF(dpu_thread_boot_safe_for_dpu(dpu, thread, NULL, false));

	do {
		FF(ufi_read_dpu_run(rank, mask, result));
		dpu_is_running = result[slice_id];
	} while ((dpu_is_running & mask_one) != 0);

	iram_array[slice_id] = &instruction;
	FF(ufi_iram_write(rank, mask, iram_array, 0, 1));

	FF(dpu_custom_for_dpu(
		dpu, DPU_COMMAND_EVENT_END,
		(dpu_custom_command_args_t)DPU_EVENT_DEBUG_ACTION));

end:
	return status;
}

#define ERROR_STORAGE_GUARD 0xFEFEB
// union meant to allow accessing some uint32_t storage, either
// by bitfields or as a whole. there can be 24 different threads.
// there can be 2 different values for memory error
typedef union error_storage {
	struct {
		uint32_t dma_fault : 1;
		uint32_t dma_fault_thread_id : 5;
		uint32_t mem_fault : 1;
		uint32_t mem_fault_thread_id : 5;
		uint32_t magic : 20; //used to check if error exists
	};
	//uint32_t member for compiler-accepted type punning.
	uint32_t uint32;
} error_storage_t;

_Static_assert(
	sizeof(error_storage_t) == sizeof(uint32_t),
	"Error storage size does not match uint32_t size, which may render type punning unreliable.");

static uint32_t encode_error_storage(bool dma_fault,
				     dpu_thread_t dma_fault_thread_id,
				     bool mem_fault,
				     dpu_thread_t mem_fault_thread_id)
{
	uint32_t storage = 0;

	error_storage_t err;

	err.dma_fault = dma_fault;
	err.dma_fault_thread_id = dma_fault_thread_id;
	err.mem_fault = mem_fault;
	err.mem_fault_thread_id = mem_fault_thread_id;
	err.magic = ERROR_STORAGE_GUARD;

	storage = err.uint32;

	return storage;
}

static bool decode_error_storage(uint32_t storage, bool *dma_fault,
				 dpu_thread_t *dma_fault_thread_id,
				 bool *mem_fault,
				 dpu_thread_t *mem_fault_thread_id)
{
	error_storage_t err;
	err.uint32 = storage;

	// if error read is irrelevant : no error has been written
	if (err.magic != ERROR_STORAGE_GUARD) {
		return false;
	}

	*dma_fault = err.dma_fault;
	*dma_fault_thread_id = err.dma_fault_thread_id;
	*mem_fault = err.mem_fault;
	*mem_fault_thread_id = err.mem_fault_thread_id;

	return true;
}

__API_SYMBOL__ dpu_error_t ci_debug_init_dpu(struct dpu_t *dpu,
					     struct dpu_context_t *context)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;

	dpu_error_t status;
	uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
	uint32_t bkp_fault;
	uint32_t dma_fault;
	uint32_t mem_fault;
	dpu_thread_t bkp_fault_thread_index;
	dpu_thread_t dma_fault_thread_index;
	dpu_thread_t mem_fault_thread_index;
	dpu_selected_mask_t mask_one = dpu_mask_one(member_id);

	uint8_t mask = CI_MASK_ONE(slice_id);
	uint8_t result_array[DPU_MAX_NR_CIS];
	uint8_t nr_of_running_threads;
	bool remember_fault =
		false; // used to write to wram only if needed (error exists)

	// 1. Draining the pipeline
	// 1.1 Read and set BKP fault
	FF(ufi_select_dpu(rank, &mask, member_id));
	FF(ufi_read_bkp_fault_thread_index(rank, mask, result_array));
	bkp_fault_thread_index = result_array[slice_id];
	// !!! Read BKP fault + Interception Fault Clear
	FF(ufi_read_bkp_fault(rank, mask, result_array));
	bkp_fault = (result_array[slice_id] & mask_one) != 0;
	FF(ufi_set_bkp_fault(rank, mask));

	// 1.2 Pipeline drain
	FF(drain_pipeline(dpu, context, true));

	// 2. Fetching PCs for non-running threads
	nr_of_running_threads = context->nr_of_running_threads;
	if (nr_of_running_threads != nr_of_threads_per_dpu) {
		dpu_thread_t each_thread;

		// 2.1 Resuming all non-running threads
		// Interception Fault Set
		FF(ufi_clear_fault_dpu(rank, mask));
		FF(ufi_set_dpu_fault_and_step(rank, mask));

		for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
		     ++each_thread) {
			if (context->scheduling[each_thread] == 0xFF) {
				FF(dpu_thread_boot_safe_for_dpu(
					dpu, each_thread, NULL, true));
			}
		}

		// Interception Fault Clear
		FF(ufi_read_bkp_fault(rank, mask, NULL));

		// 2.2 Draining the pipeline, again
		FF(drain_pipeline(dpu, context, false));
	}

	// 3. Fault identification
	// 3.1 Read and clear for dma & mem fault thread index
	FF(ufi_read_dma_fault_thread_index(rank, mask, result_array));
	dma_fault_thread_index = result_array[slice_id];
	FF(ufi_read_mem_fault_thread_index(rank, mask, result_array));
	mem_fault_thread_index = result_array[slice_id];

	// 3.2 Read and clear for dma & mem faults
	FF(ufi_clear_fault_bkp(rank, mask));
	FF(ufi_read_and_clear_dma_fault(rank, mask, result_array));
	dma_fault = (result_array[slice_id] & mask_one) != 0;
	FF(ufi_read_and_clear_mem_fault(rank, mask, result_array));
	mem_fault = (result_array[slice_id] & mask_one) != 0;

	// 3.3 Clear fault
	FF(ufi_clear_fault_dpu(rank, mask));

	if ((bkp_fault & mask_one) != 0) {
		iram_addr_t current_pc;
		iram_addr_t previous_pc;

		context->bkp_fault = true;
		context->bkp_fault_thread_index = bkp_fault_thread_index;
		current_pc = context->pcs[context->bkp_fault_thread_index];

		if (current_pc == 0) {
			previous_pc =
				rank->description->hw.memories.iram_size - 1;
		} else {
			previous_pc = current_pc - 1;
		}
		FF(extract_bkp_fault_id(dpu, previous_pc,
					&(context->bkp_fault_id)));

		/* If bkp_fault_id is unknown, it means that the fault has not been generated by the DPU program, so it came from a host
         * and we can clear the bkp_fault.
         * If the bkp_fault_id is not unknown, we need to decrement the pc as the bkp instruction should have incremented it. */
		if (context->bkp_fault_id == DPU_FAULT_UNKNOWN_ID) {
			context->bkp_fault = false;
		} else {
			FF(decrement_thread_pc(dpu, bkp_fault_thread_index,
					       context->pcs +
						       bkp_fault_thread_index));
		}
	}
	if ((dma_fault & mask_one) != 0) {
		remember_fault = true;
		context->dma_fault = true;
		context->dma_fault_thread_index = dma_fault_thread_index;
		FF(decrement_thread_pc(dpu, dma_fault_thread_index,
				       context->pcs + dma_fault_thread_index));
	}
	if ((mem_fault & mask_one) != 0) {
		remember_fault = true;
		context->mem_fault = true;
		context->mem_fault_thread_index = mem_fault_thread_index;
		FF(decrement_thread_pc(dpu, mem_fault_thread_index,
				       context->pcs + mem_fault_thread_index));
	}

	// if a program is loaded
	if (dpu->program) {
		struct dpu_symbol_t dpu_error_storage;
		dpuword_t storage = 0;
		bool dma_f, mem_f;
		dpu_thread_t dma_thread, mem_thread;
		// in case of symbol not existing (asm-coded dpu program), doing nothing
		// get the right symbol for the DPU.
		// read at this address
		// update the dpu_context_t
		// write at this address if the new debug state if necessary

		// name matching variable defined in dpu-rt/.../crt0.c
		if (dpu_get_symbol(dpu->program, "error_storage",
				   &dpu_error_storage) == DPU_OK) {
			// memory transfer here (read in storage) Wram is virtually mapped at address 0 and offset is expressed by dpuword_t indexing.
			FF(ci_copy_from_wrams_dpu(dpu, &storage,
						  dpu_error_storage.address /
							  sizeof(dpuword_t),
						  1));

			if (decode_error_storage(storage, &dma_f, &dma_thread,
						 &mem_f, &mem_thread)) {
				// update context
				context->dma_fault =
					context->dma_fault || dma_f;
				if ((dma_fault & mask_one) == 0) {
					context->dma_fault_thread_index =
						dma_thread;
				}
				context->mem_fault =
					context->mem_fault || mem_f;
				if ((mem_fault & mask_one) == 0) {
					context->mem_fault_thread_index =
						mem_thread;
				}
			}

			if (remember_fault) {
				// here write to memory, read out of storage
				storage = encode_error_storage(
					context->dma_fault,
					context->dma_fault_thread_index,
					context->mem_fault,
					context->dma_fault_thread_index);

				FF(ci_copy_to_wrams_dpu(
					dpu,
					dpu_error_storage.address /
						sizeof(dpuword_t),
					&storage, 1));
			}
		}
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_debug_teardown_dpu(struct dpu_t *dpu,
						 struct dpu_context_t *context)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;

	dpu_error_t status;
	uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
	uint8_t nr_of_running_threads = context->nr_of_running_threads;
	dpu_thread_t scheduling_order[nr_of_threads_per_dpu];
	dpu_thread_t each_thread;
	dpu_thread_t each_running_thread;

	uint8_t mask = CI_MASK_ONE(slice_id);

	FF(ufi_select_dpu(rank, &mask, member_id));

	// 1. Set fault & bkp_fault
	FF(ufi_set_dpu_fault_and_step(rank, mask));
	FF(ufi_set_bkp_fault(rank, mask));

	// 2. Resume running threads
	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		uint8_t scheduling_position = context->scheduling[each_thread];
		if (scheduling_position != 0xFF) {
			scheduling_order[scheduling_position] = each_thread;
		}
	}

	for (each_running_thread = 0;
	     each_running_thread < nr_of_running_threads;
	     ++each_running_thread) {
		FF(dpu_thread_boot_safe_for_dpu(
			dpu, scheduling_order[each_running_thread], NULL,
			true));
	}
	// Interception Fault Clear
	FF(ufi_read_bkp_fault(rank, mask, NULL));

	// 3. Clear bkp_fault & fault
	FF(ufi_clear_fault_bkp(rank, mask));

	/* If the DPU was in fault (according to the context), we need to keep it in fault
	 * so that any other process (mostly a host application) will see it in this state.
	 */
	if (!((context->bkp_fault && context->bkp_fault_id != 0) ||
	      context->mem_fault || context->dma_fault)) {
		FF(ufi_clear_fault_dpu(rank, mask));
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_debug_step_dpu(struct dpu_t *dpu,
					     struct dpu_context_t *context,
					     dpu_thread_t thread)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;

	dpu_error_t status;
	uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
	uint32_t poison_fault;
	uint32_t dma_fault;
	uint32_t mem_fault;
	uint32_t nr_of_waiting_threads;
	uint32_t step_run_state;
	uint32_t step_fault_state;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpu_selected_mask_t mask_one = dpu_mask_one(member_id);
	uint8_t nr_of_running_threads = context->nr_of_running_threads;
	dpu_thread_t scheduling_order[nr_of_threads_per_dpu];
	dpu_thread_t each_thread;
	dpu_thread_t each_running_thread;
	uint8_t result_array[DPU_MAX_NR_CIS];

	FF(ufi_select_dpu(rank, &mask, member_id));

	// 1. Set fault & fault_poison
	FF(ufi_set_dpu_fault_and_step(rank, mask));
	FF(ufi_set_poison_fault(rank, mask));

	// 2. Resume thread
	FF(dpu_thread_boot_safe_for_dpu(dpu, thread, NULL, true));

	// 3. Resume other running threads
	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		uint8_t scheduling_position = context->scheduling[each_thread];
		if (scheduling_position != 0xFF) {
			scheduling_order[scheduling_position] = each_thread;
		}
	}

	for (each_running_thread = 0;
	     each_running_thread < nr_of_running_threads;
	     ++each_running_thread) {
		dpu_thread_t thread_to_be_resumed =
			scheduling_order[each_running_thread];

		if (thread_to_be_resumed != thread) {
			FF(dpu_thread_boot_safe_for_dpu(
				dpu, thread_to_be_resumed, NULL, true));
		}
	}

	// 4. Interception Fault Clear
	FF(ufi_read_bkp_fault(rank, mask, NULL));

	// 5. Execute step (by clearing fault)
	FF(ufi_clear_fault_bkp(rank, mask));
	FF(ufi_clear_fault_dpu(rank, mask));

	// 5 Bis. Wait for end of step
	do {
		FF(ufi_read_dpu_run(rank, mask, result_array));
		step_run_state = result_array[slice_id];
		FF(ufi_read_dpu_fault(rank, mask, result_array));
		step_fault_state = result_array[slice_id];
	} while (((step_run_state & ~step_fault_state) & mask_one) != 0);

	// 6. Read & clear faults
	FF(ufi_read_poison_fault(rank, mask, result_array));
	poison_fault = result_array[slice_id];
	FF(ufi_clear_fault_poison(rank, mask));

	// Interception Fault Clear
	FF(ufi_read_bkp_fault(rank, mask, NULL));
	FF(ufi_clear_fault_bkp(rank, mask));
	FF(ufi_read_and_clear_dma_fault(rank, mask, result_array));
	dma_fault = result_array[slice_id];
	FF(ufi_read_and_clear_mem_fault(rank, mask, result_array));
	mem_fault = result_array[slice_id];

	// 7. Resetting scheduling structure
	nr_of_waiting_threads = (uint32_t)(context->nr_of_running_threads - 1);

	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		context->scheduling[each_thread] = 0xFF;
	}

	context->nr_of_running_threads = 0;

	// 8. Drain pipeline
	FF(drain_pipeline(dpu, context, true));

	// 9. Clear fault
	FF(ufi_clear_fault_dpu(rank, mask));

	if ((context->nr_of_running_threads - nr_of_waiting_threads) == 1) {
		// Only one more thread running (the one we stepped on). It may have provoked a fault (if 0 or 2 more threads, the thread
		// has executed a stop or a boot/resume and no fault can happen).
		if ((poison_fault & mask_one) == 0) {
			// If poison_fault has been cleared, the stepped instruction was a bkp.
			context->bkp_fault = true;
			context->bkp_fault_thread_index = thread;
			FF(decrement_thread_pc(dpu, thread,
					       context->pcs + thread));

			FF(extract_bkp_fault_id(
				dpu,
				context->pcs[context->bkp_fault_thread_index],
				&(context->bkp_fault_id)));
		}
		if ((dma_fault & mask_one) != 0) {
			context->dma_fault = true;
			context->dma_fault_thread_index = thread;
			FF(decrement_thread_pc(dpu, thread,
					       context->pcs + thread));
		}
		if ((mem_fault & mask_one) != 0) {
			context->mem_fault = true;
			context->mem_fault_thread_index = thread;
			FF(decrement_thread_pc(dpu, thread,
					       context->pcs + thread));
		}
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_debug_read_pcs_dpu(struct dpu_t *dpu,
						 struct dpu_context_t *context)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;

	uint8_t mask = CI_MASK_ONE(slice_id);
	uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
	dpu_error_t status;
	dpu_thread_t each_thread;

	FF(ufi_select_dpu(rank, &mask, member_id));

	// 1. Set fault
	FF(ufi_set_dpu_fault_and_step(rank, mask));
	FF(ufi_set_bkp_fault(rank, mask));

	// 2. Loop on each thread
	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		// 2.1 Resume thread
		FF(dpu_thread_boot_safe_for_dpu(dpu, each_thread, NULL, true));
	}
	// 3. Interception Fault Clear
	FF(ufi_read_bkp_fault(rank, mask, NULL));

	// 4. Drain Pipeline
	FF(drain_pipeline(dpu, context, false));

	// 5. Clear fault
	FF(ufi_select_dpu(rank, &mask, member_id));
	FF(ufi_clear_fault_bkp(rank, mask));
	FF(ufi_clear_fault_dpu(rank, mask));

end:
	return status;
}

static dpu_error_t dpu_execute_routine_for_dpu(struct dpu_t *dpu,
					       struct dpu_context_t *context,
					       fetch_program_t fetch_program,
					       routine_t routine,
					       dpu_event_kind_t custom_event)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_slice_id_t slice_id = dpu->slice_id;
	dpu_member_id_t member_id = dpu->dpu_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	uint32_t nr_of_atomic_bits_per_dpu =
		rank->description->hw.dpu.nr_of_atomic_bits;
	uint8_t nr_of_threads_per_dpu = rank->description->hw.dpu.nr_of_threads;
	uint8_t nr_of_work_registers_per_thread =
		rank->description->hw.dpu.nr_of_work_registers_per_thread;
	wram_size_t atomic_register_size_in_words =
		nr_of_atomic_bits_per_dpu / sizeof(dpuword_t);

	dpu_error_t status;
	iram_size_t program_size_in_instructions;
	wram_size_t context_size_in_words;
	dpuinstruction_t *iram_backup = NULL;
	dpuword_t *wram_backup = NULL;
	dpuword_t *raw_context = NULL;
	dpuinstruction_t *program = NULL;

	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS];
	dpuword_t *wram_array[DPU_MAX_NR_CIS];
	dpu_thread_t each_thread;

	LOG_DPU(VERBOSE, dpu, "");

	FF(dpu_custom_for_dpu(dpu, DPU_COMMAND_EVENT_START,
			      (dpu_custom_command_args_t)custom_event));

	program = fetch_program(&program_size_in_instructions);

	if (program == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	if ((iram_backup = malloc(program_size_in_instructions *
				  sizeof(*iram_backup))) == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	// 1. Save IRAM
	FF(ufi_select_dpu(rank, &mask, member_id));
	iram_array[slice_id] = iram_backup;
	FF(ufi_iram_read(rank, mask, iram_array, 0,
			 program_size_in_instructions));

	// 2. Save WRAM
	context_size_in_words =
		atomic_register_size_in_words +
		(nr_of_threads_per_dpu * (nr_of_work_registers_per_thread + 1));

	if ((wram_backup = malloc(context_size_in_words *
				  sizeof(*wram_backup))) == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	wram_array[slice_id] = wram_backup;
	FF(ufi_wram_read(rank, mask, wram_array, 0, context_size_in_words));

	// 3. Insert PCs in core dump program
	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		set_pc_in_core_dump_or_restore_registers(
			each_thread, context->pcs[each_thread], program,
			program_size_in_instructions, nr_of_threads_per_dpu);
	}

	// 4. Load IRAM with core dump program
	iram_array[slice_id] = program;
	FF(ufi_iram_write(rank, mask, iram_array, 0,
			  program_size_in_instructions));

	// 5. Execute routine
	if ((raw_context = malloc(context_size_in_words *
				  sizeof(*raw_context))) == NULL) {
		status = DPU_ERR_SYSTEM;
		goto end;
	}

	FF(routine(slice_id, member_id, rank, raw_context, context,
		   context_size_in_words, nr_of_atomic_bits_per_dpu,
		   nr_of_threads_per_dpu, nr_of_work_registers_per_thread,
		   atomic_register_size_in_words));

	// 6. Restore WRAM
	wram_array[slice_id] = wram_backup;
	FF(ufi_wram_write(rank, mask, wram_array, 0, context_size_in_words));

	// 7. Restore IRAM
	iram_array[slice_id] = iram_backup;
	FF(ufi_iram_write(rank, mask, iram_array, 0,
			  program_size_in_instructions));

	FF(dpu_custom_for_dpu(dpu, DPU_COMMAND_EVENT_END,
			      (dpu_custom_command_args_t)custom_event));

end:
	free(raw_context);
	free(wram_backup);
	free(iram_backup);
	free(program);
	return status;
}

static dpu_error_t dpu_boot_and_wait_for_dpu(dpu_slice_id_t slice_id,
					     dpu_member_id_t member_id,
					     struct dpu_rank_t *rank)
{
	dpu_error_t status;
	dpu_bitfield_t running[DPU_MAX_NR_CIS];
	dpu_selected_mask_t mask_one = dpu_mask_one(member_id);
	uint8_t mask = CI_MASK_ONE(slice_id);

	// 1. Boot thread 0
	FF(ufi_select_dpu(rank, &mask, member_id));
	FF(dpu_thread_boot_safe_for_dpu(dpu_get(rank, slice_id, member_id), 0,
					NULL, false));

	// 2. Wait for end of program
	do {
		FF(ufi_read_run_bit(rank, mask, 0, running));
	} while ((running[slice_id] & mask_one) != 0);

end:
	return status;
}

static dpu_error_t dpu_extract_context_for_dpu_routine(
	dpu_slice_id_t slice_id, dpu_member_id_t member_id,
	struct dpu_rank_t *rank, dpuword_t *raw_context,
	struct dpu_context_t *context, wram_size_t context_size_in_words,
	uint32_t nr_of_atomic_bits_per_dpu, uint8_t nr_of_threads_per_dpu,
	uint8_t nr_of_work_registers_per_thread,
	wram_size_t atomic_register_size_in_words)
{
	dpu_error_t status;
	dpuword_t *wram_array[DPU_MAX_NR_CIS];
	uint8_t mask = CI_MASK_ONE(slice_id);
	uint32_t each_atomic_bit;
	dpu_thread_t each_thread;

	// 1. Boot and wait
	FF(dpu_boot_and_wait_for_dpu(slice_id, member_id, rank));

	// 2. Fetch context from WRAM
	FF(ufi_select_dpu(rank, &mask, member_id));
	wram_array[slice_id] = raw_context;
	FF(ufi_wram_read(rank, mask, wram_array, 0, context_size_in_words));

	// 3. Format context
	for (each_atomic_bit = 0; each_atomic_bit < nr_of_atomic_bits_per_dpu;
	     ++each_atomic_bit) {
		context->atomic_register[each_atomic_bit] =
			((uint8_t *)raw_context)[each_atomic_bit] != 0;
	}

	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		uint32_t each_register_index;
		uint32_t flags = raw_context[atomic_register_size_in_words +
					     (nr_of_work_registers_per_thread *
					      nr_of_threads_per_dpu) +
					     each_thread];

		for (each_register_index = 0;
		     each_register_index < nr_of_work_registers_per_thread;
		     each_register_index += 2) {
			uint32_t even_register_index =
				each_thread * nr_of_work_registers_per_thread +
				each_register_index;
			wram_size_t odd_register_context_index =
				atomic_register_size_in_words +
				(each_register_index * nr_of_threads_per_dpu) +
				(2 * each_thread);

			context->registers[even_register_index] =
				raw_context[odd_register_context_index + 1];
			context->registers[even_register_index + 1] =
				raw_context[odd_register_context_index];
		}

		context->carry_flags[each_thread] = (flags & 1) != 0;
		context->zero_flags[each_thread] = (flags & 2) != 0;
	}

end:
	return status;
}

static dpu_error_t dpu_restore_context_for_dpu_routine(
	dpu_slice_id_t slice_id, dpu_member_id_t member_id,
	struct dpu_rank_t *rank, dpuword_t *raw_context,
	struct dpu_context_t *context, wram_size_t context_size_in_words,
	uint32_t nr_of_atomic_bits_per_dpu, uint8_t nr_of_threads_per_dpu,
	uint8_t nr_of_work_registers_per_thread,
	wram_size_t atomic_register_size_in_words)
{
	dpu_error_t status;
	dpuword_t *wram_array[DPU_MAX_NR_CIS];
	uint32_t each_atomic_bit;
	dpu_thread_t each_thread;
	uint8_t mask = CI_MASK_ONE(slice_id);

	// 1. Format raw context
	for (each_atomic_bit = 0; each_atomic_bit < nr_of_atomic_bits_per_dpu;
	     ++each_atomic_bit) {
		((uint8_t *)raw_context)[each_atomic_bit] =
			context->atomic_register[each_atomic_bit] ? 0xFF : 0x00;
	}
	for (each_thread = 0; each_thread < nr_of_threads_per_dpu;
	     ++each_thread) {
		uint32_t each_register_index;
		uint32_t flags = (context->carry_flags[each_thread] ? 1 : 0) +
				 (context->zero_flags[each_thread] ? 2 : 0);

		for (each_register_index = 0;
		     each_register_index < nr_of_work_registers_per_thread;
		     each_register_index += 2) {
			uint32_t even_regsiter_index =
				each_thread * nr_of_work_registers_per_thread +
				each_register_index;
			wram_size_t odd_register_context_index =
				atomic_register_size_in_words +
				(each_register_index * nr_of_threads_per_dpu) +
				(2 * each_thread);

			raw_context[odd_register_context_index] =
				context->registers[even_regsiter_index + 1];
			raw_context[odd_register_context_index + 1] =
				context->registers[even_regsiter_index];
		}

		raw_context[atomic_register_size_in_words +
			    (nr_of_work_registers_per_thread *
			     nr_of_threads_per_dpu) +
			    each_thread] = flags;
	}

	// 2. Load WRAM with raw context
	FF(ufi_select_dpu(rank, &mask, member_id));
	wram_array[slice_id] = raw_context;
	FF(ufi_wram_write(rank, mask, wram_array, 0, context_size_in_words));

	// 3. Boot thread 0
	FF(dpu_boot_and_wait_for_dpu(slice_id, member_id, rank));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t
ci_debug_extract_context_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
	return dpu_execute_routine_for_dpu(dpu, context,
					   fetch_core_dump_program,
					   dpu_extract_context_for_dpu_routine,
					   DPU_EVENT_EXTRACT_CONTEXT);
}

__API_SYMBOL__ dpu_error_t
ci_debug_restore_context_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
	return dpu_execute_routine_for_dpu(dpu, context,
					   fetch_restore_registers_program,
					   dpu_restore_context_for_dpu_routine,
					   DPU_EVENT_RESTORE_CONTEXT);
}

__API_SYMBOL__ dpu_error_t ci_debug_save_context_rank(struct dpu_rank_t *rank)
{
	dpu_error_t status;

	FF(ci_get_color(rank, (uint32_t *)&rank->debug.debug_result));

	memcpy(&rank->debug.debug_color, &rank->runtime.control_interface.color,
	       sizeof(dpu_ci_bitfield_t));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_debug_restore_context_rank(struct dpu_rank_t *rank)
{
	/* Restore as left by the debuggee:
	 * - the last target
	 * - the color
	 * - the structure register
	 * - the result
	 */
	dpu_error_t status = DPU_OK;
	uint64_t data[DPU_MAX_NR_CIS] = { 0 };
	uint32_t timeout = TIMEOUT_COLOR;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t identity_mask = 0;
	bool result_is_stable;
	dpu_slice_id_t each_slice;

	/* 1/ Sets the color as it was before the debugger intervention */

	/* Read the control interface as long as [63: 56] != 0: this loop is necessary in case of FPGA where
	 * a latency due to implementation makes the initial result not appear right away in case of valid command (ie
	 * not 0x00 and not 0xFF (NOP)).
	 * A NOP command does not change the color and [63: 56],
	 * A valid command changes the color and clears [63: 56],
	 * An invalid command (ie something the CI does not understand) changes the color since it sets CMD_FAULT_DECODE and clears
	 * [63: 56],
	 */
	do {
		FF(ci_update_commands(rank, data));

		timeout--;

		result_is_stable = true;
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			result_is_stable =
				result_is_stable &&
				(((data[each_slice] >> 56) & 0xFF) == 0);
		}
	} while (timeout && !result_is_stable);

	if (!timeout) {
		LOG_RANK(WARNING, rank,
			 "Timeout waiting for result to be correct");
		return DPU_ERR_TIMEOUT;
	}

	/* 2/ Restore the last target */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		uint8_t mask = CI_MASK_ONE(each_slice);
		if (rank->debug.debug_slice_info[each_slice].slice_target.type !=
		    DPU_SLICE_TARGET_NONE) {
			switch (rank->debug.debug_slice_info[each_slice]
					.slice_target.type) {
			case DPU_SLICE_TARGET_DPU:
				FF(ufi_select_dpu_uncached(
					rank, &mask,
					rank->debug.debug_slice_info[each_slice]
						.slice_target.dpu_id));
				break;
			case DPU_SLICE_TARGET_GROUP:
				FF(ufi_select_group_uncached(
					rank, &mask,
					rank->debug.debug_slice_info[each_slice]
						.slice_target.group_id));
				break;
			case DPU_SLICE_TARGET_ALL:
				FF(ufi_select_all_uncached(rank, &mask));
			default:
				break;
			}
		}
	}

	/* Here we have two important things to take care of:
	 *  - Set the color to the value the application expects,
	 *  - Replay the last write structure
	 *
	 * 2 cases:
	 *  - Current color is not the one expected: simply replay the last write structure
	 *  - Current color is     the one expected: send an identity command and replay the last write structure.
	 *
	 *  Note that replaying a write structure has no effect; structure register value is only
	 *  used when a send frame is sent, on its own, it does nothing.
	 */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		uint8_t ci_mask = CI_MASK_ONE(each_slice);
		if ((rank->runtime.control_interface.color & ci_mask) ==
		    (rank->debug.debug_color & ci_mask)) {
			identity_mask |= ci_mask;
		}
	}

	if (identity_mask != 0) {
		uint32_t identity_result[DPU_MAX_NR_CIS];
		FF(ufi_identity(rank, identity_mask, identity_result));
	}

	/* 3/ Restore the structure value */
	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		data[each_slice] =
			rank->debug.debug_slice_info[each_slice].structure_value;
		/* Update debugger last structure value written */
		rank->runtime.control_interface.slice_info[each_slice]
			.structure_value =
			rank->debug.debug_slice_info[each_slice].structure_value;
	}

	FF(ci_commit_commands(rank, data));

	timeout = TIMEOUT_COLOR;
	do {
		FF(ci_update_commands(rank, data));

		timeout--;

		result_is_stable = true;
		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			uint8_t cmd_type = (uint8_t)(((data[each_slice] &
						       0xFF00000000000000ULL) >>
						      56) &
						     0xFF);
			uint8_t valid = (uint8_t)(((data[each_slice] &
						    0xFF00000000ULL) >>
						   32) &
						  0xFF);
			result_is_stable = result_is_stable &&
					   (cmd_type == 0) && (valid == 0xFF);
		}
	} while (timeout && !result_is_stable);

	if (!timeout) {
		LOG_RANK(WARNING, rank,
			 "Timeout waiting for result to be correct");
		return DPU_ERR_TIMEOUT;
	}

	/* The above command toggled the color... */
	rank->runtime.control_interface.color ^= (1UL << nr_cis) - 1;

	for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
		/* 4/ Restore the result expected by the host application */
		data[each_slice] = 0xFF00000000000000ULL |
				   rank->debug.debug_result[each_slice];
	}

	FF(ci_commit_commands(rank, data));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t
ci_debug_restore_mux_context_rank(struct dpu_rank_t *rank)
{
	dpu_error_t status;
	dpu_member_id_t each_dpu;
	uint8_t nr_dpus =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;

	for (each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
		dpu_slice_id_t each_slice;
		uint8_t mux_mask = 0;

		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			mux_mask |= ((rank->debug.debug_slice_info[each_slice]
					      .host_mux_mram_state >>
				      each_dpu) &
				     1)
				    << each_slice;
		}
		if ((status = dpu_switch_mux_for_dpu_line(
			     rank, each_dpu, mux_mask)) != DPU_OK) {
			return status;
		}
	}

	return DPU_OK;
}
