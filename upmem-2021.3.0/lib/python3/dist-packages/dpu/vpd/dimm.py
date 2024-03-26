# Copyright 2021 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
from dpu.vpd.vpd import *
import ctypes

"""VPD Management"""

SYSFS_PATH = "/sys/class/dpu_rank/dpu_rank"


class DPUVpdError(Exception):
    """Error raised by the API"""
    pass


class DPUVpd():
    """
    VPD information

    Args:
        rank_path (str):  the path of rank the VPD information is related to
        rank_index(int): the index of the rank in the DIMM (0|1)
        vpd (struct_dpu_vpd_header): internal representation of the vpd information
        vpd_modified (bool): indicates if the VPD information was modified

    Raises:
        DPUVpdError: if an error occurs
    """

    def __init__(self, rank_path: str):
        """
        Args:
            rank_path (str): the path of rank the VPD information is related to

        Raises:
            DPUVpdError: if the VPD path cannot be determined or if the VPD information cannot be read
        """
        self.rank_path = rank_path
        self.vpd = struct_dpu_vpd()
        self.vpd_modified = False

        vpd_path = create_string_buffer(512)
        if dpu_vpd_get_vpd_path(self.rank_path,
                                vpd_path) != DPU_VPD_OK:
            raise DPUVpdError(
                "failed to get vpd path from {}".format(
                    self.rank_path))

        if dpu_vpd_init(vpd_path, self.vpd) != DPU_VPD_OK:
            raise DPUVpdError(
                "failed to get vpd content from {}".format(vpd_path))

        rank_id = re.findall(r'\d+', self.rank_path)[0]
        with open("{}{}/rank_index".format(SYSFS_PATH, rank_id), "r") as rank_index:
            self.rank_index = int(rank_index.read())

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.vpd_modified:
            if dpu_vpd_commit_to_device(
                    self.vpd, self.rank_path) != DPU_VPD_OK:
                raise DPUVpdError(
                    "failed to commit vpd to {}".format(
                        self.rank_path))

    def __str__(self):
        ranks = ""
        for i in range(self.vpd.vpd_header.rank_count):
            r = self.vpd.vpd_header.ranks[i]
            ranks += "\trank#%d disabled %016x repair WRAM %016x IRAM %016x\n" % (
                i, r.dpu_disabled, r.wram_repair, r.iram_repair)

        repair = ""
        if self.vpd.vpd_header.repair_count != VPD_UNDEFINED_REPAIR_COUNT:
            for i in range(self.vpd.vpd_header.repair_count):
                r = self.vpd.repair_entries[i]
                repair += "\tREPAIR %sRAM DPU %d.%d.%d bank%d@%04x bits %016x\n" % (
                    'W' if r.iram_wram else 'I', r.rank, r.ci, r.dpu, r.bank, r.address, r.bits)

        return ranks + repair

    def disable_dpu(self, slice_id: int, dpu_id: int) -> None:
        """
        Disable a DPU

        Args:
            slice_id (int): slice ID of the DPU to disable
            dpu_id (int): ID of the DPU to disable
        """
        if dpu_vpd_disable_dpu(
                self.vpd,
                self.rank_index,
                slice_id,
                dpu_id) == DPU_VPD_ERR_DPU_ALREADY_DISABLED:
            print("DPU is already disabled")
        else:
            self.vpd_modified = True

    def enable_dpu(self, slice_id: int, dpu_id: int) -> None:
        """
        Enable a DPU

        Args:
            slice_id (int): slice ID of the DPU to enable
            dpu_id (int): ID of the DPU to enable
        """
        if dpu_vpd_enable_dpu(
                self.vpd,
                self.rank_index,
                slice_id,
                dpu_id) == DPU_VPD_ERR_DPU_ALREADY_ENABLED:
            print("DPU is already enabled")
        else:
            self.vpd_modified = True
