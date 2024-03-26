/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_PROGRAM_H
#define DPU_PROGRAM_H

#include <stdint.h>
#include <stdbool.h>

#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_elf.h>

/**
 * @file dpu_program.h
 * @brief C API for DPU program operations.
 */

/**
 * @brief Information from a DPU program.
 */
struct dpu_program_t {
    /** Number of tasklets. */
    uint8_t nr_threads_enabled;
    /** Address of the printf buffer. */
    int32_t printf_buffer_address;
    /** Size of the printf buffer. */
    int32_t printf_buffer_size;
    /** Address of the write pointer for the printf buffer. */
    int32_t printf_write_pointer_address;
    /** Address of the variable signaling whether the write pointer has wrapped when executing printf. */
    int32_t printf_buffer_has_wrapped_address;

    /** Public symbols from the program. */
    dpu_elf_symbols_t *symbols;

    /** Address of the mcount function. */
    int32_t mcount_address;
    /** Address of the ret_mcount function. */
    int32_t ret_mcount_address;
    /** Address of the thread_profiling array. */
    int32_t thread_profiling_address;
    /** Address of the perfcounter_end_value variable. */
    int32_t perfcounter_end_value_address;
    /** Symbols for the different DPU cycle counter variables. */
    dpu_elf_symbols_t *profiling_symbols;

    /** Address of the open_print_sequence function. */
    int32_t open_print_sequence_addr;
    /** Address of the close_print_sequence function. */
    int32_t close_print_sequence_addr;

    /** Reference counter for this structure. */
    int32_t reference_count;

    /** Path to the DPU program binary if it exists, NULL otherwise. */
    char *program_path;
};

/**
 * @brief Initialize the given DPU program structure.
 * @param program the structure to initialize
 */
void
dpu_init_program_ref(struct dpu_program_t *program);

/**
 * @brief Take a reference to the given DPU program structure, incrementing the reference counter.
 * @param program the structure to take.
 */
void
dpu_take_program_ref(struct dpu_program_t *program);

/**
 * @brief Free a reference to the given DPU program structure, decrementing the reference counter.
 * @param program the structure to free.
 */
void
dpu_free_program(struct dpu_program_t *program);

/**
 * @brief Fetches the program runtime context of the specified DPU.
 * @param dpu the unique identifier of the DPU
 * @return The pointer on the runtime context of the DPU.
 */
struct dpu_program_t *
dpu_get_program(struct dpu_t *dpu);

/**
 * @brief Sets the program runtime context of the specified DPU.
 * @param dpu the unique identifier of the DPU
 * @param program the value of the DPU program runtime context
 */
void
dpu_set_program(struct dpu_t *dpu, struct dpu_program_t *program);

/**
 * @param elf_info raw ELF information on the program stored when loading
 * @param path the path to print for debug purpose
 * @param buffer_start the in-memory buffer to load
 * @param buffer_size the size of the buffer
 * @param program information on the ELF program stored when loading
 * @param mram_size_hint size of the MRAM, to adjust some symbols
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_load_elf_program_from_memory(dpu_elf_file_t *elf_info,
    const char *path,
    uint8_t *buffer_start,
    size_t buffer_size,
    struct dpu_program_t *program,
    mram_size_t mram_size_hint);

/**
 * @param elf_info raw ELF information on the program stored when loading
 * @param path the ELF binary file path
 * @param program information on the ELF program stored when loading
 * @param mram_size_hint size of the MRAM, to adjust some symbols
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_load_elf_program(dpu_elf_file_t *elf_info, const char *path, struct dpu_program_t *program, mram_size_t mram_size_hint);

#endif // DPU_PROGRAM_H
