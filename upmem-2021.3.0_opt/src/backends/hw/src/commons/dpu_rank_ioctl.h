/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef DPU_RANK_IOCTL_INCLUDE_H
#define DPU_RANK_IOCTL_INCLUDE_H

#define DPU_RANK_IOCTL_MAGIC 'd'

#define DPU_RANK_IOCTL_WRITE_TO_RANK _IOW(DPU_RANK_IOCTL_MAGIC, 0, struct dpu_transfer_matrix *)
#define DPU_RANK_IOCTL_READ_FROM_RANK _IOW(DPU_RANK_IOCTL_MAGIC, 1, struct dpu_transfer_matrix *)
#define DPU_RANK_IOCTL_COMMIT_COMMANDS _IOW(DPU_RANK_IOCTL_MAGIC, 2, uint64_t *)
#define DPU_RANK_IOCTL_UPDATE_COMMANDS _IOR(DPU_RANK_IOCTL_MAGIC, 3, uint64_t *)
#define DPU_RANK_IOCTL_DEBUG_MODE _IOW(DPU_RANK_IOCTL_MAGIC, 4, uint8_t *)

#endif /* DPU_RANK_IOCTL_INCLUDE_H */
