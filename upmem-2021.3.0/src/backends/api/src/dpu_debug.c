/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <string.h>

#include <dpu_error.h>
#include <dpu_types.h>
#include <dpu_debug.h>
#include <dpu_management.h>

#include <dpu_api_log.h>
#include <dpu_attributes.h>
#include <dpu_config.h>
#include <dpu_instruction_encoder.h>
#include <dpu_internals.h>
#include <dpu_mask.h>
#include <dpu_predef_programs.h>
#include <dpu_rank.h>
#include <verbose_control.h>
#include <dpu_memory.h>
#include <dpu_elf.h>

__API_SYMBOL__ dpu_error_t
dpu_extract_pcs_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_error_t status;

    dpu_lock_rank(rank);

    FF(RANK_FEATURE(rank, debug_read_pcs_dpu)(dpu, context));

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_extract_context_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu->rank;
    dpu_error_t status;

    dpu_lock_rank(rank);
    status = RANK_FEATURE(rank, debug_extract_context_dpu)(dpu, context);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_restore_context_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu->rank;
    dpu_error_t status;

    dpu_lock_rank(rank);
    status = RANK_FEATURE(rank, debug_restore_context_dpu)(dpu, context);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_initialize_fault_process_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_error_t status;

    dpu_lock_rank(rank);

    FF(RANK_FEATURE(rank, debug_init_dpu)(dpu, context));

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_finalize_fault_process_for_dpu(struct dpu_t *dpu, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "");

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_error_t status;

    dpu_lock_rank(rank);

    FF(RANK_FEATURE(rank, debug_teardown_dpu)(dpu, context));

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_execute_thread_step_in_fault_for_dpu(struct dpu_t *dpu, dpu_thread_t thread, struct dpu_context_t *context)
{
    LOG_DPU(DEBUG, dpu, "%d", thread);

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_error_t status;

    verify_thread_id(thread, rank);

    /* Important note: we are assuming that the thread is running/in fault. Let's just verify that it is true */
    if (context->scheduling[thread] == 0xFF) {
        LOG_DPU(WARNING, dpu, "ERROR: thread %d is not running before executing debug step", thread);
        return DPU_ERR_INTERNAL;
    }

    dpu_lock_rank(rank);

    FF(RANK_FEATURE(rank, debug_step_dpu)(dpu, context, thread));

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_save_context_for_rank(struct dpu_rank_t *rank)
{
    /* This function is used by a process that attaches a DPU rank that has been initialized. */
    LOG_RANK(DEBUG, rank, "");

    dpu_error_t status;
    dpu_lock_rank(rank);

    FF(RANK_FEATURE(rank, debug_save_context_rank)(rank));
    rank->debug.is_rank_for_debugger = true;

end:
    dpu_unlock_rank(rank);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_set_debug_slice_info(struct dpu_rank_t *rank,
    uint32_t slice_id,
    uint64_t structure_value,
    uint64_t slice_target,
    dpu_bitfield_t host_mux_mram_state)
{
    LOG_CI(DEBUG, rank, slice_id, "%016lx %016lx %08x", structure_value, slice_target, host_mux_mram_state);

    struct dpu_configuration_slice_info_t *slice_info = &rank->debug.debug_slice_info[slice_id];

    slice_info->structure_value = structure_value;
    slice_info->host_mux_mram_state = host_mux_mram_state;
    memcpy(&slice_info->slice_target, &slice_target, sizeof(slice_target));

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_restore_mux_context_for_rank(struct dpu_rank_t *rank)
{
    LOG_RANK(DEBUG, rank, "");

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, debug_restore_mux_context_rank)(rank);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_restore_context_for_rank(struct dpu_rank_t *rank)
{
    LOG_RANK(DEBUG, rank, "");

    dpu_lock_rank(rank);
    dpu_error_t status = RANK_FEATURE(rank, debug_restore_context_rank)(rank);
    dpu_unlock_rank(rank);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_context_fill(struct dpu_context_t *context,
    uint32_t nr_threads,
    uint32_t nr_work_registers,
    uint32_t nr_atomic_bits,
    iram_size_t iram_size,
    mram_size_t mram_size,
    wram_size_t wram_size)
{
    context->info.nr_threads = nr_threads;
    context->info.nr_registers = nr_work_registers;
    context->info.nr_atomic_bits = nr_atomic_bits;
    context->info.iram_size = iram_size;
    context->info.mram_size = mram_size;
    context->info.wram_size = wram_size;

    context->iram = NULL;
    context->mram = NULL;
    context->wram = NULL;

    context->registers = (uint32_t *)calloc(nr_threads * nr_work_registers, sizeof(uint32_t));
    context->pcs = (iram_addr_t *)calloc(nr_threads, sizeof(iram_addr_t));
    context->atomic_register = (bool *)calloc(nr_atomic_bits, sizeof(bool));
    context->zero_flags = (bool *)calloc(nr_threads, sizeof(bool));
    context->carry_flags = (bool *)calloc(nr_threads, sizeof(bool));
    context->scheduling = (uint8_t *)calloc(nr_threads, sizeof(uint8_t));
    if (context->registers == NULL || context->pcs == NULL || context->atomic_register == NULL || context->zero_flags == NULL
        || context->carry_flags == NULL || context->scheduling == NULL) {
        dpu_free_dpu_context(context);
        return DPU_ERR_SYSTEM;
    }

    return DPU_OK;
}

#define SERIALIZED_CONTEXT_MAGIC (0xfabddbaf)
#define SERIALIZED_CONTEXT_CHIP_VERSION 18
#define SERIALIZED_CONTEXT_VERSION 3

#define DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, curr_elem_size)                                       \
    do {                                                                                                                         \
        (serialized_context) += (curr_elem_size);                                                                                \
        if (serialized_context_end < serialized_context)                                                                         \
            return DPU_ERR_ELF_INVALID_FILE;                                                                                     \
    } while (0)

#define DESERIALIZED_ELEM(elem, serialized_context, serialized_context_end, curr_elem_size)                                      \
    do {                                                                                                                         \
        memcpy((elem), (serialized_context), (curr_elem_size));                                                                  \
        DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, curr_elem_size);                                      \
    } while (0)

static const uint32_t MAGIC_SIZE = sizeof(uint32_t);
static const uint32_t CHIP_VERSION_SIZE = sizeof(uint32_t);
static const uint32_t CONTEXT_VERSION_SIZE = sizeof(uint32_t);
static const uint32_t RESERVED_SIZE = sizeof(uint32_t);
static const uint32_t NR_THREADS_SIZE = sizeof(uint32_t);
static const uint32_t NR_REGISTERS_SIZE = sizeof(uint32_t);
static const uint32_t NR_ATOMIC_BITS_SIZE = sizeof(uint32_t);
static const uint32_t NR_INSTRUCTIONS_SIZE = sizeof(uint32_t);
static const uint32_t NR_MRAM_BYTES_SIZE = sizeof(uint32_t);
static const uint32_t NR_WRAM_WORDS_SIZE = sizeof(uint32_t);
static const uint32_t NR_OF_RUNNING_THREADS_SIZE = sizeof(uint8_t);
static const uint32_t BKP_FAULT_SIZE = sizeof(bool);
static const uint32_t DMA_FAULT_SIZE = sizeof(bool);
static const uint32_t MEM_FAULT_SIZE = sizeof(bool);
static const uint32_t BKP_FAULT_THREAD_INDEX_SIZE = sizeof(dpu_thread_t);
static const uint32_t DMA_FAULT_THREAD_INDEX_SIZE = sizeof(dpu_thread_t);
static const uint32_t MEM_FAULT_THREAD_INDEX_SIZE = sizeof(dpu_thread_t);
static const uint32_t BKP_FAULT_ID_SIZE = sizeof(uint32_t);

__API_SYMBOL__ dpu_error_t
dpu_checkpoint_deserialize(uint8_t *serialized_context, uint32_t serialized_context_size, struct dpu_context_t *context)
{
    uint8_t *serialized_context_end = serialized_context + serialized_context_size;

    if (*((uint32_t *)serialized_context) != SERIALIZED_CONTEXT_MAGIC)
        return DPU_ERR_ELF_INVALID_FILE;
    DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, MAGIC_SIZE);

    if (*((uint32_t *)serialized_context) != SERIALIZED_CONTEXT_CHIP_VERSION)
        return DPU_ERR_ELF_INVALID_FILE;
    DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, CHIP_VERSION_SIZE);

    if (*((uint32_t *)serialized_context) != SERIALIZED_CONTEXT_VERSION)
        return DPU_ERR_ELF_INVALID_FILE;
    DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, CONTEXT_VERSION_SIZE);

    if (*((uint32_t *)serialized_context) != SERIALIZED_CONTEXT_MAGIC)
        return DPU_ERR_ELF_INVALID_FILE;
    DESERIALIZED_NEXT_ELEM(serialized_context, serialized_context_end, RESERVED_SIZE);

    uint32_t nr_of_threads;
    uint32_t nr_of_work_registers_per_thread;
    uint32_t nr_of_atomic_bits;

    uint32_t iram_size;
    uint32_t mram_size;
    uint32_t wram_size;

    DESERIALIZED_ELEM(&nr_of_threads, serialized_context, serialized_context_end, NR_THREADS_SIZE);
    DESERIALIZED_ELEM(&nr_of_work_registers_per_thread, serialized_context, serialized_context_end, NR_REGISTERS_SIZE);
    DESERIALIZED_ELEM(&nr_of_atomic_bits, serialized_context, serialized_context_end, NR_ATOMIC_BITS_SIZE);
    DESERIALIZED_ELEM(&iram_size, serialized_context, serialized_context_end, NR_INSTRUCTIONS_SIZE);
    DESERIALIZED_ELEM(&mram_size, serialized_context, serialized_context_end, NR_MRAM_BYTES_SIZE);
    DESERIALIZED_ELEM(&wram_size, serialized_context, serialized_context_end, NR_WRAM_WORDS_SIZE);

    dpu_error_t status;
    if ((status = dpu_context_fill(
             context, nr_of_threads, nr_of_work_registers_per_thread, nr_of_atomic_bits, iram_size, mram_size, wram_size))
        != DPU_OK) {
        return status;
    }

    context->info.nr_threads = nr_of_threads;
    context->info.nr_registers = nr_of_work_registers_per_thread;
    context->info.nr_atomic_bits = nr_of_atomic_bits;
    context->info.iram_size = iram_size;
    context->info.mram_size = mram_size;
    context->info.wram_size = wram_size;

    if (iram_size != 0) {
        uint32_t iram_byte_size = sizeof(dpuinstruction_t) * iram_size;
        if ((context->iram = malloc(iram_byte_size)) == NULL) {
            return DPU_ERR_SYSTEM;
        }
        DESERIALIZED_ELEM(context->iram, serialized_context, serialized_context_end, iram_byte_size);
    }

    if (mram_size != 0) {
        uint32_t mram_byte_size = sizeof(uint8_t) * mram_size;
        if ((context->mram = malloc(mram_byte_size)) == NULL) {
            free(context->iram);
            return DPU_ERR_SYSTEM;
        }
        DESERIALIZED_ELEM(context->mram, serialized_context, serialized_context_end, mram_byte_size);
    }

    if (wram_size != 0) {
        uint32_t wram_byte_size = sizeof(dpuword_t) * wram_size;
        if ((context->wram = malloc(wram_byte_size)) == NULL) {
            free(context->iram);
            free(context->mram);
            return DPU_ERR_SYSTEM;
        }
        DESERIALIZED_ELEM(context->wram, serialized_context, serialized_context_end, wram_byte_size);
    }

    uint32_t registers_size = sizeof(uint32_t) * nr_of_threads * nr_of_work_registers_per_thread;
    uint32_t pcs_size = sizeof(iram_addr_t) * nr_of_threads;
    uint32_t atomic_register_size = sizeof(bool) * nr_of_atomic_bits;
    uint32_t zero_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t carry_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t scheduling_size = sizeof(uint8_t) * nr_of_threads;

    DESERIALIZED_ELEM(context->registers, serialized_context, serialized_context_end, registers_size);
    DESERIALIZED_ELEM(context->pcs, serialized_context, serialized_context_end, pcs_size);
    DESERIALIZED_ELEM(context->atomic_register, serialized_context, serialized_context_end, atomic_register_size);
    DESERIALIZED_ELEM(context->zero_flags, serialized_context, serialized_context_end, zero_flags_size);
    DESERIALIZED_ELEM(context->carry_flags, serialized_context, serialized_context_end, carry_flags_size);
    DESERIALIZED_ELEM(&context->nr_of_running_threads, serialized_context, serialized_context_end, NR_OF_RUNNING_THREADS_SIZE);
    DESERIALIZED_ELEM(context->scheduling, serialized_context, serialized_context_end, scheduling_size);
    DESERIALIZED_ELEM(&context->bkp_fault, serialized_context, serialized_context_end, BKP_FAULT_SIZE);
    DESERIALIZED_ELEM(&context->dma_fault, serialized_context, serialized_context_end, DMA_FAULT_SIZE);
    DESERIALIZED_ELEM(&context->mem_fault, serialized_context, serialized_context_end, MEM_FAULT_SIZE);
    DESERIALIZED_ELEM(&context->bkp_fault_thread_index, serialized_context, serialized_context_end, BKP_FAULT_THREAD_INDEX_SIZE);
    DESERIALIZED_ELEM(&context->dma_fault_thread_index, serialized_context, serialized_context_end, DMA_FAULT_THREAD_INDEX_SIZE);
    DESERIALIZED_ELEM(&context->mem_fault_thread_index, serialized_context, serialized_context_end, MEM_FAULT_THREAD_INDEX_SIZE);
    DESERIALIZED_ELEM(&context->bkp_fault_id, serialized_context, serialized_context_end, BKP_FAULT_ID_SIZE);

    return DPU_OK;
}

#define SERIALIZED_ELEM(serialized_context, elem, elem_size)                                                                     \
    do {                                                                                                                         \
        memcpy((serialized_context), (elem), (elem_size));                                                                       \
        (serialized_context) += (elem_size);                                                                                     \
    } while (0)

__API_SYMBOL__ uint32_t
dpu_checkpoint_get_serialized_context_size(struct dpu_context_t *context)
{
    uint32_t nr_of_threads = context->info.nr_threads;
    uint32_t nr_of_work_registers_per_thread = context->info.nr_registers;
    uint32_t nr_of_atomic_bits = context->info.nr_atomic_bits;

    uint32_t iram_size = (context->iram == NULL) ? 0 : sizeof(dpuinstruction_t) * context->info.iram_size;
    uint32_t mram_size = (context->mram == NULL) ? 0 : sizeof(uint8_t) * context->info.mram_size;
    uint32_t wram_size = (context->wram == NULL) ? 0 : sizeof(dpuword_t) * context->info.wram_size;
    uint32_t registers_size = sizeof(uint32_t) * nr_of_threads * nr_of_work_registers_per_thread;
    uint32_t pcs_size = sizeof(iram_addr_t) * nr_of_threads;
    uint32_t atomic_register_size = sizeof(bool) * nr_of_atomic_bits;
    uint32_t zero_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t carry_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t scheduling_size = sizeof(uint8_t) * nr_of_threads;
    return MAGIC_SIZE + CHIP_VERSION_SIZE + CONTEXT_VERSION_SIZE + RESERVED_SIZE + NR_THREADS_SIZE + NR_REGISTERS_SIZE
        + NR_ATOMIC_BITS_SIZE + NR_INSTRUCTIONS_SIZE + NR_MRAM_BYTES_SIZE + NR_WRAM_WORDS_SIZE + iram_size + mram_size + wram_size
        + registers_size + pcs_size + atomic_register_size + zero_flags_size + carry_flags_size + NR_OF_RUNNING_THREADS_SIZE
        + scheduling_size + BKP_FAULT_SIZE + DMA_FAULT_SIZE + MEM_FAULT_SIZE + BKP_FAULT_THREAD_INDEX_SIZE
        + DMA_FAULT_THREAD_INDEX_SIZE + MEM_FAULT_THREAD_INDEX_SIZE + BKP_FAULT_ID_SIZE;
}

__API_SYMBOL__ dpu_error_t
dpu_checkpoint_serialize(struct dpu_context_t *context, uint8_t **serialized_context, uint32_t *serialized_context_size)
{
    uint32_t serialized_size = dpu_checkpoint_get_serialized_context_size(context);

    if (*serialized_context == NULL) {
        *serialized_context = malloc(serialized_size);

        if (*serialized_context == NULL) {
            return DPU_ERR_SYSTEM;
        }

        *serialized_context_size = serialized_size;
    } else if (*serialized_context_size != serialized_size) {
        return DPU_ERR_INVALID_BUFFER_SIZE;
    }

    uint8_t *curr = *serialized_context;

    const uint32_t serialized_context_magic = SERIALIZED_CONTEXT_MAGIC;
    const uint32_t serialized_context_chip_version = SERIALIZED_CONTEXT_CHIP_VERSION;
    const uint32_t serialized_context_version = SERIALIZED_CONTEXT_VERSION;

    uint32_t nr_of_threads = context->info.nr_threads;
    uint32_t nr_of_work_registers_per_thread = context->info.nr_registers;
    uint32_t nr_of_atomic_bits = context->info.nr_atomic_bits;

    uint32_t nr_instructions = (context->iram == NULL) ? 0 : context->info.iram_size;
    uint32_t nr_wram_words = (context->wram == NULL) ? 0 : context->info.wram_size;
    uint32_t mram_size = (context->mram == NULL) ? 0 : sizeof(uint8_t) * context->info.mram_size;
    uint32_t iram_size = sizeof(dpuinstruction_t) * nr_instructions;
    uint32_t wram_size = sizeof(dpuword_t) * nr_wram_words;
    uint32_t registers_size = sizeof(uint32_t) * nr_of_threads * nr_of_work_registers_per_thread;
    uint32_t pcs_size = sizeof(iram_addr_t) * nr_of_threads;
    uint32_t atomic_register_size = sizeof(bool) * nr_of_atomic_bits;
    uint32_t zero_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t carry_flags_size = sizeof(bool) * nr_of_threads;
    uint32_t scheduling_size = sizeof(uint8_t) * nr_of_threads;

    SERIALIZED_ELEM(curr, &serialized_context_magic, MAGIC_SIZE);
    SERIALIZED_ELEM(curr, &serialized_context_chip_version, CHIP_VERSION_SIZE);
    SERIALIZED_ELEM(curr, &serialized_context_version, CONTEXT_VERSION_SIZE);
    SERIALIZED_ELEM(curr, &serialized_context_magic, RESERVED_SIZE);
    SERIALIZED_ELEM(curr, &context->info.nr_threads, NR_THREADS_SIZE);
    SERIALIZED_ELEM(curr, &context->info.nr_registers, NR_REGISTERS_SIZE);
    SERIALIZED_ELEM(curr, &context->info.nr_atomic_bits, NR_ATOMIC_BITS_SIZE);
    SERIALIZED_ELEM(curr, &nr_instructions, NR_INSTRUCTIONS_SIZE);
    SERIALIZED_ELEM(curr, &mram_size, NR_MRAM_BYTES_SIZE);
    SERIALIZED_ELEM(curr, &nr_wram_words, NR_WRAM_WORDS_SIZE);
    SERIALIZED_ELEM(curr, context->iram, iram_size);
    SERIALIZED_ELEM(curr, context->mram, mram_size);
    SERIALIZED_ELEM(curr, context->wram, wram_size);
    SERIALIZED_ELEM(curr, context->registers, registers_size);
    SERIALIZED_ELEM(curr, context->pcs, pcs_size);
    SERIALIZED_ELEM(curr, context->atomic_register, atomic_register_size);
    SERIALIZED_ELEM(curr, context->zero_flags, zero_flags_size);
    SERIALIZED_ELEM(curr, context->carry_flags, carry_flags_size);
    SERIALIZED_ELEM(curr, &context->nr_of_running_threads, NR_OF_RUNNING_THREADS_SIZE);
    SERIALIZED_ELEM(curr, context->scheduling, scheduling_size);
    SERIALIZED_ELEM(curr, &context->bkp_fault, BKP_FAULT_SIZE);
    SERIALIZED_ELEM(curr, &context->dma_fault, DMA_FAULT_SIZE);
    SERIALIZED_ELEM(curr, &context->mem_fault, MEM_FAULT_SIZE);
    SERIALIZED_ELEM(curr, &context->bkp_fault_thread_index, BKP_FAULT_THREAD_INDEX_SIZE);
    SERIALIZED_ELEM(curr, &context->dma_fault_thread_index, DMA_FAULT_THREAD_INDEX_SIZE);
    SERIALIZED_ELEM(curr, &context->mem_fault_thread_index, MEM_FAULT_THREAD_INDEX_SIZE);
    SERIALIZED_ELEM(curr, &context->bkp_fault_id, BKP_FAULT_ID_SIZE);

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_create_core_dump(struct dpu_rank_t *rank,
    const char *exe_path,
    const char *core_file_path,
    struct dpu_context_t *context,
    uint8_t *wram,
    uint8_t *mram,
    uint8_t *iram,
    uint32_t wram_size,
    uint32_t mram_size,
    uint32_t iram_size)
{
    uint8_t *context_serialized = NULL;
    uint32_t context_serialized_size = 0;
    dpu_error_t status;

    LOG_RANK(DEBUG, rank, "");

    status = dpu_checkpoint_serialize(context, &context_serialized, &context_serialized_size);
    if (status != DPU_OK) {
        LOG_RANK(WARNING, rank, "dpu_serialize_context failed (%s)", dpu_error_to_string(status));
        return status;
    }

    status = dpu_elf_create_core_dump(
        exe_path, core_file_path, wram, mram, iram, context_serialized, wram_size, mram_size, iram_size, context_serialized_size);
    if (status != DPU_OK) {
        LOG_RANK(WARNING, rank, "dpu_elf_create_core_dump failed (%s)", dpu_error_to_string(status));
        free(context_serialized);
    }

    return status;
}

__API_SYMBOL__ void
dpu_free_dpu_context(struct dpu_context_t *context)
{
    if (context == NULL)
        return;

    free(context->iram);
    free(context->mram);
    free(context->wram);

    free(context->registers);
    free(context->pcs);
    free(context->atomic_register);
    free(context->zero_flags);
    free(context->carry_flags);
    free(context->scheduling);
}

__API_SYMBOL__ dpu_error_t
dpu_context_fill_from_rank(struct dpu_context_t *context, struct dpu_rank_t *rank)
{
    dpu_description_t description = dpu_get_description(rank);
    uint32_t nr_work_registers = description->hw.dpu.nr_of_work_registers_per_thread;
    uint32_t nr_threads = description->hw.dpu.nr_of_threads;
    uint32_t nr_atomic_bits = description->hw.dpu.nr_of_atomic_bits;
    iram_size_t iram_size = description->hw.memories.iram_size;
    mram_size_t mram_size = description->hw.memories.mram_size;
    wram_size_t wram_size = description->hw.memories.wram_size;

    return dpu_context_fill(context, nr_threads, nr_work_registers, nr_atomic_bits, iram_size, mram_size, wram_size);
}

__API_SYMBOL__ dpu_error_t
dpu_pop_debug_context(struct dpu_t *dpu, struct dpu_context_t **debug_context)
{
    LOG_DPU(DEBUG, dpu, "");

    *debug_context = dpu->debug_context;
    dpu->debug_context = NULL;

    return DPU_OK;
}
