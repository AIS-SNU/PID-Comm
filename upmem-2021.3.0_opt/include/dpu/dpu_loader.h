/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_LOADER_H
#define DPU_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_elf.h>

/**
 * @file dpu_loader.h
 * @brief C API for ELF program loading on DPUs.
 */

/**
 * @brief Target destination for the DPU loader.
 */
typedef enum _dpu_loader_target_t {
    /** Loader will target a DPU rank. */
    DPU_LOADER_TARGET_RANK,
    /** Loader will target a DPU. */
    DPU_LOADER_TARGET_DPU
} dpu_loader_target_t;

/**
 * @brief DPU Loader environment, containing its target.
 */
typedef struct _dpu_loader_env_t {
    /** The target for the loader. */
    dpu_loader_target_t target;
    /** Reference to the target. */
    union {
        /** The DPU rank reference, when the target is a DPU rank. */
        struct dpu_rank_t *rank;
        /** The DPU reference, when the target is a DPU. */
        struct dpu_t *dpu;
    };
} * dpu_loader_env_t;

/**
 * @brief A function that will be called before loading content into memory to modify the content.
 */
typedef dpu_error_t (
    *mem_patch_function_t)(dpu_loader_env_t env, void *content, dpu_mem_max_addr_t address, dpu_mem_max_size_t size, bool init);

/**
 * @brief DPU Loader context.
 */
typedef struct _dpu_loader_context_t {
    /** IRAM patch function. */
    mem_patch_function_t patch_iram;
    /** WRAM patch function. */
    mem_patch_function_t patch_wram;
    /** MRAM patch function. */
    mem_patch_function_t patch_mram;

    /** Current number of loaded instructions. */
    uint32_t nr_of_instructions;
    /** Current number of loaded WRAM words. */
    uint32_t nr_of_wram_words;
    /** Current number of loaded MRAM bytes. */
    uint32_t nr_of_mram_bytes;
    /** Dummy field used when a reference is needed but no other field should be modified. */
    uint32_t dummy;

    /** The DPU loader environment. */
    struct _dpu_loader_env_t env;
} * dpu_loader_context_t;

/**
 * @brief Set up a DPU Loader context targetting the specified DPU.
 * @param context The context to fill
 * @param dpu The loader target
 */
void
dpu_loader_fill_dpu_context(dpu_loader_context_t context, struct dpu_t *dpu);

/**
 * @brief Set up a DPU Loader context targetting the specified DPU rank.
 * @param context The context to fill
 * @param rank The loader target
 */
void
dpu_loader_fill_rank_context(dpu_loader_context_t context, struct dpu_rank_t *rank);

/**
 * @brief Loads the specified ELF file using the specified DPU loader context
 *        to determine the patch functions and target.
 * @param file the ELF file to be loaded
 * @param context the DPU Loader context
 * @return Whether the operation was successful.
 */
dpu_error_t
dpu_elf_load(dpu_elf_file_t file, dpu_loader_context_t context);

#endif // DPU_LOADER_H
