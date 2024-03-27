/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dpu_rank.h"
#include <stdlib.h>

#include <dpu_api_verbose.h>
#include <dpu_types.h>
#include <dpu_elf.h>
#include <dpu_error.h>
#include <dpu_description.h>
#include <dpu_management.h>
#include <dpu_loader.h>
#include <dpu_internals.h>
#include <dpu_program.h>
#include <dpu_attributes.h>
#include <dpu_log_utils.h>
#include <dpu_thread_job.h>
#include <dpu.h>

dpu_error_t
dpu_load_rank(struct dpu_rank_t *rank, struct dpu_program_t *program, dpu_elf_file_t elf_info)
{
    dpu_error_t status;
    dpu_description_t description = dpu_get_description(rank);
    uint8_t nr_of_control_interfaces = description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_of_dpus_per_control_interface = description->hw.topology.nr_of_dpus_per_control_interface;

    if ((status = dpu_fill_profiling_info(rank,
             (iram_addr_t)program->mcount_address,
             (iram_addr_t)program->ret_mcount_address,
             (wram_addr_t)program->thread_profiling_address,
             (wram_addr_t)program->perfcounter_end_value_address,
             program->profiling_symbols))
        != DPU_OK) {
        goto end;
    }

    dpu_lock_rank(rank);

    struct _dpu_loader_context_t loader_context;
    dpu_loader_fill_rank_context(&loader_context, rank);

    if ((status = dpu_elf_load(elf_info, &loader_context)) != DPU_OK) {
        goto unlock_rank;
    }

    for (dpu_slice_id_t each_slice = 0; each_slice < nr_of_control_interfaces; ++each_slice) {
        for (dpu_member_id_t each_dpu = 0; each_dpu < nr_of_dpus_per_control_interface; ++each_dpu) {
            struct dpu_t *dpu = DPU_GET_UNSAFE(rank, each_slice, each_dpu);

            if (!dpu->enabled)
                continue;

            struct dpu_program_t *previous_program = dpu_get_program(dpu);
            dpu_free_program(previous_program);
            dpu_take_program_ref(program);
            dpu_set_program(dpu, program);
        }
    }

unlock_rank:
    dpu_unlock_rank(rank);
end:
    return status;
}

static dpu_error_t
dpu_load_dpu(struct dpu_t *dpu, struct dpu_program_t *program, dpu_elf_file_t elf_info)
{
    dpu_error_t status;

    if (!dpu->enabled) {
        status = DPU_ERR_DPU_DISABLED;
        goto end;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);

    if ((status = dpu_fill_profiling_info(rank,
             (iram_addr_t)program->mcount_address,
             (iram_addr_t)program->ret_mcount_address,
             (wram_addr_t)program->thread_profiling_address,
             (wram_addr_t)program->perfcounter_end_value_address,
             program->profiling_symbols))
        != DPU_OK) {
        free(program);
        goto end;
    }

    dpu_lock_rank(rank);

    struct _dpu_loader_context_t loader_context;
    dpu_loader_fill_dpu_context(&loader_context, dpu);

    if ((status = dpu_elf_load(elf_info, &loader_context)) != DPU_OK) {
        goto unlock_rank;
    }

    struct dpu_program_t *previous_program = dpu_get_program(dpu);
    dpu_free_program(previous_program);
    dpu_take_program_ref(program);
    dpu_set_program(dpu, program);

unlock_rank:
    dpu_unlock_rank(rank);
end:
    return status;
}

typedef dpu_error_t (*load_elf_program_fct_t)(dpu_elf_file_t *elf_info,
    const char *path,
    uint8_t *buffer,
    size_t buffer_size,
    struct dpu_program_t *program,
    mram_size_t mram_size_hint);

static dpu_description_t
get_set_description(struct dpu_set_t *set)
{
    struct dpu_rank_t *rank = NULL;

    switch (set->kind) {
        case DPU_SET_RANKS:
            rank = set->list.ranks[0];
            break;
        case DPU_SET_DPU:
            rank = set->dpu->rank;
            break;
        default:
            return NULL;
    }

    return dpu_get_description(rank);
}

static dpu_error_t
dpu_load_generic(struct dpu_set_t dpu_set,
    const char *path,
    uint8_t *buffer,
    size_t buffer_size,
    struct dpu_program_t **program,
    load_elf_program_fct_t load_elf_program)
{
    dpu_error_t status;
    dpu_elf_file_t elf_info;
    struct dpu_program_t *runtime;

    if ((runtime = malloc(sizeof(*runtime))) == NULL) {
        status = DPU_ERR_SYSTEM;
        goto end;
    }
    dpu_init_program_ref(runtime);

    dpu_description_t description = get_set_description(&dpu_set);

    if ((status = load_elf_program(&elf_info, path, buffer, buffer_size, runtime, description->hw.memories.mram_size))
        != DPU_OK) {
        free(runtime);
        goto end;
    }

    uint32_t freq_addr = DPU_ADDRESSES_FREQ_UNSET;
    struct dpu_symbol_t clocks_per_sec_symbol;
    if (dpu_get_symbol(runtime, "CLOCKS_PER_SEC", &clocks_per_sec_symbol) == DPU_OK) {
        freq_addr = clocks_per_sec_symbol.address;
    }

    switch (dpu_set.kind) {
        case DPU_SET_RANKS: {
            uint32_t nr_jobs_per_rank;
            struct dpu_thread_job_sync sync;
            DPU_THREAD_JOB_GET_JOBS(
                dpu_set.list.ranks, dpu_set.list.nr_ranks, nr_jobs_per_rank, jobs, &sync, true /*sync*/, status);

            struct dpu_rank_t *rank;
            struct dpu_thread_job *job;
            DPU_THREAD_JOB_SET_JOBS(dpu_set.list.ranks, rank, dpu_set.list.nr_ranks, jobs, job, &sync, true /*sync*/, {
                rank->dpu_addresses.freq = freq_addr;
                job->type = DPU_THREAD_JOB_LOAD;
                job->load_info.runtime = runtime;
                job->load_info.elf_info = elf_info;
            });

            status = dpu_thread_job_do_jobs(dpu_set.list.ranks, dpu_set.list.nr_ranks, nr_jobs_per_rank, jobs, true, &sync);
        } break;
        case DPU_SET_DPU: {
            struct dpu_t *dpu = dpu_set.dpu;
            dpu->rank->dpu_addresses.freq = freq_addr;
            if ((status = dpu_load_dpu(dpu, runtime, elf_info)) != DPU_OK) {
                goto free_runtime;
            }
            break;
        }
        default:
            status = DPU_ERR_INTERNAL;
            goto free_runtime;
    }

    if (program != NULL) {
        *program = runtime;
    }
    goto close_elf;

free_runtime:
    runtime->reference_count = 1;
    dpu_free_program(runtime);
close_elf:
    dpu_elf_close(elf_info);
end:
    return status;
}

static dpu_error_t
__dpu_load_elf_program_from_incbin(dpu_elf_file_t *elf_info,
    const char *path,
    uint8_t *buffer,
    size_t buffer_size,
    struct dpu_program_t *program,
    mram_size_t mram_size_hint)
{
    return dpu_load_elf_program_from_memory(elf_info, path, buffer, buffer_size, program, mram_size_hint);
}

__API_SYMBOL__ dpu_error_t
dpu_load_from_memory(struct dpu_set_t dpu_set, uint8_t *buffer, size_t buffer_size, struct dpu_program_t **program)
{
    LOG_FN(DEBUG, "%p %lu", buffer, buffer_size);

    return dpu_load_generic(dpu_set, NULL, buffer, buffer_size, program, __dpu_load_elf_program_from_incbin);
}

__API_SYMBOL__ dpu_error_t
dpu_load_from_incbin(struct dpu_set_t dpu_set, struct dpu_incbin_t *incbin, struct dpu_program_t **program)
{
    LOG_FN(INFO, "%p %zu %s", incbin->buffer, incbin->size, incbin->path);

    return dpu_load_generic(dpu_set, incbin->path, incbin->buffer, incbin->size, program, __dpu_load_elf_program_from_incbin);
}

static dpu_error_t
__dpu_load_elf_program(dpu_elf_file_t *elf_info,
    const char *path,
    __attribute__((unused)) uint8_t *buffer,
    __attribute__((unused)) size_t buffer_size,
    struct dpu_program_t *program,
    mram_size_t mram_size_hint)
{
    return dpu_load_elf_program(elf_info, path, program, mram_size_hint);
}

__API_SYMBOL__ dpu_error_t
dpu_load(struct dpu_set_t dpu_set, const char *binary_path, struct dpu_program_t **program)
{
    LOG_FN(INFO, "\"%s\"", binary_path);

    return dpu_load_generic(dpu_set, binary_path, NULL, 0, program, __dpu_load_elf_program);
}
