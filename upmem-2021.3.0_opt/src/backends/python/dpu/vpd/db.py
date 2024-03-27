# Copyright 2021 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ctypes
from dpu.vpd.vpd import *
from typing import Optional

"""VPD Database Management"""


class DPUVpdError(Exception):
    """Error raised by the API"""
    pass


class DPUVpdDatabase():
    """
    Representation of a VPD Database

    Args:
        db (struct_dpu_vpd_database): internal representation of the database

    Raises:
        DPUVpdError: if an error occurs
    """

    def __init__(self, db_path: Optional[str] = None):
        """
        Args:
            db_path (Optional[str]): the path of the file used to initialize the database

        Raises:
            DPUVpdError: if the database cannot be initialized
        """
        self.db = struct_dpu_vpd_database()

        if dpu_vpd_db_init(-1, db_path, self.db) != DPU_VPD_OK:
            raise DPUVpdError("failed to initialize the database")

    def __del__(self):
        dpu_vpd_db_destroy(self.db)

    def add_string(self,
                   key: str,
                   string: str) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            string (str): string value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        string_c = create_string_buffer(bytes(string, "utf-8"))
        value_c = cast(string_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                value_c,
                len(string),
                VPD_TYPE_STRING) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def add_byte(self,
                 key: str,
                 byte: int) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            byte (int): byte value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        value_c = (c_ubyte * 1)(byte)
        pvalue_c = cast(value_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                pvalue_c,
                1,
                VPD_TYPE_BYTE) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def add_short(
            self,
            key: str,
            short: int) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            short (int): short value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        value_c = (c_ushort * 1)(short)
        pvalue_c = cast(value_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                pvalue_c,
                2,
                VPD_TYPE_SHORT) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def add_int(
            self,
            key: str,
            integer: int) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            integer (int): integer value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        value_c = (c_uint * 1)(integer)
        pvalue_c = cast(value_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                pvalue_c,
                4,
                VPD_TYPE_INT) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def add_long(
            self,
            key: str,
            long: int) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            long (int): long value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        value_c = (c_ulong * 1)(long)
        pvalue_c = cast(value_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                pvalue_c,
                8,
                VPD_TYPE_LONG) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def add_numeric(
            self,
            key: str,
            val: int) -> None:
        """
        Add the key/value pair to the database. This function chooses the c type that fits best.

        Args:
            key (str): key
            long (int): integer value

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        if val <= 0xFF:
            self.add_byte(key, val)
        elif val <= 0xFFFF:
            self.add_short(key, val)
        elif val <= 0xFFFFFFFF:
            self.add_int(key, val)
        elif val <= 0xFFFFFFFFFFFFFFFF:
            self.add_long(key, val)
        else:
            DPUVpdError("numeric value is too big")

    def add_bytearray(
            self,
            key: str,
            byte_array: bytearray) -> None:
        """
        Add the key/value pair to the database

        Args:
            key (str): key
            byte_array (bytearray): byte array

        Raises:
            DPUVpdError: if the database cannot be updated
        """
        value_c = (c_uint8 * len(byte_array)).from_buffer(byte_array)
        pvalue_c = cast(value_c, POINTER(c_uint8))
        if dpu_vpd_db_update(
                self.db,
                key,
                pvalue_c,
                len(byte_array),
                VPD_TYPE_BYTEARRAY) != DPU_VPD_OK:
            raise DPUVpdError("failed to update the database")

    def write_to_file(
            self,
            file_name: str) -> None:
        """
        Encode the database and dump its content into file

        Args:
            file_name (str): output file name

        Raises:
            DPUVpdError: if the database cannot be dumped into file
        """
        if dpu_vpd_db_write(self.db, file_name) != DPU_VPD_OK:
            raise DPUVpdError("failed to dump the database")

    def write_to_device(
            self,
            rank_name: str) -> None:
        """
        Encode the database and write it to the specified rank

        Args:
            rank_name (str): rank to write

        Raises:
            DPUVpdError: if the database cannot be written to the rank
        """
        if dpu_vpd_db_commit_to_device(self.db, rank_name) != DPU_VPD_OK:
            raise DPUVpdError("failed to commit the database")

    def dump(self) -> None:
        """
        Display the database
        """
        string_pair = self.db.first
        while string_pair:
            string_pair = string_pair.contents
            key_c = cast(string_pair.key, c_char_p)
            value_len = string_pair.value_len
            value_type = string_pair.value_type

            print(key_c.value.decode("utf-8"), end="\n  ")
            if value_type == VPD_TYPE_STRING:
                value_c = cast(string_pair.value, c_char_p)
                print(value_c.value.decode("utf-8"))
            elif value_type == VPD_TYPE_BYTE:
                value_c = cast(string_pair.value, POINTER(c_uint8))
                print(value_c.contents.value)
            elif value_type == VPD_TYPE_SHORT:
                value_c = cast(string_pair.value, POINTER(c_ushort))
                print(value_c.contents.value)
            elif value_type == VPD_TYPE_INT:
                value_c = cast(string_pair.value, POINTER(c_uint))
                print(value_c.contents.value)
            elif value_type == VPD_TYPE_LONG:
                value_c = cast(string_pair.value, POINTER(c_ulong))
                print(value_c.contents.value)
            elif value_type == VPD_TYPE_BYTEARRAY:
                value = string_pair.value
                for i in range(value_len):
                    if (i % 16) == 0 and i != 0:
                        print("\n  ", end='')
                    print("%02x  " % value[i], end='')
                print("")

            string_pair = string_pair.next
