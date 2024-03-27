#!/usr/bin/env python3
""" Generate VPD DATABASE for UPMEM DIMM """

import argparse
from collections import namedtuple
from dpu.vpd.db import *

DB_FILE = 'db.bin'
P21D_KEYS = 'fck fck_min fck_max div_min div_max vdddpu chip_version'
P21D_DFLT_VALUES = (700, 700, 1000, 2, 3, 1100, 'v1.3')
P21_KEYS = 'fck fck_min fck_max div_min div_max ltc7106_rfb1 ltc7106_rfb2 ltc7106_vref ltc7106_vdddpu vdd_limit chip_version'
P21_DFLT_VALUES = (700, 700, 700, 2, 3, 909, 10000, 1100, 1200, 19000, 'v1.3')
E19_KEYS = 'fck fck_min fck_max div_min div_max vdd_limit chip_version'
E19_DFTL_VALUES = (800, 10, 950, 3, 4, 11700, 'v1.1')

P21dDatabase = namedtuple("P21d_database", P21D_KEYS)
p21d_database = P21dDatabase._make(P21D_DFLT_VALUES)

P21Database = namedtuple("P21_database", P21_KEYS)
p21_database = P21Database._make(P21_DFLT_VALUES)

E19Database = namedtuple("E19_database", E19_KEYS)
e19_database = E19Database._make(E19_DFTL_VALUES)

db = DPUVpdDatabase()

def get_p21d_user_values():
    global p21d_database

    # Dftl values
    fck = getattr(p21d_database, 'fck')
    fck_min = getattr(p21d_database, 'fck_min')
    fck_max = getattr(p21d_database, 'fck_max')
    div_min = getattr(p21d_database, 'div_min')
    div_max = getattr(p21d_database, 'div_max')
    vdddpu = getattr(p21d_database, 'vdddpu')
    chip_version = getattr(p21d_database, 'chip_version')

    kw = dict()

    kw['fck'] = int(
        input(
            "Enter the default frequency [{}]: ".format(
                str(fck))) or fck)
    kw['fck_min'] = int(
        input(
            "Enter the mininum frequency [{}]: ".format(
                str(fck_min))) or fck_min)
    kw['fck_max'] = int(
        input(
            "Enter the maximum frequency [{}]: ".format(
                str(fck_max))) or fck_max)
    kw['div_min'] = int(
        input(
            "Enter the mininum clock div [{}]: ".format(
                str(div_min))) or div_min)
    kw['div_max'] = int(
        input(
            "Enter the maximum clock div [{}]: ".format(
                str(div_max))) or div_max)
    kw['vdddpu'] = int(
        input(
            "Enter Vdd DPU [{}]: ".format(
                str(vdddpu))) or vdddpu)
    kw['chip_version'] = input(
        "Enter the chip version [{}]: ".format(
            str(chip_version))) or chip_version

    p21d_database = p21d_database._replace(**kw)

def get_p21_user_values():
    global p21_database

    # Dftl values
    fck = getattr(p21_database, 'fck')
    fck_min = getattr(p21_database, 'fck_min')
    fck_max = getattr(p21_database, 'fck_max')
    div_min = getattr(p21_database, 'div_min')
    div_max = getattr(p21_database, 'div_max')
    ltc7106_rfb1 = getattr(p21_database, 'ltc7106_rfb1')
    ltc7106_rfb2 = getattr(p21_database, 'ltc7106_rfb2')
    ltc7106_vref = getattr(p21_database, 'ltc7106_vref')
    ltc7106_vdddpu = getattr(p21_database, 'ltc7106_vdddpu')
    vdd_limit = getattr(p21_database, 'vdd_limit')
    chip_version = getattr(p21_database, 'chip_version')

    kw = dict()

    kw['fck'] = int(
        input(
            "Enter the default frequency [{}]: ".format(
                str(fck))) or fck)
    kw['fck_min'] = int(
        input(
            "Enter the mininum frequency [{}]: ".format(
                str(fck_min))) or fck_min)
    kw['fck_max'] = int(
        input(
            "Enter the maximum frequency [{}]: ".format(
                str(fck_max))) or fck_max)
    kw['div_min'] = int(
        input(
            "Enter the mininum clock div [{}]: ".format(
                str(div_min))) or div_min)
    kw['div_max'] = int(
        input(
            "Enter the maximum clock div [{}]: ".format(
                str(div_max))) or div_max)
    kw['ltc7106_rfb1'] = int(
        input(
            "Enter LTC7106 RFB1 [{}]: ".format(
                str(ltc7106_rfb1))) or ltc7106_rfb1)
    kw['ltc7106_rfb2'] = int(
        input(
            "Enter LTC7106 RFB2 [{}]: ".format(
                str(ltc7106_rfb2))) or ltc7106_rfb2)
    kw['ltc7106_vref'] = int(
        input(
            "Enter LTC7106 Vref [{}]: ".format(
                str(ltc7106_vref))) or ltc7106_vref)
    kw['ltc7106_vdddpu'] = int(
        input(
            "Enter LTC7106 Vdd DPU [{}]: ".format(
                str(ltc7106_vdddpu))) or ltc7106_vdddpu)
    kw['vdd_limit'] = int(
        input(
            "Enter VDD limit [{}]: ".format(
                str(vdd_limit))) or vdd_limit)
    kw['chip_version'] = input(
        "Enter the chip version [{}]: ".format(
            str(chip_version))) or chip_version

    p21_database = p21_database._replace(**kw)


def get_e19_user_values():
    global e19_database

    # Dftl values
    fck = getattr(e19_database, 'fck')
    fck_min = getattr(e19_database, 'fck_min')
    fck_max = getattr(e19_database, 'fck_max')
    div_min = getattr(e19_database, 'div_min')
    div_max = getattr(e19_database, 'div_max')
    vdd_limit = getattr(e19_database, 'vdd_limit')
    chip_version = getattr(e19_database, 'chip_version')

    kw = dict()

    kw['fck'] = int(
        input(
            "Enter the default frequency [{}]: ".format(
                str(fck))) or fck)
    kw['fck_min'] = int(
        input(
            "Enter the mininum frequency [{}]: ".format(
                str(fck_min))) or fck_min)
    kw['fck_max'] = int(
        input(
            "Enter the maximum frequency [{}]: ".format(
                str(fck_max))) or fck_max)
    kw['div_min'] = int(
        input(
            "Enter the mininum clock div [{}]: ".format(
                str(div_min))) or div_min)
    kw['div_max'] = int(
        input(
            "Enter the maximum clock div [{}]: ".format(
                str(div_max))) or div_max)
    kw['vdd_limit'] = int(
        input(
            "Enter VDD limit [{}]: ".format(
                str(vdd_limit))) or vdd_limit)
    kw['chip_version'] = input(
        "Enter the chip version [{}]: ".format(
            str(chip_version))) or chip_version

    e19_database = e19_database._replace(**kw)


parser = argparse.ArgumentParser(
    description='Upmem DIMM VPD database generation tool')
parser.add_argument(
    '--dimm',
    default="P21D",
    choices=[
        "P21",
        "E19",
        "P21D"],
    help="DIMM type this VPD file is intended for")
parser.add_argument(
    '--default',
    default=False,
    action='store_true',
    help="Use database default values")

args = parser.parse_args()

# Update namedtuple with user inputs (if needed)
if args.dimm == "P21D":
    if not args.default:
        get_p21d_user_values()

    database = p21d_database
elif args.dimm == "P21":
    if not args.default:
        get_p21_user_values()

    database = p21_database
else:
    if not args.default:
        get_e19_user_values()

    database = e19_database

# Update database based on namedtuple
for key in database._fields:
    value = getattr(database, key)
    # We do not use add_numeric() here as fck_min might be detected as a byte
    # whereas the MCU FW expects a short value.
    # So use explicit value length for every key/value pairs.
    if key in ('div_min', 'div_max'):
        db.add_byte(key, value)
    elif key in ('fck', 'fck_min', 'fck_max', 'vdd_limit', 'ltc7106_rfb1', 'ltc7106_rfb2', 'ltc7106_vref', 'ltc7106_vdddpu', 'vdddpu'):
        db.add_short(key, value)
    elif key in ('chip_version'):
        db.add_string(key, value)

# Dump to file
db.write_to_file(DB_FILE)
