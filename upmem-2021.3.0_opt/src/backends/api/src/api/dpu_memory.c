/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_transfer_matrix.h>
#include <dpu.h>
#include <dpu_api_verbose.h>
#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_log_utils.h>
#include <dpu_rank.h>
#include <dpu_management.h>
#include <dpu_program.h>
#include <dpu_memory.h>
#include <dpu_internals.h>
#include <dpu_attributes.h>
#include <dpu_thread_job.h>

#include <stdint.h>
#include <sys/time.h>
#define T int32_t

#define IRAM_MASK (0x80000000u)
#define MRAM_MASK (0x08000000u)

#define IRAM_ALIGN (3u)
#define WRAM_ALIGN (2u)

#define ALIGN_MASK(align) (~((1u << (align)) - 1u))
#define IRAM_ALIGN_MASK ALIGN_MASK(IRAM_ALIGN)
#define WRAM_ALIGN_MASK ALIGN_MASK(WRAM_ALIGN)

#define MEMORY_SWITCH(address, iram_statement, mram_statement, wram_statement)                                                   \
    do {                                                                                                                         \
        if (((address)&IRAM_MASK) == IRAM_MASK) {                                                                                \
            iram_statement;                                                                                                      \
        } else if (((address)&MRAM_MASK) == MRAM_MASK) {                                                                         \
            mram_statement;                                                                                                      \
        } else {                                                                                                                 \
            wram_statement;                                                                                                      \
        }                                                                                                                        \
    } while (0)

#define UPDATE_MRAM_COPY_PARAMETERS(address, length)                                                                             \
    do {                                                                                                                         \
        (address) &= ~MRAM_MASK;                                                                                                 \
    } while (0)

#define UPDATE_WRAM_COPY_PARAMETERS(address, length)                                                                             \
    do {                                                                                                                         \
        (address) >>= WRAM_ALIGN;                                                                                                \
        (length) >>= WRAM_ALIGN;                                                                                                 \
    } while (0)

#define UPDATE_IRAM_COPY_PARAMETERS(address, length)                                                                             \
    do {                                                                                                                         \
        (address) = ((address) & ~IRAM_MASK) >> IRAM_ALIGN;                                                                      \
        (length) >>= IRAM_ALIGN;                                                                                                 \
    } while (0)

#define MEMORY_SWITCH_AND_UPDATE_COPY_PARAMETERS(address, length, iram_statement, mram_statement, wram_statement)                \
    do {                                                                                                                         \
        MEMORY_SWITCH((address), iram_statement; UPDATE_IRAM_COPY_PARAMETERS((address), (length)), mram_statement;               \
                      UPDATE_MRAM_COPY_PARAMETERS((address), (length));                                                          \
                      , wram_statement;                                                                                          \
                      UPDATE_WRAM_COPY_PARAMETERS((address), (length)));                                                         \
    } while (0)

#define CHECK_SYMBOL(symbol, symbol_offset, length)                                                                              \
    do {                                                                                                                         \
        dpu_mem_max_addr_t _address = (symbol).address + (symbol_offset);                                                        \
        if (((symbol_offset) + (length)) > (symbol).size) {                                                                      \
            LOG_FN(WARNING, "invalid symbol access (offset:%u + length:%u > size:%u)", symbol_offset, length, (symbol).size);    \
            return DPU_ERR_INVALID_SYMBOL_ACCESS;                                                                                \
        }                                                                                                                        \
        MEMORY_SWITCH(                                                                                                           \
            _address,                                                                                                            \
            if ((_address & ~IRAM_ALIGN_MASK) != 0) {                                                                            \
                LOG_FN(WARNING, "invalid iram access (offset:0x%x, address:0x%x)", symbol_offset, (symbol).address);             \
                return DPU_ERR_INVALID_IRAM_ACCESS;                                                                              \
            } if (((length) & ~IRAM_ALIGN_MASK) != 0) {                                                                          \
                LOG_FN(WARNING, "invalid iram access (length:0x%x)", length);                                                    \
                return DPU_ERR_INVALID_IRAM_ACCESS;                                                                              \
            },                                                                                                                   \
            /* All alignment are allowed */,                                                                                     \
            if ((_address & ~WRAM_ALIGN_MASK) != 0) {                                                                            \
                LOG_FN(WARNING, "invalid wram access (offset:0x%x, address:0x%x)", symbol_offset, (symbol).address);             \
                return DPU_ERR_INVALID_WRAM_ACCESS;                                                                              \
            } if (((length) & ~WRAM_ALIGN_MASK) != 0) {                                                                          \
                LOG_FN(WARNING, "invalid wram access (length:0x%x)", length);                                                    \
                return DPU_ERR_INVALID_WRAM_ACCESS;                                                                              \
            });                                                                                                                  \
    } while (0)

#define DPU_BROADCAST_SET_JOB_TYPE(job_type, address, length)                                                                    \
    do {                                                                                                                         \
        MEMORY_SWITCH_AND_UPDATE_COPY_PARAMETERS(address,                                                                        \
            length,                                                                                                              \
            (job_type) = DPU_THREAD_JOB_COPY_IRAM_TO_RANK,                                                                       \
            (job_type) = DPU_THREAD_JOB_COPY_MRAM_TO_RANK,                                                                       \
            (job_type) = DPU_THREAD_JOB_COPY_WRAM_TO_RANK);                                                                      \
    } while (0)

#define DPU_COPY_MATRIX_SET_JOB_TYPE(job_type, xfer, address, length)                                                            \
    do {                                                                                                                         \
        switch ((xfer)) {                                                                                                        \
            case DPU_XFER_TO_DPU:                                                                                                \
                MEMORY_SWITCH_AND_UPDATE_COPY_PARAMETERS(address,                                                                \
                    length,                                                                                                      \
                    (job_type) = DPU_THREAD_JOB_COPY_IRAM_TO_MATRIX,                                                             \
                    (job_type) = DPU_THREAD_JOB_COPY_MRAM_TO_MATRIX,                                                             \
                    (job_type) = DPU_THREAD_JOB_COPY_WRAM_TO_MATRIX);                                                            \
                break;                                                                                                           \
            case DPU_XFER_FROM_DPU:                                                                                              \
                MEMORY_SWITCH_AND_UPDATE_COPY_PARAMETERS(address,                                                                \
                    length,                                                                                                      \
                    (job_type) = DPU_THREAD_JOB_COPY_IRAM_FROM_MATRIX,                                                           \
                    (job_type) = DPU_THREAD_JOB_COPY_MRAM_FROM_MATRIX,                                                           \
                    (job_type) = DPU_THREAD_JOB_COPY_WRAM_FROM_MATRIX);                                                          \
                break;                                                                                                           \
            default:                                                                                                             \
                return DPU_ERR_INVALID_MEMORY_TRANSFER;                                                                          \
        }                                                                                                                        \
    } while (0)

#define SYNCHRONOUS_FLAGS(flags) ((DPU_XFER_ASYNC & flags) == 0)

static dpu_error_t
dpu_copy_symbol_dpu(struct dpu_t *dpu,
    struct dpu_symbol_t symbol,
    uint32_t symbol_offset,
    void *buffer,
    size_t length,
    dpu_xfer_t xfer,
    dpu_xfer_flags_t flags)
{
    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }
    CHECK_SYMBOL(symbol, symbol_offset, length);

    dpu_mem_max_addr_t address = symbol.address + symbol_offset;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_error_t status = DPU_OK;

    enum dpu_thread_job_type job_type;
    DPU_COPY_MATRIX_SET_JOB_TYPE(job_type, xfer, address, length);

    uint32_t nr_jobs_per_rank;
    struct dpu_thread_job_sync sync;
    DPU_THREAD_JOB_GET_JOBS(&rank, 1, nr_jobs_per_rank, jobs, &sync, SYNCHRONOUS_FLAGS(flags), status);

    struct dpu_rank_t *rrank __attribute__((unused));
    struct dpu_thread_job *job;
    DPU_THREAD_JOB_SET_JOBS(&rank, rrank, 1, jobs, job, &sync, SYNCHRONOUS_FLAGS(flags), {
        job->type = job_type;
        dpu_transfer_matrix_clear_all(rank, &job->matrix);
        dpu_transfer_matrix_add_dpu(dpu, &job->matrix, buffer);
        job->matrix.offset = address;
        job->matrix.size = length;
    });

    status = dpu_thread_job_do_jobs(&rank, 1, nr_jobs_per_rank, jobs, SYNCHRONOUS_FLAGS(flags), &sync);

    return status;
}

static dpu_error_t
dpu_broadcast_to_symbol_for_ranks(struct dpu_rank_t **ranks,
    uint32_t nr_ranks,
    struct dpu_symbol_t symbol,
    uint32_t symbol_offset,
    const void *src,
    size_t length,
    dpu_xfer_flags_t flags)
{
    CHECK_SYMBOL(symbol, symbol_offset, length);

    dpu_error_t status = DPU_OK;
    dpu_mem_max_addr_t address = symbol.address + symbol_offset;

    enum dpu_thread_job_type job_type;
    DPU_BROADCAST_SET_JOB_TYPE(job_type, address, length);

    uint32_t nr_jobs_per_rank;
    struct dpu_thread_job_sync sync;
    DPU_THREAD_JOB_GET_JOBS(ranks, nr_ranks, nr_jobs_per_rank, jobs, &sync, SYNCHRONOUS_FLAGS(flags), status);

    struct dpu_rank_t *rank __attribute__((unused));
    struct dpu_thread_job *job;
    DPU_THREAD_JOB_SET_JOBS(ranks, rank, nr_ranks, jobs, job, &sync, SYNCHRONOUS_FLAGS(flags), {
        job->type = job_type;
        job->address = address;
        job->length = length;
        job->buffer = src;
    });

    status = dpu_thread_job_do_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs, SYNCHRONOUS_FLAGS(flags), &sync);

    return status;
}

static const char *
dpu_transfer_to_string(dpu_xfer_t transfer)
{
    switch (transfer) {
        case DPU_XFER_TO_DPU:
            return "HOST_TO_DPU";
        case DPU_XFER_FROM_DPU:
            return "DPU_TO_HOST";
        default:
            return "UNKNOWN";
    }
}

static dpu_error_t
dpu_get_common_program(struct dpu_set_t *dpu_set, struct dpu_program_t **program)
{
    struct dpu_program_t *the_program = NULL;

    switch (dpu_set->kind) {
        case DPU_SET_RANKS:
            for (uint32_t each_rank = 0; each_rank < dpu_set->list.nr_ranks; ++each_rank) {
                struct dpu_rank_t *rank = dpu_set->list.ranks[each_rank];
                uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
                uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;

                for (int each_ci = 0; each_ci < nr_cis; ++each_ci) {
                    for (int each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
                        struct dpu_t *dpu = DPU_GET_UNSAFE(rank, each_ci, each_dpu);

                        if (!dpu_is_enabled(dpu)) {
                            continue;
                        }

                        struct dpu_program_t *dpu_program = dpu_get_program(dpu);

                        if (the_program == NULL) {
                            the_program = dpu_program;
                        }

                        if (the_program != dpu_program) {
                            return DPU_ERR_DIFFERENT_DPU_PROGRAMS;
                        }
                    }
                }
            }
            break;
        case DPU_SET_DPU:
            the_program = dpu_get_program(dpu_set->dpu);
            break;
        default:
            return DPU_ERR_INTERNAL;
    }
    if (the_program == NULL) {
        return DPU_ERR_NO_PROGRAM_LOADED;
    }

    *program = the_program;
    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_get_symbol(struct dpu_program_t *program, const char *symbol_name, struct dpu_symbol_t *symbol)
{
    LOG_FN(DEBUG, "\"%s\"", symbol_name);

    dpu_error_t status = DPU_OK;

    uint32_t nr_symbols = program->symbols->nr_symbols;

    for (uint32_t each_symbol = 0; each_symbol < nr_symbols; ++each_symbol) {
        dpu_elf_symbol_t *elf_symbol = program->symbols->map + each_symbol;
        if (strcmp(symbol_name, elf_symbol->name) == 0) {
            symbol->address = elf_symbol->value;
            symbol->size = elf_symbol->size;
            goto end;
        }
    }

    status = DPU_ERR_UNKNOWN_SYMBOL;

end:
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_broadcast_to(struct dpu_set_t dpu_set,
    const char *symbol_name,
    uint32_t symbol_offset,
    const void *src,
    size_t length,
    dpu_xfer_flags_t flags)
{
    LOG_FN(DEBUG, "%s, %d, %zd, 0x%x", symbol_name, symbol_offset, length, flags);
    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol;

    if ((status = dpu_get_common_program(&dpu_set, &program)) != DPU_OK) {
        return status;
    }

    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_broadcast_to_symbol(dpu_set, symbol, symbol_offset, src, length, flags);
}

__API_SYMBOL__ dpu_error_t
dpu_broadcast_to_symbol(struct dpu_set_t dpu_set,
    struct dpu_symbol_t symbol,
    uint32_t symbol_offset,
    const void *src,
    size_t length,
    dpu_xfer_flags_t flags)
{
    LOG_FN(DEBUG, "0x%08x, %d, %d, %zd, 0x%x", symbol.address, symbol.size, symbol_offset, length, flags);
    dpu_error_t status = DPU_OK;

    switch (dpu_set.kind) {
        case DPU_SET_RANKS:
            status = dpu_broadcast_to_symbol_for_ranks(
                dpu_set.list.ranks, dpu_set.list.nr_ranks, symbol, symbol_offset, src, length, flags);
            break;
        case DPU_SET_DPU:
            status = dpu_copy_symbol_dpu(dpu_set.dpu, symbol, symbol_offset, (void *)src, length, DPU_XFER_TO_DPU, flags);
            break;
        default:
            return DPU_ERR_INTERNAL;
    }

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_copy_to(struct dpu_set_t dpu_set, const char *symbol_name, uint32_t symbol_offset, const void *src, size_t length)
{
    LOG_FN(DEBUG, "\"%s\", %d, %p, %zd)", symbol_name, symbol_offset, src, length);

    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol;

    if ((status = dpu_get_common_program(&dpu_set, &program)) != DPU_OK) {
        return status;
    }

    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_broadcast_to_symbol(dpu_set, symbol, symbol_offset, src, length, DPU_XFER_DEFAULT);
}

__API_SYMBOL__ dpu_error_t
dpu_copy_from(struct dpu_set_t dpu_set, const char *symbol_name, uint32_t symbol_offset, void *dst, size_t length)
{
    LOG_FN(DEBUG, "\"%s\", %d, %p, %zd)", symbol_name, symbol_offset, dst, length);

    if (dpu_set.kind != DPU_SET_DPU) {
        return dpu_set.kind == DPU_SET_RANKS ? DPU_ERR_INVALID_DPU_SET : DPU_ERR_INTERNAL;
    }

    struct dpu_t *dpu = dpu_set.dpu;
    dpu_error_t status = DPU_OK;

    struct dpu_symbol_t symbol;
    struct dpu_program_t *program;

    if ((program = dpu_get_program(dpu)) == NULL) {
        return DPU_ERR_NO_PROGRAM_LOADED;
    }

    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_copy_symbol_dpu(dpu_set.dpu, symbol, symbol_offset, dst, length, DPU_XFER_FROM_DPU, DPU_XFER_DEFAULT);
}

__API_SYMBOL__ dpu_error_t
dpu_copy_to_symbol(struct dpu_set_t dpu_set, struct dpu_symbol_t symbol, uint32_t symbol_offset, const void *src, size_t length)
{
    LOG_FN(DEBUG, "0x%08x, %d, %d, %p, %zd)", symbol.address, symbol.size, symbol_offset, src, length);

    return dpu_broadcast_to_symbol(dpu_set, symbol, symbol_offset, src, length, DPU_XFER_DEFAULT);
}

__API_SYMBOL__ dpu_error_t
dpu_copy_from_symbol(struct dpu_set_t dpu_set, struct dpu_symbol_t symbol, uint32_t symbol_offset, void *dst, size_t length)
{
    LOG_FN(DEBUG, "0x%08x, %d, %d, %p, %zd)", symbol.address, symbol.size, symbol_offset, dst, length);

    if (dpu_set.kind != DPU_SET_DPU) {
        return dpu_set.kind == DPU_SET_RANKS ? DPU_ERR_INVALID_DPU_SET : DPU_ERR_INTERNAL;
    }

    return dpu_copy_symbol_dpu(dpu_set.dpu, symbol, symbol_offset, dst, length, DPU_XFER_FROM_DPU, DPU_XFER_DEFAULT);
}

struct dpu_transfer_matrix *
dpu_get_transfer_matrix(struct dpu_rank_t *rank)
{
    if (rank->api.callback_tid_set && rank->api.callback_tid == pthread_self()) {
        return &rank->api.callback_matrix;
    } else {
        return &rank->api.matrix;
    }
}

__API_SYMBOL__ dpu_error_t
dpu_prepare_xfer(struct dpu_set_t dpu_set, void *buffer)
{
    LOG_FN(DEBUG, "%p", buffer);

    switch (dpu_set.kind) {
        case DPU_SET_RANKS:
            for (uint32_t each_rank = 0; each_rank < dpu_set.list.nr_ranks; ++each_rank) {
                struct dpu_rank_t *rank = dpu_set.list.ranks[each_rank];
                uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
                uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;

                for (uint8_t each_ci = 0; each_ci < nr_cis; ++each_ci) {
                    for (uint8_t each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
                        struct dpu_t *dpu = DPU_GET_UNSAFE(rank, each_ci, each_dpu);

                        if (!dpu_is_enabled(dpu)) {
                            continue;
                        }

                        dpu_transfer_matrix_add_dpu(dpu, dpu_get_transfer_matrix(rank), buffer);
                    }
                }
            }

            break;
        case DPU_SET_DPU: {
            struct dpu_t *dpu = dpu_set.dpu;

            if (!dpu_is_enabled(dpu)) {
                return DPU_ERR_DPU_DISABLED;
            }

            dpu_transfer_matrix_add_dpu(dpu, dpu_get_transfer_matrix(dpu_get_rank(dpu)), buffer);

            break;
        }
        default:
            return DPU_ERR_INTERNAL;
    }

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_push_xfer(struct dpu_set_t dpu_set,
    dpu_xfer_t xfer,
    const char *symbol_name,
    uint32_t symbol_offset,
    size_t length,
    dpu_xfer_flags_t flags)
{
    LOG_FN(DEBUG, "%s, %s, %d, %zd, 0x%x", dpu_transfer_to_string(xfer), symbol_name, symbol_offset, length, flags);

    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol;

    if ((status = dpu_get_common_program(&dpu_set, &program)) != DPU_OK) {
        return status;
    }

    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_push_xfer_symbol(dpu_set, xfer, symbol, symbol_offset, length, flags);
}

__API_SYMBOL__ dpu_error_t
dpu_push_xfer_symbol(struct dpu_set_t dpu_set,
    dpu_xfer_t xfer,
    struct dpu_symbol_t symbol,
    uint32_t symbol_offset,
    size_t length,
    dpu_xfer_flags_t flags)
{
        
    LOG_FN(DEBUG,
        "%s, 0x%08x, %d, %d, %zd, 0x%x",
        dpu_transfer_to_string(xfer),
        symbol.address,
        symbol.size,
        symbol_offset,
        length,
        flags);

    dpu_error_t status = DPU_OK;
    dpu_mem_max_addr_t address = symbol.address + symbol_offset;

    switch (dpu_set.kind) 
    {
        case DPU_SET_RANKS: 
        {
            CHECK_SYMBOL(symbol, symbol_offset, length);

            uint32_t nr_ranks = dpu_set.list.nr_ranks;
            struct dpu_rank_t **ranks = dpu_set.list.ranks;

            enum dpu_thread_job_type job_type;
            DPU_COPY_MATRIX_SET_JOB_TYPE(job_type, xfer, address, length);

            uint32_t nr_jobs_per_rank;
            struct dpu_thread_job_sync sync;
            DPU_THREAD_JOB_GET_JOBS(ranks, nr_ranks, nr_jobs_per_rank, jobs, &sync, SYNCHRONOUS_FLAGS(flags), status);

            struct dpu_rank_t *rank;
            struct dpu_thread_job *job;
            DPU_THREAD_JOB_SET_JOBS(ranks, rank, nr_ranks, jobs, job, &sync, SYNCHRONOUS_FLAGS(flags), 
            {
                dpu_transfer_matrix_copy(&job->matrix, dpu_get_transfer_matrix(rank));
                job->type = job_type;
                job->matrix.offset = address;
                job->matrix.size = length;
            });

            status = dpu_thread_job_do_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs, SYNCHRONOUS_FLAGS(flags), &sync);

        } break;
        case DPU_SET_DPU: 
        {
            struct dpu_t *dpu = dpu_set.dpu;
            struct dpu_rank_t *rank = dpu_get_rank(dpu);
            uint8_t dpu_transfer_matrix_index = get_transfer_matrix_index(rank, dpu_get_slice_id(dpu), dpu_get_member_id(dpu));
            void *buffer = dpu_get_transfer_matrix(rank)->ptr[dpu_transfer_matrix_index];

            status = dpu_copy_symbol_dpu(dpu, symbol, symbol_offset, buffer, length, xfer, flags);

            break;
        }
        default:
            return DPU_ERR_INTERNAL;
    }

    if (flags != DPU_XFER_NO_RESET) {
        switch (dpu_set.kind) {
            case DPU_SET_RANKS:
                for (uint32_t each_rank = 0; each_rank < dpu_set.list.nr_ranks; ++each_rank) {
                    struct dpu_rank_t *rank = dpu_set.list.ranks[each_rank];
                    dpu_transfer_matrix_clear_all(rank, dpu_get_transfer_matrix(rank));
                }
                break;
            case DPU_SET_DPU: {
                struct dpu_t *dpu = dpu_set.dpu;
                dpu_transfer_matrix_clear_dpu(dpu, dpu_get_transfer_matrix(dpu_get_rank(dpu)));
                break;
            }
            default:
                return DPU_ERR_INTERNAL;
        }
    }

    return status;
}

/* Custom Functions */
__API_SYMBOL__ dpu_error_t
make_rnc_threads(
    __attribute__((unused)) RNS_Job_Queue_t* job_queue)
{
    __attribute__((unused)) dpu_error_t status;

    for (int r = 0; r < job_queue->num_ranks; r++)
    {
        struct dpu_program_t *program;
        struct dpu_symbol_t symbol_src;

        if ((status = dpu_get_common_program(job_queue->rank_sets[r], &program)) != DPU_OK) 
        {
            return status;
        }
        if ((status = dpu_get_symbol(program, DPU_MRAM_HEAP_POINTER_NAME, &symbol_src)) != DPU_OK) 
        {
            return status;
        } 

        job_queue->dpu_rank_strcts[r] = job_queue->rank_sets[r]->list.ranks[0];
    }

    status = job_queue->dpu_rank_strcts[0]->handler_context->handler->do_rnc(job_queue);

    return DPU_OK;
}

__API_SYMBOL__  dpu_error_t
do_rnc_job(
    rotate_n_stream_job_t* job)
{
    ////////////////////////////////////
    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol_src;
    struct dpu_symbol_t symbol_dst;

    if ((status = dpu_get_common_program(job->src_rank_dpu_set, &program)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_symbol(program, DPU_MRAM_HEAP_POINTER_NAME, &symbol_src)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_common_program(job->dst_rank_dpu_set, &program)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_symbol(program, DPU_MRAM_HEAP_POINTER_NAME, &symbol_dst)) != DPU_OK) 
    {
        return status;
    }

    __attribute__((unused)) dpu_mem_max_addr_t address_src = symbol_src.address + job->mram_src_offset;
    __attribute__((unused)) dpu_mem_max_addr_t address_dst = symbol_dst.address + job->mram_dst_offset;

    __attribute__((unused)) struct dpu_rank_t* dpu_rank_src = job->src_rank_dpu_set->list.ranks[0];
    __attribute__((unused)) struct dpu_rank_t* dpu_rank_dst = job->dst_rank_dpu_set->list.ranks[0];
    
    ////////////////////////////////////

    return DPU_OK;

}

__API_SYMBOL__ dpu_error_t
dpu_RNS(
    struct dpu_set_t dpu_set_src,
    struct dpu_set_t dpu_set_dst,
    dpu_xfer_t xfer,
    const char *symbol_name,
    uint32_t symbol_offset_src,
    uint32_t symbol_offset_dst,
    size_t length,
    dpu_xfer_flags_t flags)
{
    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol_src;
    struct dpu_symbol_t symbol_dst;

    if ((status = dpu_get_common_program(&dpu_set_src, &program)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_symbol(program, symbol_name, &symbol_src)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_common_program(&dpu_set_dst, &program)) != DPU_OK) 
    {
        return status;
    }
    if ((status = dpu_get_symbol(program, symbol_name, &symbol_dst)) != DPU_OK) 
    {
        return status;
    }
    

    return dpu_RNS_symbol(
        dpu_set_src, 
        dpu_set_dst, 
        xfer, 
        symbol_src, 
        symbol_dst, 
        symbol_offset_src, 
        symbol_offset_dst, 
        length, flags);
}


__API_SYMBOL__ dpu_error_t
dpu_RNS_symbol(
    struct dpu_set_t dpu_set_src,
    struct dpu_set_t dpu_set_dst,
    dpu_xfer_t xfer,
    struct dpu_symbol_t symbol_src,
    struct dpu_symbol_t symbol_dst,
    uint32_t symbol_offset_src,
    uint32_t symbol_offset_dst,
    size_t length,
    dpu_xfer_flags_t flags)
{
    // LOG_FN(DEBUG,
    //     "%s, 0x%08x, %d, %d, %zd, 0x%x",
    //     dpu_transfer_to_string(xfer),
    //     symbol.address,
    //     symbol.size,
    //     symbol_offset,
    //     length,
    //     flags);

    dpu_error_t status = DPU_OK;
    
    dpu_mem_max_addr_t address_src = symbol_src.address + symbol_offset_src;
    dpu_mem_max_addr_t address_dst = symbol_dst.address + symbol_offset_dst;

    switch (dpu_set_src.kind) 
    {
        case DPU_SET_RANKS: 
        {
            CHECK_SYMBOL(symbol_src, symbol_offset_src, length);

            uint32_t nr_ranks_src = dpu_set_src.list.nr_ranks;
            struct dpu_rank_t **ranks_src = dpu_set_src.list.ranks;

            enum dpu_thread_job_type job_type_src;
            DPU_COPY_MATRIX_SET_JOB_TYPE(job_type_src, xfer, address_src, length);

            uint32_t nr_jobs_per_rank_src;
            struct dpu_thread_job_sync sync_src;
            DPU_THREAD_JOB_GET_JOBS_RNS(ranks_src, nr_ranks_src, nr_jobs_per_rank_src, jobs_src, static_job_src, &sync_src, SYNCHRONOUS_FLAGS(flags), status);

            struct dpu_rank_t *rank_src;
            struct dpu_thread_job *job_src;
            
            DPU_THREAD_JOB_SET_JOBS(ranks_src, rank_src, nr_ranks_src, jobs_src, job_src, &sync_src, SYNCHRONOUS_FLAGS(flags), 
            {
                dpu_transfer_matrix_copy(&job_src->matrix, dpu_get_transfer_matrix(rank_src));
                job_src->type = job_type_src;
                job_src->matrix.offset = address_src;
                job_src->matrix.size = length;
            });

            CHECK_SYMBOL(symbol_dst, symbol_offset_dst, length);

            uint32_t nr_ranks_dst = dpu_set_dst.list.nr_ranks;
            struct dpu_rank_t **ranks_dst = dpu_set_dst.list.ranks;

            enum dpu_thread_job_type job_type_dst;
            DPU_COPY_MATRIX_SET_JOB_TYPE(job_type_dst, xfer, address_dst, length);

            uint32_t nr_jobs_per_rank_dst;
            struct dpu_thread_job_sync sync_dst;
            DPU_THREAD_JOB_GET_JOBS_RNS(ranks_dst, nr_ranks_dst, nr_jobs_per_rank_dst, jobs_dst, static_job_dst, &sync_dst, SYNCHRONOUS_FLAGS(flags), status);

            struct dpu_rank_t *rank_dst;
            struct dpu_thread_job *job_dst;
            
            DPU_THREAD_JOB_SET_JOBS(ranks_dst, rank_dst, nr_ranks_dst, jobs_dst, job_dst, &sync_dst, SYNCHRONOUS_FLAGS(flags), 
            {
                dpu_transfer_matrix_copy(&job_dst->matrix, dpu_get_transfer_matrix(rank_dst));
                job_dst->type = job_type_dst;
                job_dst->matrix.offset = address_dst;
                job_dst->matrix.size = length;
            });

            // bool ignored;
            dpu_lock_rank(ranks_src[0]);
            dpu_lock_rank(ranks_dst[0]);
            status = dpu_copy_mrams_RNS(ranks_src[0], ranks_dst[0], &jobs_src[0]->matrix, &jobs_dst[0]->matrix);
            dpu_unlock_rank(ranks_dst[0]);
            dpu_unlock_rank(ranks_src[0]);
            // status = dpu_thread_job_do_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs, SYNCHRONOUS_FLAGS(flags), &sync);

        } break;
        default:
            return DPU_ERR_INTERNAL;
    }
    
    return status;
}

//gather
/*__API_SYMBOL__ dpu_error_t
gather(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis, host_buffer);
    return status;
}*/

__API_SYMBOL__ dpu_error_t
gather(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    dimension+=0; alltoall_comm_type+=0; comm_axis[0]+=0;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, axis_len[0], axis_len[1], axis_len[2], 0, communication_buffer_offset, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
gather_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
gather_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
gather_z(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void **host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_z_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, target_dpu_index, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
gather_xz(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void **host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->gather_xz_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, target_dpu_index, host_buffer);
    return status;
}

//reduce
__API_SYMBOL__ dpu_error_t
reduce(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t total_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t data_type, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, total_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis, data_type, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
reduce_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, data_type, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
reduce_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, data_type, host_buffer);
    return status;
}

//scatter
__API_SYMBOL__ dpu_error_t
scatter(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->scatter_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
scatter_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->scatter_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
scatter_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->scatter_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, host_buffer);
    return status;
}

__API_SYMBOL__ dpu_error_t
reduce_scatter(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_scatter_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis, size);
    return status;
}

__API_SYMBOL__ dpu_error_t
reduce_scatter_cpu_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_scatter_cpu_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, size);
    return status;
}

__API_SYMBOL__ dpu_error_t
reduce_scatter_cpu_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->reduce_scatter_cpu_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, size);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_reduce(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, uint32_t reduce_type)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_reduce_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis, size, reduce_type);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_reduce_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_reduce_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, size, reduce_type);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_reduce_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_reduce_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset, size, reduce_type);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_gather(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_gather_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_gather_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_gather_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_gather_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_gather_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_gather_z(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_gather_z_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_gather_xz(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_gather_xz_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_to_all(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_to_all_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, comm_type, communication_buffer_offset, dimension, axis_len, comm_axis);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_to_all_x(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_to_all_x_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_to_all_xz(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_to_all_xz_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_to_all_y(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_to_all_y_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}

__API_SYMBOL__ dpu_error_t
all_to_all_z(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset)
{
    dpu_error_t status = DPU_OK;
    struct dpu_rank_t *rank_set = comm_dpu_set->list.ranks[0];
    status = rank_set->handler_context->handler->all_to_all_z_rns(comm_dpu_set, src_start_offset, dst_start_offset, byte_length, a, b, c, alltoall_comm_type, communication_buffer_offset);
    return status;
}



/*PID-Comm*/
#ifndef DPU_BINARY_RELOCATE_CLOCKWISE
#define DPU_BINARY_RELOCATE_CLOCKWISE "./bin/data_relocate_clockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_CLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_CLOCKWISE_INT8 "./bin/data_relocate_clockwise_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_CLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_CLOCKWISE_INT32 "./bin/data_relocate_clockwise_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE "./bin/data_relocate_reverse_clockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_INT8 "./bin/data_relocate_reverse_clockwise_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_INT32 "./bin/data_relocate_reverse_clockwise_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_COUNTERCLOCKWISE
#define DPU_BINARY_RELOCATE_COUNTERCLOCKWISE "./bin/data_relocate_counterclockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT8 "./bin/data_relocate_counterclockwise_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT32 "./bin/data_relocate_counterclockwise_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE
#define DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE "./bin/data_relocate_incremental_counterclockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT8 "./bin/data_relocate_incremental_counterclockwise_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT32 "./bin/data_relocate_incremental_counterclockwise_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE
#define DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE "./bin/data_relocate_modified_clockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT8 "./bin/data_relocate_modified_clockwise_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT32 "./bin/data_relocate_modified_clockwise_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_CLOCKWISE_SHORT
#define DPU_BINARY_RELOCATE_CLOCKWISE_SHORT "./bin/data_relocate_clockwise_short"
#endif
#ifndef DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT8
#define DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT8 "./bin/data_relocate_clockwise_short_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT32
#define DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT32 "./bin/data_relocate_clockwise_short_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE
#define DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE "./bin/data_relocate_modified_reverse_clockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT8
#define DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT8 "./bin/data_relocate_modified_reverse_clockwise"
#endif
#ifndef DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT32
#define DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT32 "./bin/data_relocate_modified_reverse_clockwise"
#endif

#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT "./bin/data_relocate_reverse_clockwise_short"
#endif
#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT8
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT8 "./bin/data_relocate_reverse_clockwise_short_int8"
#endif
#ifndef DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT32
#define DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT32 "./bin/data_relocate_reverse_clockwise_short_int32"
#endif



typedef struct {
    uint32_t start_offset;
    uint32_t target_offset;
    uint32_t total_data_size;
    uint32_t num_comm_dpu;
    uint32_t each_dpu;
    bool no_rotate;
    uint32_t num_row;
    uint32_t comm_type;
    uint32_t a_length;
    uint32_t num_comm_rg;
} dpu_arguments_comm_t;

//Timer.h
typedef struct Timer{
    struct timeval startTime[12];
    struct timeval stopTime[12];
    double         time[12];

} Timer;

static void resetTimer(Timer *timer){
    for(int i=0; i<12; i++){
        timer->time[i] = 0.0;
    }
}

static void startTimer(Timer *timer, int i) {
    timer->time[i]=0.0;
    gettimeofday(&timer->startTime[i], NULL);
}

static void stopTimer(Timer *timer, int i) {
    gettimeofday(&timer->stopTime[i], NULL);
    timer->time[i] += (timer->stopTime[i].tv_sec - timer->startTime[i].tv_sec) * 1000000.0 +
        (timer->stopTime[i].tv_usec - timer->startTime[i].tv_usec);
}

static void printTimer(Timer *timer, int i) { 
    if (i == 0 || i == 2)
        printf("Time (ms): \t\t\t%f\n", timer->time[i] / (1000)); 
    else if (i == 1)
        printf("Time (ms): \t\t%f\n", timer->time[i] / (1000)); 
    else
        printf("Time (ms): \t%f\n", timer->time[i] / (1000)); 
}

typedef struct {
    struct dpu_set_t dpu_set;
    uint32_t dimension;
    uint32_t* axis_len;
} hypercube_manager;

__API_SYMBOL__
hypercube_manager* init_hypercube_manager(struct dpu_set_t dpu_set, uint32_t dimension, uint32_t* axis_len){
    hypercube_manager* manager = malloc(sizeof(hypercube_manager));

    manager->dpu_set = dpu_set;
    manager->dimension = dimension;
    manager->axis_len = axis_len;

    return manager;
}

//Supported Communication Primitives
__API_SYMBOL__
void pidcomm_broadcast(hypercube_manager* manager, uint32_t total_data_size, uint32_t target_offset, void* data){
    struct dpu_set_t dpu_set = manager -> dpu_set;
    DPU_ASSERT(dpu_broadcast_to(dpu_set, DPU_MRAM_HEAP_POINTER_NAME, target_offset, data, total_data_size, DPU_XFER_DEFAULT));
}

__API_SYMBOL__
void pidcomm_alltoall(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }


    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //relocate before kernel
    if(!comm_type){

        //DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));     
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    all_to_all(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //relocate after kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        //DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_ALLTOALL_22"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else if(!comm_type  || (axis_len[0]<8 && comm_axis[1]==1) || (axis_len[0]*axis_len[1]==4 && (comm_axis[1] == 1 || comm_axis[2] == 1)) ){
        //DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_ALLTOALL_X_2"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        //DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset+ buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

__API_SYMBOL__
void pidcomm_reduce_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t size){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    if(axis_len[0]==2 && axis_len[1]==2 && ( ((comm_axis[0]==0) && (comm_axis[1]==1) && (comm_axis[2]==0)) || ((comm_axis[0]==1) && (comm_axis[1]==0) && (comm_axis[2]==1)))){
        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_RS_22_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_RS_22_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = 2;
            dpu_argument[i].num_comm_rg = 2;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    //relocate before kernel
    else if(!comm_type){

        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset + buffer_offset, 8, DPU_XFER_DEFAULT));

    reduce_scatter(&dpu_set, start_offset, target_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}

__API_SYMBOL__
void pidcomm_all_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
                    uint32_t buffer_offset, uint32_t size, uint32_t reduce_type){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //relocate before kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ( ((comm_axis[0]==0) && (comm_axis[1]==1) && (comm_axis[2]==0)) || ((comm_axis[0]==1) && (comm_axis[1]==0) && (comm_axis[2]==1)))){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_REVERSE_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = 2;
            dpu_argument[i].num_comm_rg = 2;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_RS_24_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_RS_24_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_SHORT_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT32, NULL));
        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU1_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU1_INT32"), NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //kernel function of All-Reduce
    all_reduce(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, reduce_type);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //relocate before kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE_SHORT_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset+buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_COUNTERCLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_AR_2_Y_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_AR_2_Y_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_INCREMENTAL_COUNTERCLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));
}

__API_SYMBOL__
void pidcomm_allgather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }


    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }
    //relocate before kernel

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    all_gather(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    //relocate after kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        // DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_ALLTOALL_22"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_MODIFIED_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else if(!comm_type  || (axis_len[0]<8 && comm_axis[1]==1) || (axis_len[0]*axis_len[1]==4 && (comm_axis[1] == 1 || comm_axis[2] == 1))){
        // DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_ALLTOALL_X_2"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_REVERSE_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        // DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2"), NULL));
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));
}

__API_SYMBOL__
void pidcomm_gather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, 
                                                        uint32_t buffer_offset, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    gather(&dpu_set, start_offset, start_offset, total_data_size, 0, buffer_offset, dimension, axis_len, comm_axis, host_buffer);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

__API_SYMBOL__
void pidcomm_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, \
                    uint32_t buffer_offset, uint32_t size, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //relocate before kernel
    if(!comm_type){

        // if(size==1) DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT8"), NULL));
        // else DPU_ASSERT(dpu_load(dpu_set, getenv("DPU_BINARY_RELOCATE_2_INT32"), NULL));
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_CLOCKWISE_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].a_length = axis_len[0];
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //kernel function of All-Reduce
    reduce(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, total_data_size, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, host_buffer);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}

//total data size is size of data each dpu will receive
__API_SYMBOL__
void pidcomm_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, \
                    uint32_t buffer_offset, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //kernel function of All-Reduce
    scatter(&dpu_set, start_offset, start_offset, total_data_size, comm_type, buffer_offset, dimension, axis_len, comm_axis, host_buffer);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));
}