#!/usr/bin/env python3

import ctypes
from dpu.vpd import vpd
from dpu.vpd.db import *
from dpu.vpd.dimm import *
import argparse
import re
import glob
import subprocess
import sys
import os
import serial
import time

error_mode = 0


def subprocess_verbose(args, command_list, timeout=10):
    proc = subprocess.run(
        command_list,
        timeout=timeout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)

    if args.verbose:
        print(proc.stdout.decode())

    return proc


def vpd_mode(args, rank_path_list):
    global error_mode

    # Make sure the options are correct.
    # disable/enable_dpu on all ranks is at least suspect, so ask for a rank
    # to be given.
    if (args.disable_dpu or args.enable_dpu) and not args.rank:
        print("Error, you must specify at least a rank with --rank option.")
        sys.exit(-1)

    for rank_path in rank_path_list:
        with DPUVpd(rank_path) as dimm_vpd:
            if args.info:
                print("* VPD for {}".format(rank_path))
                print(dimm_vpd)
            elif args.disable_dpu:
                for dpu in args.disable_dpu:
                    slice_id = int(dpu.split(".")[0])
                    dpu_id = int(dpu.split(".")[1])
                    dimm_vpd.disable_dpu(slice_id, dpu_id)
            elif args.enable_dpu:
                for dpu in args.enable_dpu:
                    slice_id = int(dpu.split(".")[0])
                    dpu_id = int(dpu.split(".")[1])
                    dimm_vpd.enable_dpu(slice_id, dpu_id)


def flash_mcu_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print(
            "* Flashing MCU of rank {}...".format(rank_path),
            end='',
            flush=True)

        command = "ectool --interface=ci --name={} sysjump 1".format(rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to jump to RO FW.")
            error_mode = 1
            continue

        command = "ectool --interface=ci --name={} flasherase {} {}".format(
            rank_path, vpd.FLASH_OFF_RW, vpd.FLASH_SIZE_RW)
        proc = subprocess_verbose(args, command.split(" "), timeout=20)
        if proc.returncode != 0:
            print("Failed to erase RW partition.")
            error_mode = 1
            continue

        command = "ectool --interface=ci --name={} flashwrite {} {}".format(
            rank_path, vpd.FLASH_OFF_RW, args.flash_mcu)
        proc = subprocess_verbose(args, command.split(" "), timeout=20)
        if proc.returncode != 0:
            print("Failed to write RW partition.")
            error_mode = 1
            continue

        command = "ectool --interface=ci --name={} reboot_ec cold".format(
            rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to reboot MCU.")
            error_mode = 1
            continue

        print("Success.")


def flash_mcu_dfu_mode(args, rank_path_list):
    global error_mode

    # 1. Look for all UPMEM DIMMs connected with USB and switch them to DFU mode,
    # if the device is already in DFU mode, nothing happens.
    for tty_dev in glob.iglob("/sys/bus/usb-serial/drivers/google/tty*"):
        tty_name = os.path.basename(tty_dev)
        tty_ser = serial.Serial("/dev/{}".format(tty_name), 115200)
        tty_ser.write(b'dfu\n')
        tty_ser.close()

    # Wait for devices to reboot in DFU mode
    time.sleep(5)

    # 2. Go through all DFU devices and flash them using dfu-util.
    for dfu_dev in glob.iglob("/sys/bus/usb/devices/*"):
        try:
            with open("{}/idVendor".format(dfu_dev), "r") as f:
                id_vendor = f.read().rstrip()
        except FileNotFoundError:
            continue

        try:
            with open("{}/idProduct".format(dfu_dev), "r") as f:
                id_product = f.read().rstrip()
        except FileNotFoundError:
            continue

        if id_vendor != "0483" or id_product != "df11":
            continue

        print("* Flashing USB device {}...".format(os.path.basename(dfu_dev)),
              end='', flush=True)

        # This suite of commands is taken from ec/util/flash_ec.
        # The current device is in DFU mode, just use dfu-util to flash it now.
        command = "dfu-util -a 0 -s {}:{} -D {} -p {}".format(
            vpd.FLASH_BASE_ADDRESS +
            vpd.FLASH_OFF_RO,
            vpd.FLASH_SIZE_RO_RW,
            args.flash_mcu_dfu,
            os.path.basename(dfu_dev))
        proc = subprocess_verbose(args, command.split(" "), timeout=30)
        if proc.returncode != 0:
            print("Failed to flash device {}".format(dfu_dev))

        command = "dfu-util -a 0 -s {}:{}:force:unprotect -D {} -p {}".format(
            vpd.FLASH_BASE_ADDRESS +
            vpd.FLASH_OFF_RO,
            vpd.FLASH_SIZE_RO_RW,
            args.flash_mcu_dfu,
            os.path.basename(dfu_dev))
        proc = subprocess_verbose(args, command.split(" "), timeout=30)
        if proc.returncode != 0:
            print("Failed to reboot device {}".format(dfu_dev))

        print("Success")


def flash_vpd_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print(
            "* Flashing VPD of rank {}...".format(rank_path),
            end='',
            flush=True)

        if vpd.dpu_vpd_commit_to_device_from_file(args.flash_vpd, rank_path):
            print("Failed to write VPD.")
            sys.error_mode = 1
            continue

        print("Success.")


def flash_vpd_db_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print(
            "* Flashing VPD database of rank {}...".format(rank_path),
            end='',
            flush=True)

        if vpd.dpu_vpd_db_commit_to_device_from_file(
                args.flash_vpd_db, rank_path):
            print("Failed to write VPD database.")
            sys.error_mode = 1
            continue

        print("Success.")


def vdd_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print(
            "* Voltage and intensity of rank {}: ".format(rank_path),
            end='',
            flush=True)

        command = "ectool --interface=ci --name={} vdd".format(rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to get VDD.")
            error_mode = 1
            continue

        print("{}".format(proc.stdout.decode().rstrip()))


def reboot_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print("* Rebooting rank {}...".format(rank_path), end='', flush=True)

        command = "ectool --interface=ci --name={} reboot_ec RW".format(
            rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to reboot.")
            error_mode = 1
            continue

        print("Success")


def mcu_version_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print("* MCU firmware of rank {}: ".format(rank_path), end='', flush=True)

        command = "ectool --interface=ci --name={} version".format(rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to print MCU firmware versions.")
            error_mode = 1
            continue

        ro_version, rw_version, fw_copy = proc.stdout.decode().split('\n')[0:3]
        print("")
        print("\t{}".format(ro_version))
        print("\t{}".format(rw_version))
        print("\t{}".format(fw_copy), end="\n\n")


def osc_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print("* Frequency of rank {}: ".format(rank_path), end='', flush=True)

        command = "ectool --interface=ci --name={} osc".format(rank_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to print FCK frequency.")
            error_mode = 1
            continue

        fck, div = proc.stdout.decode().split('\n')[0:2]
        print("")
        print("\t{}".format(fck))
        print("\t{}".format(div), end="\n\n")


def database_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print("* VPD database for {}: ".format(rank_path), end='', flush=True)

        db_path = "db.bin"

        # Read flash segment info file
        command = "ectool --interface=ci --name={} flashread {} {} {}".format(
            rank_path, vpd.FLASH_OFF_VPD_DB, vpd.FLASH_SIZE_VPD_DB, db_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to read VPD database flash segment.")
            error_mode = 1
            continue

        # Init database based on flash content
        db = DPUVpdDatabase(db_path)

        print("")
        db.dump()
        print("")
        os.remove(db_path)


def update_db_mode(args, rank_path_list):
    global error_mode

    for rank_path in rank_path_list:
        print(
            "* Updating VPD database for {}: ".format(rank_path),
            end='',
            flush=True)

        # Read flash segment into file
        db_path = "db.bin"
        command = "ectool --interface=ci --name={} flashread {} {} {}".format(
            rank_path, vpd.FLASH_OFF_VPD_DB, vpd.FLASH_SIZE_VPD_DB, db_path)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to read VPD database flash segment.")
            error_mode = 1
            continue

        # Init database based on flash content
        db = DPUVpdDatabase(db_path)

        # Add or update key/value pairs
        # For key/value pairs used by the MCU FW, use explicit value length
        # (otherwise the MCU might read past the value)
        d = dict(pair.split('=') for pair in args.update_db)
        for key, val in d.items():
            if key in ('div_min', 'div_max'):
                db.add_byte(key, int(val))
            elif key in ('fck', 'fck_min', 'fck_max', 'vdd_limit', 'ltc7106_rfb1', 'ltc7106_rfb2', 'ltc7106_vref', 'ltc7106_vdddpu'):
                db.add_short(key, int(val))
            elif key in ('chip_version'):
                db.add_string(key, val)
            elif val.isnumeric():
                db.add_numeric(key, int(val))
            else:
                db.add_string(key, val)

        # Flash new database
        db.write_to_device(rank_path)
        os.remove(db_path)

        print("Success.")


def set_vdd_mode(args, rank_path_list):
    global error_mode

    vdd_value = int(args.set_vdd)

    if vdd_value < 1000 or vdd_value >= 1600:
        print("VDD value is out of range, must be in [1000: 1600[")
        error_mode = 1
        return

    for rank_path in rank_path_list:
        print("* Setting voltage to {} mV for rank {}: ".format(vdd_value,
              rank_path), end='', flush=True)

        command = "ectool --interface=ci --name={} vdd {}".format(
            rank_path, vdd_value)
        proc = subprocess_verbose(args, command.split(" "))
        if proc.returncode != 0:
            print("Failed to set VDD.")
            error_mode = 1
            continue

        print("Success")


parser = argparse.ArgumentParser(description='Upmem DIMM configuration tool')
parser.add_argument(
    '--rank',
    metavar='<rank_path>',
    default="",
    help="Comma-separated rank path (e.g.: /dev/dpu_rankX), default to all ranks")

parser.add_argument('--verbose',
                    action='store_true',
                    help="Verbose mode: print debugging messages")

parser.add_argument('--info',
                    action='store_true',
                    help="Display info about the DIMM (i.e.: VPD)")
parser.add_argument(
    '--disable-dpu',
    action='append',
    metavar='<x.y>',
    help="Disable dpu y in chip x on rank specified by --rank option, can be set multiple times (e.g.: --disable-dpu 0.1 --disable-dpu 3.4)")
parser.add_argument(
    '--enable-dpu',
    action='append',
    metavar='<x.y>',
    help="Enable dpu y in chip x on rank specified by --rank option, can be set multiple times (e.g.: --enable-dpu 0.1 --enable-dpu 3.4)")

parser.add_argument('--flash-mcu',
                    metavar='<fw_path>',
                    help="Flash MCU with the firmware given in argument")
parser.add_argument(
    '--flash-mcu-dfu',
    metavar='<fw_path>',
    help="Flash *ALL* MCUs connected with USB cable with the firmware given in argument, --rank option is not used and it requires dfu-util.")
parser.add_argument('--flash-vpd',
                    metavar='<vpd_path>',
                    help="Flash VPD with the VPD given in argument")
parser.add_argument(
    '--flash-vpd-db',
    metavar='<db_path>',
    help="Flash VPD database with the database given in argument")

parser.add_argument(
    '--set-vdd',
    metavar='<vdd_mV>',
    help="Set voltage of the chips on the DIMM (cannot be used when the rank is used)")

parser.add_argument(
    '--vdd',
    action='store_true',
    help="Display voltage and intensity of the chips on the DIMM (cannot be used when the rank is used)")
parser.add_argument(
    '--reboot',
    action='store_true',
    help="Reboot the MCU (cannot be used when the rank is used)")
parser.add_argument(
    '--mcu-version',
    action='store_true',
    help="Print MCU firwmare versions (cannot be used when the rank is used)")
parser.add_argument(
    '--osc',
    action='store_true',
    help="Print FCK frequency (cannot be used when the rank is used)")

parser.add_argument(
    '--database',
    action='store_true',
    help="Print DIMM database (cannot be used when the rank is used)")
parser.add_argument(
    '--update-db',
    action='append',
    metavar='<x=y>',
    help="Add or update the key/value pair in the database. Only decimal values and strings are supported (e.g.: --update-db ltc7106_vdddpu=1200)")

args = parser.parse_args()

# Get the ranks to work on.
if args.rank:
    rank_path_list = args.rank.split(",")
else:
    # args.rank being empty means ALL the ranks, so get them.
    rank_path_list = list()
    for dev in glob.iglob("/dev/dpu_rank*"):
        rank_path_list.append(dev)

if args.info or args.disable_dpu or args.enable_dpu:
    vpd_mode(args, rank_path_list)
elif args.flash_mcu:
    flash_mcu_mode(args, rank_path_list)
elif args.flash_mcu_dfu:
    flash_mcu_dfu_mode(args, rank_path_list)
elif args.flash_vpd:
    flash_vpd_mode(args, rank_path_list)
elif args.flash_vpd_db:
    flash_vpd_db_mode(args, rank_path_list)
elif args.vdd:
    vdd_mode(args, rank_path_list)
elif args.reboot:
    reboot_mode(args, rank_path_list)
elif args.mcu_version:
    mcu_version_mode(args, rank_path_list)
elif args.osc:
    osc_mode(args, rank_path_list)
elif args.database:
    database_mode(args, rank_path_list)
elif args.update_db:
    update_db_mode(args, rank_path_list)
elif args.set_vdd:
    set_vdd_mode(args, rank_path_list)
else:
    parser.print_help(sys.stderr)

if error_mode:
    sys.exit(-1)
