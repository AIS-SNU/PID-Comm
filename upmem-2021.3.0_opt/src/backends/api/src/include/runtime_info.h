/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_RUNTIME_INFO_H
#define DPU_RUNTIME_INFO_H

#include <stdint.h>
#include <dpu_elf.h>
/**
 * @fn reset_runtime_info
 *
 * Clears a runtime information structure, setting every value to "not existing" with default values.
 *
 * @param runtime_info the reset structure
 */
void
reset_runtime_info(dpu_elf_runtime_info_t *runtime_info);

/**
 * @fn register_runtime_info_if_needed_with
 * @param symbol the symbol name
 * @param value its value
 * @param size its size
 * @param runtime_info if the provided symbol represents a runtime information, the updated structure
 */
void
register_runtime_info_if_needed_with(const char *symbol, uint32_t value, uint32_t size, dpu_elf_runtime_info_t *runtime_info);

#endif // DPU_RUNTIME_INFO_H
