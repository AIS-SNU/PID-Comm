/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_PROFILER_H
#define DPU_PROFILER_H

#include <dpu_elf.h>
#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_profiler.h
 * @brief C API for DPU profiling operations.
 */

/**
 * @brief The different profiling methods on DPU.
 */
enum dpu_profiling_type_e {
    /** Do not profile and replace profiling calls with no-operations. */
    DPU_PROFILING_NOP,
    DPU_PROFILING_MCOUNT,
    /** Profile by sampling WRAM information on the current function from the host. */
    DPU_PROFILING_STATS,
    /** Profile by sampling the PC from the host. */
    DPU_PROFILING_SAMPLES,
    /** Profile sections of the DPU program with DPU counters. */
    DPU_PROFILING_SECTIONS,
};

/**
 * @brief Information to profile a DPU program.
 * @internal Currently, we can only profile one DPU.
 */
typedef struct _dpu_profiling_context_t { /** The profiling method used. */
    enum dpu_profiling_type_e enable_profiling;
    /** Reference to the DPU to profile. */
    struct dpu_t *dpu;

    /**
     * @brief Sample hit counter for each IRAM address.
     * @internal In sample mode, pcs are randomly sampled without knowing the current thread.
     */
    uint64_t *sample_stats;

    /** Number of IRAM addresses patched to profile using mcount. */
    uint16_t nr_of_mcount_stats;
    /** Sample hit counter for each mcount patched IRAM address. */
    struct { /** IRAM address for this mcount call instance. */
        iram_addr_t address;
        /** Sample hit counter for this mcount call instance. */
        uint64_t count;
    } * *mcount_stats;
    /** IRAM address for the mcount function. */
    iram_addr_t mcount_address;
    /** IRAM address for the ret_mcount function. */
    iram_addr_t ret_mcount_address;
    /** WRAM address for the thread_profiling array. */
    wram_addr_t thread_profiling_address;

    /** Symbols for each cycle counter used in section mode. */
    dpu_elf_symbols_t *profiling_symbols;
    /** WRAM address for the perfcounter_end_value variable. */
    wram_addr_t perfcounter_end_value_address;
} * dpu_profiling_context_t;

/**
 * @brief Fill the DPU rank profiling context with the given information.
 * @param rank the unique identifier of the rank
 * @param mcount_address address for the mcount function
 * @param ret_mcount_address address for the ret_mcount function
 * @param thread_profiling_address address for the thread_profiling array
 * @param perfcounter_end_value_address address for the perfcounter_end_value variable
 * @param profiling_symbols the symbols of the cycle counter in section mode
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_fill_profiling_info(struct dpu_rank_t *rank,
    iram_addr_t mcount_address,
    iram_addr_t ret_mcount_address,
    wram_addr_t thread_profiling_address,
    wram_addr_t perfcounter_end_value_address,
    dpu_elf_symbols_t *profiling_symbols);

/**
 * @brief Patch the DPU program IRAM to enable profiling.
 * @param dpu the DPU
 * @param source the IRAM content buffer
 * @param address the start address for the IRAM content
 * @param size the size of the IRAM content, in instructions
 * @param init whether this is the first time this function called for this program
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_patch_profiling_for_dpu(struct dpu_t *dpu, dpuinstruction_t *source, uint32_t address, uint32_t size, bool init);

/**
 * @brief Fetch the information from the thread_profiling array
 * @param dpu the DPU
 * @param nr_threads the number of elements in the thread_profiling array
 * @param profiled_address storage for the thread_profiling array data
 */
void
dpu_collect_statistics_profiling(struct dpu_t *dpu, uint8_t nr_threads, const uint32_t *profiled_address);

/**
 * @brief Update the sampling information with the given information.
 * @param dpu the DPU
 * @param profiled_address the new sampling result to collect
 */
void
dpu_collect_samples_profiling(struct dpu_t *dpu, iram_addr_t profiled_address);

/**
 * @brief Dump the collected profiling information for the stats method.
 * @param dpu the DPU
 * @param nr_threads the number of elements in the thread_profiling array
 */
void
dpu_dump_statistics_profiling(struct dpu_t *dpu, uint8_t nr_threads);

/**
 * @brief Dump the collected profiling information for the sample method.
 * @param dpu the DPU
 */
void
dpu_dump_samples_profiling(struct dpu_t *dpu);

/**
 * @brief Set a "magic" number in the WRAM profiling information during init.
 * @param dpu the DPU
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_set_magic_profiling_for_dpu(struct dpu_t *dpu);

/**
 * @brief Dump the collected profiling information for the section method.
 * @param dpu the DPU
 * @param nr_threads the number of threads on the DPU
 * @param section_name the name of the profiling counter section
 * @param section_info the raw content in WRAM for the profiling section
 * @param perfcounter_end_value the perfcounter value at the end of the DPU execution
 */
void
dpu_dump_section_profiling(struct dpu_t *dpu,
    uint8_t nr_threads,
    const char *section_name,
    const dpuword_t *section_info,
    uint32_t perfcounter_end_value);

/**
 * @brief Fetches the profiling context of the specified rank.
 * @param rank the unique identifier of the rank
 * @return The pointer on the profiling context of the rank.
 */
dpu_profiling_context_t
dpu_get_profiling_context(struct dpu_rank_t *rank);

#endif // DPU_PROFILER_H
