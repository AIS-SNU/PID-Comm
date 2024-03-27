/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_POLLING_H
#define DPU_POLLING_H

#include <dpu_error.h>
#include <dpu_rank.h>
#include <stdint.h>

dpu_error_t
dpu_sync_rank(struct dpu_rank_t *rank);

dpu_error_t
dpu_sync_dpu(struct dpu_t *dpu);

dpu_error_t
polling_thread_create();

void
polling_thread_set_dpu_running(uint32_t idx);

#endif /* DPU_POLLING_H */
