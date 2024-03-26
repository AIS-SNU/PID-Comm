/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_FPGA_ILA_H
#define DPU_FPGA_ILA_H

#include "hw_dpu_sysfs.h"

bool
trace_command(struct dpu_rank_fs *rank_fs, bool activate);
bool
reset_ila(struct dpu_rank_fs *rank_fs);
bool
activate_filter_ila(struct dpu_rank_fs *rank_fs);
bool
deactivate_filter_ila(struct dpu_rank_fs *rank_fs);
bool
activate_ila(struct dpu_rank_fs *rank_fs);
bool
deactivate_ila(struct dpu_rank_fs *rank_fs);
bool
set_mram_bypass_to(struct dpu_rank_fs *rank_fs, bool mram_bypass_is_enabled);
bool
disable_refresh_emulation(struct dpu_rank_fs *rank_fs);
bool
enable_refresh_emulation(struct dpu_rank_fs *rank_fs, uint32_t threshold);
bool
dump_ila_report(struct dpu_rank_fs *rank_fs, const char *report_file_name);
bool
inject_faults(struct dpu_rank_fs *rank_fs);

#endif // DPU_FPGA_ILA_H
