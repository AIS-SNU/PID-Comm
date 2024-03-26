#!/usr/bin/env python3
# To check the SPD EEPROM content/CRC:
# $ gen_spd.py > spd.bin
# $ hexdump spd.bin > spd.bin.hex
# $ decode-dimms -X spd.bin.hex
import datetime
import functools
import math
import subprocess
import tempfile
import sys
import argparse

def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

# Helper for 0-filled 'reserved' area
def Zeroes(start, end):
    return [0] * (end - start + 1)

def Lo(val):
    return val & 0xff

def Hi(val):
    return (val >> 8) & 0xff

# Helper for inserting little-endian 16-bit integer
def U16(val):
    return [Lo(val), Hi(val)]

# Helper for inserting little-endian 32-bit integer
def U32(val):
    return [val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff]

# CRC as defined by Annex L
def CRC16(data):
    data = bytearray(data)
    crc = 0
    for b in data:
        crc = crc ^ (b & 0xff) << 8
        for _ in range(0, 8):
            if (crc & 0x8000):
                crc = (crc << 1 ^ 0x1021)
            else:
                crc = crc << 1
    return crc & 0xFFFF

# Closure to recompute CRC once the array is filled with values
def FixupCRC(start, end):
    def RecomputeCRC(spd):
        return U16(CRC16(spd[start:end + 1]))
    return RecomputeCRC

def BCD(u8):
    return int("0x%02d" % (u8), 16)

def RevBCD(msb, lsb):
    return BCD(msb * 10 + lsb)

# Helper for manufacturing date: year and week number in BCD
def DateBCD(manuf_date):
    yr, wk, _ = manuf_date.isocalendar()
    return [BCD(yr % 100), BCD(wk)]

# Helper for supported CAS latencies
def CASRange(min_cas, max_cas):
    return U32(functools.reduce(lambda v, i: v + (1 << (i - 7)), range(min_cas, max_cas + 1), 0))

# Helper for JEDEC Manufacturer ID code
def JedecID(bank, number):
    def OddParity(u8):
        return u8 + (0 if bin(u8).count("1") & 1 else 0x80)
    return [OddParity(bank - 1), OddParity(number)]

# UPMEM got the number 68 in bank 11
UPMEM_ID = JedecID(11, 68)
# IDT is number 51 bank 1
IDT_ID = JedecID(1, 51)
# Montage is number 50 bank 7
MONTAGE_ID = JedecID(7, 50)

# We built it today
manufacturing_date = datetime.datetime.now()

# Time units and helpers (in picoseconds)
PS = 1
NS = 1000 * PS
MTB = 125 * PS
FTB =   1 * PS

#
# Timing parameters (U-Pj)
# DDR4-2400U (CL-nRCD-nRP 18-18-18)  CWL 16
#                           simu
T_CK_MIN =   0.833 * NS #             Minimum Cycle Time (tCKmin)
T_CK_MAX =   1.600 * NS #             Maximum Cycle Time (tCKmax)
T_AA     =  14.160 * NS #  12.87 * NS Minimum CAS Latency Time (tAA)             [2400T] 2400U: 15.000 * NS
T_RCD    =  14.160 * NS #  13.25 * NS Minimum RAS to CAS Delay (tRCD)            [2400T] 2400U: 15.000 * NS
T_RP     =  14.160 * NS #  13.83 * NS Minimum Row Precharge Delay (tRP)          [2400T] 2400U: 15.000 * NS
T_RAS    =  32.000 * NS #  27.49 * NS Minimum Active to Precharge Delay (tRAS)   [2400T] 2400U: 32.000 * NS
T_RC     =  46.160 * NS #  41.32 * NS Minimum Active to Auto-Refresh Delay (tRC) [2400T] 2400U: 47.000 * NS
T_RFC1   = 350.000 * NS # 297.6  * NS Minimum Recovery Delay (tRFC1) [8Gb]
T_RFC2   = 260.000 * NS #             Minimum Recovery Delay (tRFC2) [8Gb]
T_RFC4   = 160.000 * NS #             Minimum Recovery Delay (tRFC4) [8Gb]
T_FAW    =  21.000 * NS #             Minimum Four Activate Window Delay (tFAW)  [1KB page]
T_RRD_S  =   3.300 * NS #             Minimum Row Active to Row Active Delay (tRRD_S)
T_RRD_L  =   4.900 * NS #             Minimum Row Active to Row Active Delay (tRRD_L)
T_CCD_L  =   5.000 * NS #             Minimum CAS to CAS Delay (tCCD_L)
T_WR     =  15.000 * NS #   9.45 * NS Minimum Write Recovery Time (tWR)
T_WTR_S  =   2.500 * NS #             Minimum Write to Read Time (tWTR_S)
T_WTR_L  =   7.500 * NS #   7.03 * NS Minimum Write to Read Time (tWTR_L)

def TimeMTB(dly):
    return int(math.ceil(dly / MTB))
def OffsetFTB(dly):
    return int((dly - TimeMTB(dly) * MTB) / FTB) & 0xff

# Module organization
module_organization_possible_values = [
    0x01, 0x09, 0x00, 0x08, 0x19, 0x18, 0x02, 0x0A]
MODULE_ORGANIZATION = 0x09 # Module Organization: 2 Ranks of x8 chips

def SetModuleOrganization(organization):
    global MODULE_ORGANIZATION

    organization = int(organization, 16)
    if organization in module_organization_possible_values:
        MODULE_ORGANIZATION = organization
    else:
        print_error("Invalid module organization (%s). Possible values are %s" % (
            hex(organization), [hex(i) for i in module_organization_possible_values]))
        sys.exit(-1)

# Register manufacturer ID code
rcd_manufacturer_id_possible_values = ["montage", "idt"]
RCD_MANUFACTURER_ID = MONTAGE_ID # Register Manufacturer ID Code : Montage
RCD_REV_NUMBER = 0xB1 # Register Revision Number

def SetRegisterManufacturerID(manufacturer):
    global RCD_MANUFACTURER_ID
    global RCD_REV_NUMBER

    manufacturer = manufacturer.lower()
    if manufacturer in rcd_manufacturer_id_possible_values:
        if manufacturer == "montage":
            RCD_MANUFACTURER_ID = MONTAGE_ID
            RCD_REV_NUMBER = 0xB1
        elif manufacturer == "idt":
            RCD_MANUFACTURER_ID = IDT_ID
            RCD_REV_NUMBER = 0x30
    else:
        print_error("Invalid RCD manufacturer ID. Possible values are %s" %
                    rcd_manufacturer_id_possible_values)
        sys.exit(-1)

# Part Number
PART_NUMBER_LEN = 20
PART_NUMBER = 'UpMxxP21X18G24CxCC  ' # Module Part Number

def SetPartNumber(part_number):
    global PART_NUMBER

    part_number = [ord(i) for i in part_number.ljust(PART_NUMBER_LEN)]
    if len(part_number) == PART_NUMBER_LEN:
        PART_NUMBER = part_number
    else:
        print_error("Invalid part number. Part number is at most 20-byte long")
        sys.exit(-1)

# DRAM stepping
dram_stepping_possible_values = [0x00, 0x01, 0x31, 0xA3, 0xB1, 0xFF]
DRAM_STEPPING = 0xB1 # DRAM Stepping (die revision level of DRAM chip) : 0xB1 == stepping B.1

def SetDRAMStepping(stepping):
    global DRAM_STEPPING

    stepping = int(stepping, 16)
    if stepping in dram_stepping_possible_values:
        DRAM_STEPPING = stepping
    else:
        print_error("Invalid DRAM stepping (%s). Possible values are %s" % (
            hex(stepping), [hex(i) for i in dram_stepping_possible_values]))
        sys.exit(-1)

# Transform the SPD information into an array of byte values
def Flatten(data):
    flat = bytearray()
    for b in data:
        if type(b) == int:
            flat.append(b)
        elif type(b) == list:
            flat.extend(b)
        elif type(b) == str:
            flat.extend([ord(c) for c in b])
        elif type(b) == type(Flatten):
            flat.extend(b(flat))
        else:
            print_error("JUNK %s" % (b))
    return flat

# Dump SPD EEPROM content to stream
def Dump(data, strm):
    strm.write(data)

# Parse arguments
parser = argparse.ArgumentParser(
    description="SPD EEPROM content generating script")
parser.add_argument(
    "--module-organization",
    help="This byte describes the organization of the module, default is 09h. Possible values are %s" % [hex(i) for i in module_organization_possible_values])
parser.add_argument(
    "--rcd-manufacturer-id",
    help="This two-byte field indicates the manufacturer of the register on the module, default is \"montage\". Possible values are %s" % rcd_manufacturer_id_possible_values)
parser.add_argument(
    "--part-number",
    help="The manufacturer’s part number is written in ASCII format, default is 'UpMxxP21X18G24CxCC'")
parser.add_argument(
    "--dram-stepping",
    help="This byte defines the vendor die revision level of the DRAMs on the module, default is B1h. Possible values are %s" % [hex(i) for i in dram_stepping_possible_values])
args = parser.parse_args()

if args.module_organization:
    SetModuleOrganization(args.module_organization)

if args.rcd_manufacturer_id:
    SetRegisterManufacturerID(args.rcd_manufacturer_id)

if args.part_number:
    SetPartNumber(args.part_number)

if args.dram_stepping:
    SetDRAMStepping(args.dram_stepping)

# Serial Presence Detect 512-byte EEPROM content
SPD = [
    # @ 0x000 : Base Configuration and DRAM Parameters
    0x23, # Number of SPD bytes written / device size: 384 bytes in 512-byte EEPROM
    RevBCD(1, 1), # SPD Revision: 1.1
    0x0c, # DRAM Device type: DDR4 SDRAM
    0x01, # Module Type: RDIMM (=0x01)
    0x85, # DRAM density and Banks: [7:6] 4 Bank groups [5:4] 4 Bank address [3:0] 8 Gb
    0x21, # DRAM Addressing: [5:3] 16b Row addr [2:0] 10b 10-bit Col addr
    0x00, # Primary DRAM Package Type: 0 == Monolithic single-die package
    0x08, # DRAM Optional Features: [5:4] tMAW = 8192 tREFI [3:0] 8 ==Unlimited Max Activate
    0x00, # DRAM Thermal and Refresh Options:
    0x60, # Other DRAM Optional Features: Post package repair supported, one row per bank group / Soft PPR supported
    0x00, # Secondary DRAM Package Type: Monolithic single-die package
    0x03, # Module Nominal Voltage, VDD: 1.2V
    MODULE_ORGANIZATION, # Module Organization
    0x03, # Module Memory Bus Width: 64b no ECC (64b + 8b ECC is 0x0b)
    0x80, # Module Thermal Sensor: includes TSE2004a temperature sensor
    0x00, # Extended module type
    # @ 0x010
    0x00, # Reserved (must be 0)
    0x00, # Timebases: [3:2] MTB == 125ps [1:0] FTB == 1ps
    TimeMTB(T_CK_MIN), # DRAM Minimum Cycle Time (tCKAVGmin)
    TimeMTB(T_CK_MAX), # DRAM Maximum Cycle Time (tCKAVGmax)
    CASRange(10, 21), # CAS Latencies Supported
    TimeMTB(T_AA), # Minimum CAS Latency Time (tAAmin)
    TimeMTB(T_RCD), # Minimum RAS to CAS Delay Time (tRCDmin)
    TimeMTB(T_RP), # Minimum Row Precharge Delay Time (tRPmin)
    (Hi(TimeMTB(T_RC)) << 4) | Hi(TimeMTB(T_RC)), # Upper Nibbles for tRASmin and tRCmin
    Lo(TimeMTB(T_RAS)), # Minimum Active to Precharge Delay Time (tRASmin), LSB
    Lo(TimeMTB(T_RC)), # Minimum Active to Active/Refresh Delay Time (tRCmin), LSB
    U16(TimeMTB(T_RFC1)), # Minimum Refresh Recovery Delay Time (tRFC1min)
    # @ 0x020
    U16(TimeMTB(T_RFC2)), # Minimum Refresh Recovery Delay Time (tRFC2min)
    U16(TimeMTB(T_RFC4)), # Minimum Refresh Recovery Delay Time (tRFC4min)
    Hi(TimeMTB(T_FAW)), # Minimum Four Activate Window Time (tFAWmin), Most Significant Nibble
    Lo(TimeMTB(T_FAW)), # Minimum Four Activate Window Time (tFAWmin), Least Significant Byte
    TimeMTB(T_RRD_S), # Minimum Activate to Activate Delay Time (tRRD_Smin), different bank group
    TimeMTB(T_RRD_L), # Minimum Activate to Activate Delay Time (tRRD_Lmin), same bank group
    TimeMTB(T_CCD_L), # Minimum CAS to CAS Delay Time (tCCD_Lmin), same bank group
    Hi(TimeMTB(T_WR)), # Upper Nibble for tWRmin
    Lo(TimeMTB(T_WR)), # Minimum Write Recovery Time (tWRmin)
    Hi(TimeMTB(T_WTR_L)), # Upper nibbles for tWTRmin
    Lo(TimeMTB(T_WTR_S)), # Minimum Write to Read Time (tWTR_Smin), different bank group
    Lo(TimeMTB(T_WTR_L)), # Minimum Write to Read Time (tWTR_Lmin), same bank group
    Zeroes(0x02E, 0x03B), # Reserved (must be 0)
    # 0x03C - 0x04D Connector to SDRAM Bit Mapping
    # 0x03C DQ0-3   0x042 DQ24-27 0x048 DQ40-43 0x03D DQ4-7   0x043 DQ28-31 0x049 DQ44-47
    # 0x03E DQ8-11  0x044 CB0-3   0x04A DQ48-51 0x03F DQ12-15 0x045 CB4-7   0x04B DQ52-55
    # 0x040 DQ16-19 0x046 DQ32-35 0x04C DQ56-59 0x041 DQ20-23 0x047 DQ36-39 0x04D DQ60-63
    0x0e, 0x2e, 0x16, 0x36, 0x16, 0x36, 0x16, 0x36, 0x0e, 0x2e, 0x23, 0x04, 0x2b, 0x0c, 0x2b, 0x0c, 0x23, 0x04,
    Zeroes(0x04E, 0x074), # Reserved
    OffsetFTB(T_CCD_L), # Fine Offset for Minimum CAS to CAS Delay Time (tCCD_Lmin), same bank group
    OffsetFTB(T_RRD_L), # Fine Offset for Minimum Activate to Activate Delay Time (tRRD_Lmin), same bank group
    OffsetFTB(T_RRD_S), # Fine Offset for Minimum Activate to Activate Delay Time (tRRD_Smin), different bank group
    OffsetFTB(T_RP), # Fine Offset for Minimum Activate to Activate/Refresh Delay Time (tRCmin)
    OffsetFTB(T_RP), # Fine Offset for Minimum Row Precharge Delay Time (tRPmin)
    OffsetFTB(T_RCD), # Fine Offset for Minimum RAS to CAS Delay Time (tRCDmin)
    OffsetFTB(T_AA), # Fine Offset for Minimum CAS Latency Time (tAAmin)
    OffsetFTB(T_CK_MAX), # Fine Offset for SDRAM Maximum Cycle Time (tCKAVGmax)
    OffsetFTB(T_CK_MIN), # Fine Offset for SDRAM Minimum Cycle Time (tCKAVGmin)
    FixupCRC(0x000, 0x07D), # CRC for base configuration section
    # @ 0x080 : Module Specific Parameters : R-DIMM overlay
    0x11, # Raw Card Extension, Module Nominal Height
    0x11, # Module Maximum Thickness
    0xFF, # Reference Raw Card Used (custom)
    0x09, # DIMM Attributes: [7:4] DDR4RCD01 [3:2] 2 Row [1:0] 1 Register
    0x00, # RDIMM Thermal Heat Spreader Solution: no heat spreader
    RCD_MANUFACTURER_ID, # Register Manufacturer ID Code
    RCD_REV_NUMBER, # Register Revision Number
    0x00, # Address Mapping from Register to DRAM: Standard (aka NOT mirrored)
    0x00, # Register Output Drive Strength for Control and Command/Address: light drive
    0x00, # Register Output Drive Strength for Clock: light drive
    # @ 0x08B-0xBF : R-DIMM reserved (must be 0)
    Zeroes(0x08B, 0x0BF),
    # @ 0x0C0 - 0x0FD : Hybrid Memory Parameters
    Zeroes(0x0C0, 0x0FD),
    # @ 0x0FE - 0x0FF
    FixupCRC(0x080, 0x0FD),  # CRC for block 1
    # @ 0x100 --0x13F : Extended Function Parameter Block
    Zeroes(0x100, 0x13F),
    # @ 0x140 : Manufacturing information
    UPMEM_ID, # Module Manufacturer ID Code : UPMEM
    0x02, # Module Manufacturing Location
    DateBCD(manufacturing_date), # Module Manufacturing Date
    U32(0xAFDBBDFA), # Module Serial Number
    PART_NUMBER, # Module Part Number
    0x01, # Module Revision Code
    UPMEM_ID, # DRAM Manufacturer ID Code
    # @ 0x160
    DRAM_STEPPING, # DRAM Stepping (die revision level of DRAM chip)
    Zeroes(0x161, 0x17D), # Module Manufacturer Specific Data
    Zeroes(0x17E, 0x17F), # Reserved (must be 0)
    # @ 0x180 - 0x1FF End User Programmable
    Zeroes(0x180, 0x1FF)
]

SPD = Flatten(SPD)
Dump(SPD, sys.stdout.buffer)
