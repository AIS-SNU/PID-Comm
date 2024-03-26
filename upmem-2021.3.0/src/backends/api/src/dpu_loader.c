/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dpu_error.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <dpu_checkpoint.h>
#include <dpu_loader.h>
#include <dpu_profiler.h>

#include <verbose_control.h>
#include <dpu_api_log.h>

#include <gelf.h>
#include <dpu_elf_internals.h>
#include <dpu_attributes.h>
#include <dpu_rank.h>
#include <dpu_memory.h>
#include <dpu_description.h>
#include <dpu_program.h>
#include <dpu_management.h>
#include <dpu_internals.h>

#define IRAM_MASK (0x80000000)
#define MRAM_MASK (0x08000000)
#define REGS_MASK (0xa0000000)

#define IRAM_ALIGN (3)
#define WRAM_ALIGN (2)

#define ALIGN_MASK(align) (~((1 << (align)) - 1))
#define IRAM_ALIGN_MASK ALIGN_MASK(IRAM_ALIGN)
#define WRAM_ALIGN_MASK ALIGN_MASK(WRAM_ALIGN)

typedef dpu_error_t (
    *mem_load_function_t)(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);

struct dpu_load_memory_functions_t {
    mem_load_function_t load_iram;
    mem_load_function_t load_wram;
    mem_load_function_t load_mram;
    mem_load_function_t load_regs;
};

static dpu_error_t
extract_and_convert_memory_information(GElf_Phdr *phdr,
    dpu_loader_context_t context,
    struct dpu_load_memory_functions_t *load_functions,
    uint32_t *addr,
    uint32_t *size,
    uint32_t **size_accumulator,
    mem_load_function_t *do_load,
    mem_patch_function_t *do_patch);
static dpu_error_t
fetch_content(elf_fd info, GElf_Phdr *phdr, uint8_t **content);
static dpu_error_t
load_segment(elf_fd info, dpu_loader_context_t context, struct dpu_load_memory_functions_t *load_functions, GElf_Phdr *phdr);

static dpu_error_t
patch_dpu_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init);
static dpu_error_t
patch_rank_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init);
static dpu_error_t
patch_rank_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init);
static dpu_error_t
patch_dpu_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init);

static dpu_error_t
load_dpu_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_dpu_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_dpu_mram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_dpu_regs(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_rank_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_rank_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_rank_mram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);
static dpu_error_t
load_rank_regs(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size);

__API_SYMBOL__ void
dpu_loader_fill_dpu_context(dpu_loader_context_t context, struct dpu_t *dpu)
{
    context->env.target = DPU_LOADER_TARGET_DPU;
    context->env.dpu = dpu;

    context->nr_of_instructions = 0;
    context->nr_of_mram_bytes = 0;
    context->nr_of_wram_words = 0;

    context->patch_iram = patch_dpu_iram;
    context->patch_mram = NULL;
    context->patch_wram = patch_dpu_wram;
}

__API_SYMBOL__ void
dpu_loader_fill_rank_context(dpu_loader_context_t context, struct dpu_rank_t *rank)
{
    context->env.target = DPU_LOADER_TARGET_RANK;
    context->env.rank = rank;

    context->nr_of_instructions = 0;
    context->nr_of_mram_bytes = 0;
    context->nr_of_wram_words = 0;

    context->patch_iram = patch_rank_iram;
    context->patch_mram = NULL;
    context->patch_wram = patch_rank_wram;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_load(dpu_elf_file_t file, dpu_loader_context_t context)
{
    dpu_error_t status = DPU_OK;
    elf_fd info = (elf_fd)file;
    size_t phdrnum = info->phnum;
    struct dpu_load_memory_functions_t load_functions;

    switch (context->env.target) {
        case DPU_LOADER_TARGET_RANK:
            load_functions.load_iram = load_rank_iram;
            load_functions.load_wram = load_rank_wram;
            load_functions.load_mram = load_rank_mram;
            load_functions.load_regs = load_rank_regs;

            if (info->filename != NULL) {
                if ((status = dpu_custom_for_rank(
                         context->env.rank, DPU_COMMAND_BINARY_PATH, (dpu_custom_command_args_t)info->filename))
                    != DPU_OK) {
                    goto end;
                }
            }

            if ((status = dpu_custom_for_rank(
                     context->env.rank, DPU_COMMAND_EVENT_START, (dpu_custom_command_args_t)DPU_EVENT_LOAD_PROGRAM))
                != DPU_OK) {
                goto end;
            }
            break;
        case DPU_LOADER_TARGET_DPU:
            load_functions.load_iram = load_dpu_iram;
            load_functions.load_wram = load_dpu_wram;
            load_functions.load_mram = load_dpu_mram;
            load_functions.load_regs = load_dpu_regs;

            if (info->filename != NULL) {
                if ((status = dpu_custom_for_dpu(
                         context->env.dpu, DPU_COMMAND_BINARY_PATH, (dpu_custom_command_args_t)info->filename))
                    != DPU_OK) {
                    goto end;
                }
            }

            if ((status = dpu_custom_for_dpu(
                     context->env.dpu, DPU_COMMAND_EVENT_START, (dpu_custom_command_args_t)DPU_EVENT_LOAD_PROGRAM))
                != DPU_OK) {
                goto end;
            }
            break;
        default:
            status = DPU_ERR_INTERNAL;
            goto end;
    }

    for (unsigned int each_phdr = 0; each_phdr < phdrnum; ++each_phdr) {
        GElf_Phdr phdr;
        if (gelf_getphdr(info->elf, each_phdr, &phdr) != &phdr) {
            status = DPU_ERR_ELF_INVALID_FILE;
            goto end;
        }

        if (phdr.p_type == PT_LOAD) {
            if ((status = load_segment(info, context, &load_functions, &phdr)) != DPU_OK) {
                goto end;
            }
        }
    }

    switch (context->env.target) {
        case DPU_LOADER_TARGET_RANK:
            if ((status = dpu_custom_for_rank(
                     context->env.rank, DPU_COMMAND_EVENT_END, (dpu_custom_command_args_t)DPU_EVENT_LOAD_PROGRAM))
                != DPU_OK) {
                goto end;
            }
            break;
        case DPU_LOADER_TARGET_DPU:
            if ((status = dpu_custom_for_dpu(
                     context->env.dpu, DPU_COMMAND_EVENT_END, (dpu_custom_command_args_t)DPU_EVENT_LOAD_PROGRAM))
                != DPU_OK) {
                goto end;
            }
            break;
        default:
            status = DPU_ERR_INTERNAL;
            goto end;
    }

end:
    return status;
}

static dpu_error_t
load_segment(elf_fd info, dpu_loader_context_t context, struct dpu_load_memory_functions_t *load_functions, GElf_Phdr *phdr)
{
    dpu_error_t status;
    uint32_t addr;
    uint32_t size;
    mem_load_function_t do_load;
    mem_patch_function_t do_patch;
    uint8_t *content;
    uint32_t *size_accumulator;

    if ((status = extract_and_convert_memory_information(
             phdr, context, load_functions, &addr, &size, &size_accumulator, &do_load, &do_patch))
        != DPU_OK) {
        goto end;
    }

    if ((status = fetch_content(info, phdr, &content)) != DPU_OK) {
        goto end;
    }

    if ((do_patch != NULL) && ((status = do_patch(&context->env, content, addr, size, *size_accumulator == 0)) != DPU_OK)) {
        goto free_content;
    }

    if ((status = do_load(&context->env, content, addr, size)) != DPU_OK) {
        goto free_content;
    }

    *size_accumulator += size;

free_content:
    free(content);
end:
    return status;
}

static dpu_error_t
extract_and_convert_memory_information(GElf_Phdr *phdr,
    dpu_loader_context_t context,
    struct dpu_load_memory_functions_t *load_functions,
    uint32_t *addr,
    uint32_t *size,
    uint32_t **size_accumulator,
    mem_load_function_t *do_load,
    mem_patch_function_t *do_patch)
{
    *addr = (uint32_t)phdr->p_vaddr;
    *size = (uint32_t)phdr->p_memsz;

    if (*addr == REGS_MASK) {
        *do_load = load_functions->load_regs;
        *do_patch = NULL;
        *size_accumulator = &context->dummy;
    } else if ((*addr & IRAM_MASK) == IRAM_MASK) {
        if ((*addr & ~IRAM_ALIGN_MASK) != 0) {
            return DPU_ERR_INVALID_IRAM_ACCESS;
        }
        if ((*size & ~IRAM_ALIGN_MASK) != 0) {
            return DPU_ERR_INVALID_IRAM_ACCESS;
        }

        *addr = (*addr & ~IRAM_MASK) >> IRAM_ALIGN;
        *size = *size >> IRAM_ALIGN;
        *do_load = load_functions->load_iram;
        *do_patch = context->patch_iram;
        *size_accumulator = &context->nr_of_instructions;
    } else if ((*addr & MRAM_MASK) == MRAM_MASK) {
        *addr = (*addr & ~MRAM_MASK);
        *do_load = load_functions->load_mram;
        *do_patch = context->patch_mram;
        *size_accumulator = &context->nr_of_mram_bytes;
    } else {
        if ((*addr & ~WRAM_ALIGN_MASK) != 0) {
            return DPU_ERR_INVALID_WRAM_ACCESS;
        }
        if ((*size & ~WRAM_ALIGN_MASK) != 0) {
            return DPU_ERR_INVALID_WRAM_ACCESS;
        }

        *addr = *addr >> WRAM_ALIGN;
        *size = *size >> WRAM_ALIGN;
        *do_load = load_functions->load_wram;
        *do_patch = context->patch_wram;
        *size_accumulator = &context->nr_of_wram_words;
    }

    return DPU_OK;
}

static dpu_error_t
fetch_content(elf_fd info, GElf_Phdr *phdr, uint8_t **content)
{
    dpu_error_t status;

    if ((*content = calloc(1, phdr->p_memsz)) == NULL) {
        status = DPU_ERR_SYSTEM;
        goto end;
    }

    char *raw_ptr;
    size_t size;
    if ((raw_ptr = elf_rawfile(info->elf, &size)) == NULL) {
        status = DPU_ERR_ELF_INVALID_FILE;
        goto free_content;
    }

    memcpy(*content, raw_ptr + phdr->p_offset, phdr->p_filesz);

    return DPU_OK;

free_content:
    free(*content);
end:
    return status;
}

static dpu_error_t
patch_dpu_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init)
{
    return dpu_patch_profiling_for_dpu(env->dpu, content, address, size, init);
}

static dpu_error_t
patch_wram_with_dpu_frequency(dpuword_t *content, wram_addr_t addr, wram_size_t sz, struct dpu_rank_t *rank)
{

    // -1 is the default value
    if (rank->dpu_addresses.freq == DPU_ADDRESSES_FREQ_UNSET) {
        return DPU_OK;
    }

    uint32_t freq_addr = rank->dpu_addresses.freq >> WRAM_ALIGN;

    // assume that the dpu frequency is not in between two segments
    if (freq_addr < addr || freq_addr >= addr + sz)
        return DPU_OK;

    uint32_t freq = rank->description->hw.timings.fck_frequency_in_mhz;
    uint32_t div = rank->description->hw.timings.clock_division;
    uint32_t clocks_per_sec = freq * 1e6 / div;
    content[freq_addr - addr] = clocks_per_sec;

    return DPU_OK;
}

static dpu_error_t
patch_dpu_wram(dpu_loader_env_t env,
    void *content,
    dpu_mem_max_addr_t address,
    dpu_mem_max_size_t size,
    __attribute__((unused)) bool init)
{
    return patch_wram_with_dpu_frequency(content, address, size, env->dpu->rank);
}

static dpu_error_t
patch_rank_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init)
{
    if (!env->rank->profiling_context.dpu->enabled) {
        return DPU_OK;
    }

    return dpu_patch_profiling_for_dpu(env->rank->profiling_context.dpu, content, address, size, init);
}

static dpu_error_t
patch_rank_wram(dpu_loader_env_t env,
    void *content,
    dpu_mem_max_addr_t address,
    dpu_mem_max_size_t size,
    __attribute__((unused)) bool init)
{
    return patch_wram_with_dpu_frequency(content, address, size, env->rank);
}

static dpu_error_t
load_dpu_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    return dpu_copy_to_iram_for_dpu(env->dpu, (iram_addr_t)address, content, (iram_size_t)size);
}

static dpu_error_t
load_dpu_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    /* Calling 'dpu_copy_to_wram_for_dpu' with a size of '0' generate a WARNING.
     * As section like 'rodata' can often be empty, do not copy if the size is zero in order not to print an unneeded WARNING.
     */
    if (size == 0)
        return DPU_OK;

    return dpu_copy_to_wram_for_dpu(env->dpu, (wram_addr_t)address, content, (wram_size_t)size);
}

static dpu_error_t
load_dpu_mram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    return dpu_copy_to_mram(env->dpu, (mram_addr_t)address, content, (mram_size_t)size);
}

static dpu_error_t
load_dpu_regs(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    dpu_error_t status;
    struct dpu_t *dpu = env->dpu;
    if (address != REGS_MASK) {
        return DPU_ERR_SYSTEM;
    }

    struct dpu_context_t *context = malloc(sizeof(*context));
    if (context == NULL) {
        return DPU_ERR_SYSTEM;
    }

    status = dpu_checkpoint_deserialize(content, size, context);
    if (status != DPU_OK) {
        free(context);
        return status;
    }

    dpu->debug_context = context;
    status = dpu_restore_context_for_dpu(dpu, context);
    return status;
}

static dpu_error_t
load_rank_iram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    return dpu_copy_to_iram_for_rank(env->rank, (iram_addr_t)address, content, (iram_size_t)size);
}

static dpu_error_t
load_rank_wram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    return dpu_copy_to_wram_for_rank(env->rank, (wram_addr_t)address, content, (wram_size_t)size);
}

static dpu_error_t
load_rank_mram(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size)
{
    dpu_error_t status;
    struct dpu_rank_t *rank = env->rank;
    struct dpu_transfer_matrix transfer_matrix;

    dpu_transfer_matrix_set_all(rank, &transfer_matrix, content);
    transfer_matrix.offset = address;
    transfer_matrix.size = size;

    status = dpu_copy_to_mrams(rank, &transfer_matrix);

    return status;
}

static dpu_error_t
load_rank_regs(__attribute__((unused)) dpu_loader_env_t env,
    __attribute__((unused)) void *content,
    __attribute__((unused)) dpu_mem_max_addr_t address,
    __attribute__((unused)) dpu_mem_max_size_t size)
{
    LOG_RANK(WARNING, env->rank, "Cannot load a core dump elf into multiple DPUs");
    return DPU_ERR_ELF_INVALID_FILE;
}
