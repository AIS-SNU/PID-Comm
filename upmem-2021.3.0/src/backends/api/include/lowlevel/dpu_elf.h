/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_ELF_H
#define DPU_ELF_H

#include <stdint.h>
#include <stdbool.h>
#include <dpu_error.h>

/**
 * @file dpu_elf.h
 * @brief General purpose ELF loader, able to load and map any section
 *
 * Purpose of this loader is to provide an easy to use interface to upper level applications, in particular from
 * Java world, with functions to load and read ELF files, getting sections and associated symbols in a map-like
 * form.
 *
 * The first operation consists of loading the ELF file, using dpu_elf_open, or directly from memory, using dpu_elf_map.
 * Both functions return a "DPU elf descriptor".
 * The descriptor is used as reference to other functions and must be deleted with dpu_elf_close when finished.
 *
 * The information extracted from a file are:
 *   - The list of sections (dpu_elf_get_sections)
 *   - The contents of a given section (dpu_elf_load_section)
 *   - The symbols defined in a given section (dpu_elf_load_symbols)
 *   - The runtime information fetched from symbols (dpu_elf_get_runtime_info)
 */

/**
 * @brief Elf Machine Identifier for the DPU.
 * @hideinitializer
 */
#define EM_DPU 245

/**
 * @brief Description of one symbol in its section.
 */
typedef struct dpu_elf_symbol {
    /** The symbol name. */
    char *name;
    /** The associated value. */
    unsigned int value;
    /** The associated size. */
    unsigned int size;
} dpu_elf_symbol_t;

/**
 * @brief A map of symbols.
 */
typedef struct dpu_elf_symbols {
    /** The number of enclosed symbols. */
    unsigned int nr_symbols;
    /** A table of nr_symbols descriptors. */
    dpu_elf_symbol_t *map;
} dpu_elf_symbols_t;

/**
 * @brief One runtime information fetched (or not) from ELF file
 */
typedef struct _dpu_elf_runtime_info_item {
    /** If the item exists, the fetched value, a default value otherwise. */
    uint32_t value;
    /** If the item exists, the size of the symbol, `0` otherwise. */
    uint32_t size;
    /** Whether the symbol defining the runtime information was found in the ELF file. */
    bool exists;
} dpu_elf_runtime_info_item_t;

/**
 * @brief Structure describing the runtime information of a DPU program
 */
typedef struct _dpu_elf_runtime_info {
    /** The initial heap pointer value. */
    dpu_elf_runtime_info_item_t sys_heap_pointer_reset;
    /** The current heap pointer value. */
    dpu_elf_runtime_info_item_t sys_heap_pointer;
    /** The wait queue table. */
    dpu_elf_runtime_info_item_t sys_wq_table;
    /** The stack table. */
    dpu_elf_runtime_info_item_t sys_stack_table;
    /** The printf buffer array. */
    dpu_elf_runtime_info_item_t printf_buffer;
    /** The printf state structure. */
    dpu_elf_runtime_info_item_t printf_state;
    /** The final stop address. */
    dpu_elf_runtime_info_item_t sys_end;
    /** The mcount function address. */
    dpu_elf_runtime_info_item_t mcount;
    /** The ret_mcount function address. */
    dpu_elf_runtime_info_item_t ret_mcount;
    /** The thread_profiling array. */
    dpu_elf_runtime_info_item_t thread_profiling;
    /** The number of threads value. */
    dpu_elf_runtime_info_item_t nr_threads;
    /** The perfcounter_end_value variable. */
    dpu_elf_runtime_info_item_t perfcounter_end_value;
    /** The open_print_sequence function address. */
    dpu_elf_runtime_info_item_t open_print_sequence;
    /** The close_print_sequence function address. */
    dpu_elf_runtime_info_item_t close_print_sequence;
    /** All the mutex symbols information. */
    dpu_elf_symbols_t mutex_info;
    /** All the semaphore symbols information. */
    dpu_elf_symbols_t semaphore_info;
    /** All the barrier symbols information. */
    dpu_elf_symbols_t barrier_info;
} dpu_elf_runtime_info_t;

/**
 * @brief A "file pointer" to an ELF file
 *
 * Such a descriptor is created by dpu_elf_load_file, as a reference for subsequent requests. It must be
 * destroyed with dpu_elf_delete.
 */
typedef void *dpu_elf_file_t;

/**
 * @brief Loads an ELF file
 * @param path path to the ELF file
 * @param file set to the created file descriptor in case of success
 * @return DPU_OK, or an error code if the file is corrupted or not found
 */
dpu_error_t
dpu_elf_open(const char *path, dpu_elf_file_t *file);

/**
 * @brief Loads an ELF file stored in memory
 * @param memory the buffer containing the ELF file contents
 * @param size   buffer size, in bytes
 * @param file   set to the created file descriptor in case of success
 * @param filename_or_null   the original file name if it exists, NULL otherwise
 * @return DPU_OK, or an error code if the file is corrupted
 */
dpu_error_t
dpu_elf_map(uint8_t *memory, unsigned int size, dpu_elf_file_t *file, const char *filename_or_null);

/**
 * @brief Deletes a DPU ELF file descriptor
 * @param file the deleted structure
 */
void
dpu_elf_close(dpu_elf_file_t file);

/**
 * @brief Reads the contents of a given section
 * @param file the DPU ELF file descriptor
 * @param name requested section name
 * @param size set to the section size, in bytes
 * @param buffer set to the section contents
 * @return Whether this section was loaded or an error occurred (typically DPU_ERR_ELF_NO_SUCH_SECTION)
 */
dpu_error_t
dpu_elf_load_section(dpu_elf_file_t file, const char *name, unsigned int *size, uint8_t **buffer);

/**
 * @brief Reads the symbols of a given section, returning their names and values.
 *
 * Notice that the function performs common "fixup" operations for specific sections (e.g. IRAM
 * address translation).
 *
 * The returned map must be free with dpu_elf_free_symbols.
 *
 * @param file the DPU ELF file descriptor
 * @param name requested section name
 * @param symbols set to the requested symbols map
 * @return Whether the symbols were found or an error occurred (typically DPU_ERR_ELF_NO_SUCH_SECTION).
 */
dpu_error_t
dpu_elf_load_symbols(dpu_elf_file_t file, const char *name, dpu_elf_symbols_t **symbols);

/**
 * @brief Frees symbols loaded by dpu_elf_load_symbols.
 * @param symbols the symbols to free
 */
void
dpu_elf_free_symbols(dpu_elf_symbols_t *symbols);

/**
 * @brief Returns the list of sections found in the ELF file
 * @param file the DPU ELF file descriptor
 * @param nr_sections set to the number of sections found
 * @param sections set to an array of nr_sections section names, which must be freed when used
 * @return Whether the section list was successfully fetched
 */
dpu_error_t
dpu_elf_get_sections(dpu_elf_file_t file, unsigned int *nr_sections, char ***sections);

/**
 * @brief Parses the loaded symbol to construct the program runtime information
 *
 * Resulting information may either exist (found the proper symbol) or not (no such symbol found).
 *
 * @param file the DPU ELF file descriptor
 * @param runtime_info filled with the fetched information
 */
void
dpu_elf_get_runtime_info(dpu_elf_file_t file, dpu_elf_runtime_info_t *runtime_info);

/**
 * @param runtime_info the structure to free
 */
void
free_runtime_info(dpu_elf_runtime_info_t *runtime_info);

/**
 * @brief Create a core dump file from the given context.
 * @param exe_path path to the DPU program binary
 * @param core_file_path path the output core dump file
 * @param context the DPU debug context
 * @param wram snapshot of the DPU WRAM
 * @param mram snapshot of the DPU MRAM
 * @param iram snapshot of the DPU IRAM
 * @param wram_size WRAM size in bytes
 * @param mram_size MRAM size in bytes
 * @param iram_size IRAM size in bytes
 * @param context_size DPU debug context size in bytes
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_elf_create_core_dump(const char *exe_path,
    const char *core_file_path,
    uint8_t *wram,
    uint8_t *mram,
    uint8_t *iram,
    uint8_t *context,
    uint32_t wram_size,
    uint32_t mram_size,
    uint32_t iram_size,
    uint32_t context_size);

#endif // DPU_ELF_H
