/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <inttypes.h>

#include <verbose_control.h>

#include <dpu_types.h>
#include <dpu_attributes.h>
#include <dpu_instruction_encoder.h>
#include <dpu_description.h>
#include <dpu_management.h>
#include <dpu_elf.h>

#include <dpu_rank.h>
#include <dpu_api_log.h>
#include <dpu_memory.h>
#include <dpu_characteristics.h>
#include <profiling_internals.h>

#define MAGIC_PROFILING_VALUE 0xDEAD0000
#define MAGIC_PROFILING_VALUE_MASK 0xFFFF0000

__API_SYMBOL__ dpu_profiling_context_t
dpu_get_profiling_context(struct dpu_rank_t *rank)
{
    return &rank->profiling_context;
}

__API_SYMBOL__ dpu_error_t
dpu_fill_profiling_info(struct dpu_rank_t *rank,
    iram_addr_t mcount_address,
    iram_addr_t ret_mcount_address,
    wram_addr_t thread_profiling_address,
    wram_addr_t perfcounter_end_value_address,
    dpu_elf_symbols_t *profiling_symbols)
{
    dpu_profiling_context_t profiling_context = dpu_get_profiling_context(rank);

    if (profiling_context->thread_profiling_address == 0)
        profiling_context->thread_profiling_address = thread_profiling_address;
    if (profiling_context->mcount_address == 0)
        profiling_context->mcount_address = mcount_address;
    if (profiling_context->ret_mcount_address == 0)
        profiling_context->ret_mcount_address = ret_mcount_address;
    if (profiling_context->perfcounter_end_value_address == 0)
        profiling_context->perfcounter_end_value_address = perfcounter_end_value_address;
    if (profiling_context->profiling_symbols == NULL)
        profiling_context->profiling_symbols = profiling_symbols;

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_patch_profiling_for_dpu(struct dpu_t *dpu, dpuinstruction_t *source, uint32_t address, uint32_t size, bool init)
{
#define INSN_SH_ID4(addr, imm) SHrii(REG_ID4, addr, imm)
#define INSN_CALL_R23_MASK CALLri(REG_R23, 0) // Mask any immediate value
#define INSN_CALL_R23_RXX_MASK CALLrr(REG_R23, 0) // Mask any ra value
#define INSN_CALL_MCOUNT(imm) CALLri(REG_R23, imm)
#define INSN_NOP NOP()
#define INSN_RET JUMPr(REG_R23)
#define INSN_JUMP_RET_MCOUNT(imm) JUMPi(imm)

    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    iram_size_t nb_of_instructions = (iram_size_t)size;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_description_t description = dpu_get_description(rank);
    dpu_profiling_context_t profiling_context = dpu_get_profiling_context(rank);

    /* Patch the binary if necessary
     * Note that mcount presence is intentional: the user has compiled with -pg
     *  - if "mcount", use the mcount implementation
     *  - if "statistics", insert a SW %counter at a certain address
     *  - if "nop", nopify the mcount_address
     *  store the addresses in an array, that will then be used in conjunction with addr2line ADDRESS -f -e a.out
     *  to findout the functions.
     */
    if (dpu != profiling_context->dpu)
        return DPU_OK;

    if ((profiling_context->enable_profiling == DPU_PROFILING_MCOUNT)
        || (profiling_context->enable_profiling == DPU_PROFILING_SAMPLES)
        || (profiling_context->enable_profiling == DPU_PROFILING_SECTIONS))
        return DPU_OK;

    if (!profiling_context->mcount_address || !profiling_context->ret_mcount_address
        || !profiling_context->thread_profiling_address) {
        if (profiling_context->enable_profiling == DPU_PROFILING_STATS) {
            LOG_DPU(WARNING,
                dpu,
                "Profiling enabled but necessary infos (mcount, ret_mcount or thread_profiling symbol addresses) "
                "are not provided: Please provide them through profile properties mcountAddress, retMcountAddress and "
                "threadProfilingAddress "
                "if not automatically extracted from ELF.");
            return DPU_ERR_SYSTEM;
        } else if (profiling_context->enable_profiling == DPU_PROFILING_NOP) {
            /* Here that means that the user has not activated '-pg' compilation flag and
             * then there's no patching to do.
             */
            return DPU_OK;
        }
    }

    uint8_t nr_threads = description->hw.dpu.nr_of_threads;
    iram_addr_t insn;
    uint32_t mcount_idx, th_id;

    if (init) {
        /* Take care that someone might reload code, so
         * free the array if already allocated
         */
        if (profiling_context->mcount_stats) {
            LOG_DPU(VERBOSE, dpu, "Reloading code, free current stats");

            for (th_id = 0; th_id < nr_threads; ++th_id) {
                if (profiling_context->mcount_stats[th_id] != NULL) {
                    free(profiling_context->mcount_stats[th_id]);
                    profiling_context->mcount_stats[th_id] = NULL;
                }
            }

            profiling_context->nr_of_mcount_stats = 0;
        } else {
            profiling_context->mcount_stats = calloc(nr_threads, sizeof(*profiling_context->mcount_stats));
            if (!profiling_context->mcount_stats) {
                return DPU_ERR_SYSTEM;
            }
        }
    }

    uint16_t initial_nr_of_mcount_stats = profiling_context->nr_of_mcount_stats;

    /* Because there could be multiple call sites for one function (ret_mcount could
     * be then called with multiple values of r23 for one function), to allocate enough
     * space for mcount_stats, we must count the number of "call r23, @".
     */
    for (insn = 0; insn < nb_of_instructions; ++insn) {
        /* Catch all "call r23, XXX" with XXX being an immediate or XXX being a register */
        if ((source[insn] & INSN_CALL_R23_MASK) == INSN_CALL_R23_MASK
            || (source[insn] & INSN_CALL_R23_RXX_MASK) == INSN_CALL_R23_RXX_MASK)
            profiling_context->nr_of_mcount_stats++;
    }

    /* It may be overprotective, but it avoids mcount_stats to be a valid pointer of 0 element. */
    if (profiling_context->nr_of_mcount_stats == 0) {
        LOG_DPU(DEBUG, dpu, "No instruction were found to patch");
        return DPU_OK;
    }

    LOG_DPU(VERBOSE, dpu, "%d instructions to patch", profiling_context->nr_of_mcount_stats - initial_nr_of_mcount_stats);

    uint32_t mcount_address, ret_mcount_address, thread_profiling_address;

    mcount_address = profiling_context->mcount_address;
    ret_mcount_address = profiling_context->ret_mcount_address;
    thread_profiling_address = profiling_context->thread_profiling_address;

    uint64_t mcount_encoding = INSN_CALL_MCOUNT(mcount_address);

    if (profiling_context->nr_of_mcount_stats != initial_nr_of_mcount_stats) {
        for (th_id = 0; th_id < nr_threads; ++th_id) {
            profiling_context->mcount_stats[th_id] = realloc(profiling_context->mcount_stats[th_id],
                profiling_context->nr_of_mcount_stats * sizeof(*(profiling_context->mcount_stats[th_id])));
            if (!profiling_context->mcount_stats[th_id]) {
                for (th_id = 0; th_id < nr_threads; ++th_id) {
                    if (profiling_context->mcount_stats[th_id] != NULL) {
                        free(profiling_context->mcount_stats[th_id]);
                    }
                }

                free(profiling_context->mcount_stats);
                profiling_context->mcount_stats = NULL;

                return DPU_ERR_SYSTEM;
            }
        }
    }

    mcount_idx = initial_nr_of_mcount_stats;

    for (insn = 0; insn < nb_of_instructions; ++insn) {
        iram_addr_t insn_address = (iram_addr_t)(address + insn);
        if (source[insn] == mcount_encoding) {
            if (profiling_context->enable_profiling == DPU_PROFILING_NOP) {
                source[insn] = INSN_NOP;
            } else if (profiling_context->enable_profiling == DPU_PROFILING_STATS) {
                source[insn] = INSN_SH_ID4(thread_profiling_address, insn_address);
                for (th_id = 0; th_id < nr_threads; ++th_id) {
                    profiling_context->mcount_stats[th_id][mcount_idx].address = insn_address;
                    profiling_context->mcount_stats[th_id][mcount_idx].count = 0;
                }
                mcount_idx++;
            }
        } else if (source[insn] == INSN_RET && insn_address != ret_mcount_address + 1) {
            if (profiling_context->enable_profiling == DPU_PROFILING_STATS)
                source[insn] = INSN_JUMP_RET_MCOUNT(ret_mcount_address);
        } else if ((source[insn] & INSN_CALL_R23_MASK) == INSN_CALL_R23_MASK
            || (source[insn] & INSN_CALL_R23_RXX_MASK) == INSN_CALL_R23_RXX_MASK) {
            for (th_id = 0; th_id < nr_threads; ++th_id) {
                profiling_context->mcount_stats[th_id][mcount_idx].address = (iram_addr_t)(insn_address + 1);
                profiling_context->mcount_stats[th_id][mcount_idx].count = 0;
            }
            mcount_idx++;
        }
    }

    return DPU_OK;
}

__API_SYMBOL__ void
dpu_collect_statistics_profiling(struct dpu_t *dpu, uint8_t nr_threads, const uint32_t *profiled_address)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    uint32_t thread_id, insn;
    uint16_t cur_profiled_address;

    for (thread_id = 0; thread_id < nr_threads; ++thread_id) {
        bool foundAddress = false;

        if ((profiled_address[thread_id] & MAGIC_PROFILING_VALUE_MASK) != MAGIC_PROFILING_VALUE) {
            LOG_DPU(DEBUG, dpu, "Read profiling magic in wram failed, skipping stats");
            continue;
        }

        cur_profiled_address = (uint16_t)(profiled_address[thread_id] & ~MAGIC_PROFILING_VALUE_MASK);
        if (cur_profiled_address == 0x0)
            continue;

        for (insn = 0; insn < rank->profiling_context.nr_of_mcount_stats; ++insn) {
            if (rank->profiling_context.mcount_stats[thread_id][insn].address == cur_profiled_address) {
                rank->profiling_context.mcount_stats[thread_id][insn].count++;
                foundAddress = true;
                break;
            }
        }

        if (!foundAddress)
            LOG_DPU(WARNING, dpu, "Profiling address (thread: %u @0x%x) retrieved is unknown", thread_id, cur_profiled_address);
    }
}

__API_SYMBOL__ void
dpu_collect_samples_profiling(struct dpu_t *dpu, iram_addr_t profiled_address)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return;
    }

    if (profiled_address >= dpu->rank->description->hw.memories.iram_size) {
        LOG_DPU(WARNING, dpu, "Profiling address (@0x%x) retrieved is invalid", profiled_address);
        return;
    }

    dpu->rank->profiling_context.sample_stats[profiled_address]++;
}

__API_SYMBOL__ void
dpu_dump_statistics_profiling(struct dpu_t *dpu, uint8_t nr_threads)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    uint32_t thread_id, insn;

    for (thread_id = 0; thread_id < nr_threads; ++thread_id) {
        for (insn = 0; insn < rank->profiling_context.nr_of_mcount_stats; ++insn) {
            LOG_DPU(INFO,
                dpu,
                "profiling_result: %u: 0x%x: %" PRIu64,
                thread_id,
                rank->profiling_context.mcount_stats[thread_id][insn].address,
                rank->profiling_context.mcount_stats[thread_id][insn].count);
        }
    }
}

__API_SYMBOL__ void
dpu_dump_samples_profiling(struct dpu_t *dpu)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return;
    }

    for (uint32_t each_instruction = 0; each_instruction < dpu->rank->description->hw.memories.iram_size; ++each_instruction) {
        uint64_t count = dpu->rank->profiling_context.sample_stats[each_instruction];

        if (count != 0) {
            LOG_DPU(INFO, dpu, "profiling_result: total: 0x%x: %" PRIu64, each_instruction, count);
        }
    }
}

__API_SYMBOL__ dpu_error_t
dpu_set_magic_profiling_for_dpu(struct dpu_t *dpu)
{
    if (!dpu->enabled) {
        return DPU_ERR_DPU_DISABLED;
    }

    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    uint8_t nr_threads = rank->description->hw.dpu.nr_of_threads;
    uint32_t magic_value[nr_threads];

    if (rank->profiling_context.enable_profiling != DPU_PROFILING_STATS)
        return DPU_OK;

    for (int i = 0; i < nr_threads; ++i)
        magic_value[i] = MAGIC_PROFILING_VALUE;

    return dpu_copy_to_wram_for_dpu(
        dpu, (wram_addr_t)(rank->profiling_context.thread_profiling_address / 4), (const dpuword_t *)&magic_value, nr_threads);
}

__API_SYMBOL__ void
dpu_dump_section_profiling(struct dpu_t *dpu,
    uint8_t nr_threads,
    const char *section_name,
    const dpuword_t *section_info,
    uint32_t perfcounter_end_value)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return;
    }

    if (perfcounter_end_value == 0) {
        LOG_DPU(DEBUG, dpu, "perfcounter end value is null, cannot compute the ratio of the time spent in the section");
        return;
    }

    const dpu_profiling_t *section = (const dpu_profiling_t *)section_info;
    for (dpu_thread_t each_thread = 0; each_thread < nr_threads; ++each_thread) {
        uint32_t duration = section->count[each_thread];
        double ratio = (double)duration / perfcounter_end_value;
        if (ratio != 0) {
            LOG_DPU(INFO, dpu, "profiling_section: %s: thread#%02d: %f", section_name, each_thread, ratio);
        }
    }
}
