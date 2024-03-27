
/* Copyright 2021 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_API_LOAD_H
#define DPU_API_LOAD_H

#include <dpu_types.h>
#include <dpu_error.h>

dpu_error_t
dpu_load_rank(struct dpu_rank_t *rank, struct dpu_program_t *program, dpu_elf_file_t elf_info);

#endif
