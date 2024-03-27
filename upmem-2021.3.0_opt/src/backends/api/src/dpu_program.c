/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#define _GNU_SOURCE
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dpu_program.h>
#include <dpu_rank.h>
#include <dpu_error.h>
#include <runtime_info.h>
#include <dpu_attributes.h>
#include <dpu_types.h>
#include <dpu_elf_internals.h>
#include <dpu_api_verbose.h>
#include <dpu_log_utils.h>

static dpu_error_t
append_symbols(dpu_elf_symbols_t *new_symbols, dpu_elf_symbols_t *symbols, mram_size_t mram_size_hint);

static void
free_symbols(dpu_elf_symbols_t *symbols);

__API_SYMBOL__ struct dpu_program_t *
dpu_get_program(struct dpu_t *dpu)
{
    return dpu->program;
}

__API_SYMBOL__ void
dpu_set_program(struct dpu_t *dpu, struct dpu_program_t *program)
{
    dpu->program = program;
}

__API_SYMBOL__ void
dpu_init_program_ref(struct dpu_program_t *program)
{
    program->reference_count = 0;
}

__API_SYMBOL__ void
dpu_take_program_ref(struct dpu_program_t *program)
{
    __sync_fetch_and_add(&(program->reference_count), 1);
}

__API_SYMBOL__ void
dpu_free_program(struct dpu_program_t *program)
{
    if (program != NULL) {
        int32_t ref_count = __sync_sub_and_fetch(&(program->reference_count), 1);
        if (ref_count < 0) {
            // Usage error: this happens in case dpu_free_program is called more
            // times than dpu_init_program_ref
            LOG_FN(WARNING, "Error: double free detected.");
            assert(0 && "dpu_free_program: error, double free");
        } else if (ref_count == 0) {
            if (program->symbols != NULL) {
                free_symbols(program->symbols);
            }
            if (program->profiling_symbols != NULL) {
                free_symbols(program->profiling_symbols);
            }
            free(program->program_path);
            free(program);
        }
    }
}

static dpu_error_t
dpu_load_elf_program_from_elf_info(dpu_elf_file_t *elf_info, struct dpu_program_t *program, mram_size_t mram_size_hint)
{
    dpu_error_t result;
    dpu_elf_runtime_info_t runtime_info;

    dpu_elf_get_runtime_info(*elf_info, &runtime_info);

    program->printf_buffer_address = runtime_info.printf_buffer.value;
    program->printf_buffer_size = runtime_info.printf_buffer.size;
    program->printf_write_pointer_address = runtime_info.printf_state.value;
    program->printf_buffer_has_wrapped_address = runtime_info.printf_state.value
        + sizeof(uint32_t); // sizeof(uint32_t) => size of '__stdout_buffer_state.wp' in 'stdio.c'
    program->nr_threads_enabled = (uint8_t)runtime_info.nr_threads.value;

    program->mcount_address = runtime_info.mcount.value;
    program->ret_mcount_address = runtime_info.ret_mcount.value;
    program->thread_profiling_address = runtime_info.thread_profiling.value;
    program->perfcounter_end_value_address = runtime_info.perfcounter_end_value.value;

    program->open_print_sequence_addr = runtime_info.open_print_sequence.value;
    program->close_print_sequence_addr = runtime_info.close_print_sequence.value;

    dpu_elf_symbols_t *symbols;
    dpu_elf_symbols_t *profiling_symbols;

    if ((symbols = calloc(1, sizeof(*symbols))) == NULL) {
        result = DPU_ERR_SYSTEM;
        goto free_runtime_info;
    }
    if ((profiling_symbols = calloc(1, sizeof(*profiling_symbols))) == NULL) {
        result = DPU_ERR_SYSTEM;
        goto free_symbols;
    }

    uint32_t nr_sections;
    char **section_names;

    if ((result = dpu_elf_get_sections(*elf_info, &nr_sections, &section_names)) != DPU_OK) {
        goto free_profiling_symbols;
    }
    elf_fd info = ((elf_fd)*elf_info);

    for (size_t each_section = 0; each_section < info->shnum; ++each_section) {
        char *section_name = section_names[each_section];
        if (strncmp(".data", section_name, strlen(".data")) == 0) {
            if (strcmp(".data.__sys_host", section_name) == 0) {
                if ((result = append_symbols(info->symbol_maps + each_section, symbols, mram_size_hint)) != DPU_OK) {
                    goto free_section_names;
                }
            } else if (strcmp(".data.__sys_profiling", section_name) == 0) {
                // Split profiling symbols from regular ones
                if ((result = append_symbols(info->symbol_maps + each_section, profiling_symbols, mram_size_hint)) != DPU_OK) {
                    goto free_section_names;
                }
            }
            // Other .data symbols are discarded
        } else {
            if ((result = append_symbols(info->symbol_maps + each_section, symbols, mram_size_hint)) != DPU_OK) {
                goto free_section_names;
            }
        }
    }
    program->symbols = symbols;
    program->profiling_symbols = profiling_symbols;

    free(section_names);
    free_runtime_info(&runtime_info);

    return DPU_OK;

free_section_names:
    free(section_names);
free_profiling_symbols:
    free(profiling_symbols);
free_symbols:
    free(symbols);
free_runtime_info:
    free_runtime_info(&runtime_info);
    return result;
}

__API_SYMBOL__ dpu_error_t
dpu_load_elf_program_from_memory(dpu_elf_file_t *elf_info,
    const char *path,
    uint8_t *buffer_start,
    size_t buffer_size,
    struct dpu_program_t *program,
    mram_size_t mram_size_hint)
{
    dpu_error_t err = dpu_elf_map(buffer_start, buffer_size, elf_info, NULL);
    if (err != DPU_OK) {
        return err;
    }

    if (path != NULL) {
        program->program_path = strdup(path);
    } else {
        program->program_path = NULL;
    }

    return dpu_load_elf_program_from_elf_info(elf_info, program, mram_size_hint);
}

__API_SYMBOL__ dpu_error_t
dpu_load_elf_program(dpu_elf_file_t *elf_info, const char *path, struct dpu_program_t *program, mram_size_t mram_size_hint)
{
    dpu_error_t result;
    if ((program->program_path = realpath(path, NULL)) == NULL) {
        return DPU_ERR_SYSTEM;
    }

    if ((result = dpu_elf_open(program->program_path, elf_info)) != DPU_OK) {
        free(program->program_path);
        return result;
    }

    return dpu_load_elf_program_from_elf_info(elf_info, program, mram_size_hint);
}

static dpu_error_t
append_symbols(dpu_elf_symbols_t *new_symbols, dpu_elf_symbols_t *symbols, mram_size_t mram_size_hint)
{
    uint32_t previous_nr_symbols = symbols->nr_symbols;
    uint32_t nr_symbols = new_symbols->nr_symbols + symbols->nr_symbols;
    dpu_elf_symbol_t *new_symbol_array;

    if (previous_nr_symbols == nr_symbols) {
        return DPU_OK;
    }

    if ((new_symbol_array = realloc(symbols->map, nr_symbols * sizeof(*(symbols->map)))) == NULL) {
        if (symbols->map != NULL) {
            free(symbols->map);
        }
        return DPU_ERR_SYSTEM;
    }

    symbols->map = new_symbol_array;
    symbols->nr_symbols = nr_symbols;

    memcpy(symbols->map + previous_nr_symbols, new_symbols->map, new_symbols->nr_symbols * sizeof(*(new_symbols->map)));

    for (uint32_t each_symbol = 0; each_symbol < new_symbols->nr_symbols; ++each_symbol) {
        char *symbol_name = new_symbols->map[each_symbol].name;

        dpu_elf_symbol_t *symbol = symbols->map + previous_nr_symbols + each_symbol;
        if ((symbol->name = strdup(symbol_name)) == NULL) {
            for (uint32_t each_allocated_symbol = 0; each_allocated_symbol < previous_nr_symbols + each_symbol;
                 ++each_allocated_symbol) {
                free(symbols->map[each_allocated_symbol].name);
            }
            free(symbols->map);
            return DPU_ERR_SYSTEM;
        }

        if (strcmp(symbol_name, DPU_MRAM_HEAP_POINTER_NAME) == 0) {
            symbol->size = mram_size_hint - (symbol->value & ~0x08000000u);
        }
    }

    return DPU_OK;
}

static void
free_symbols(dpu_elf_symbols_t *symbols)
{
    unsigned int nr_symbols = symbols->nr_symbols;

    for (unsigned int each_symbol = 0; each_symbol < nr_symbols; ++each_symbol) {
        free(symbols->map[each_symbol].name);
    }

    if (symbols->map != NULL) {
        free(symbols->map);
    }

    free(symbols);
}
