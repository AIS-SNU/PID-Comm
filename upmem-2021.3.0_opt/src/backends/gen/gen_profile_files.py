#!/usr/bin/env python3

#
# Copyright (c) 2021 - UPMEM
#

import argparse
from collections import namedtuple
import os

Property = namedtuple('Property', ['name', 'ctype'])

PROPERTIES = [
    # All
    Property('backend'                 , None  ),
    Property('chipId'                  , 'u32' ),
    Property('clockDivision'           , 'u32' ),
    Property('fckFrequency'            , 'u32' ),
    Property('enableProfiling'         , None  ),
    Property('profilingDpuId'          , 'u32' ),
    Property('profilingSliceId'        , 'u32' ),
    Property('mcountAddress'           , 'u32' ),
    Property('retMcountAddress'        , 'u32' ),
    Property('threadProfilingAddress'  , 'u32' ),
    Property('mramAccessByDpuOnly'     , 'bool'),
    Property('disabledMask'            , 'u64' ),
    Property('disableSafeChecks'       , 'bool'),
    Property('disableMuxSwitch'        , 'bool'),
    Property('disableResetOnAlloc'     , 'bool'),
    Property('cmdsBufferSize'          , 'u64' ),
    Property('dispatchOnAllRanks'      , 'bool'),

    # High-Level API
    Property('nrThreadsPerRank'        , 'u32' ),
    Property('nrJobsPerRank'           , 'u32' ),

    # FSIM
    Property('nrDpusPerCi'             , 'u32' ),
    Property('iramSize'                , 'u32' ),
    Property('mramSize'                , 'u32' ),
    Property('wramSize'                , 'u32' ),
    Property('doProfiling'             , 'bool'),
    Property('profileInstruction'      , 'str' ),
    Property('profileDma'              , 'str' ),
    Property('profileMemory'           , 'str' ),
    Property('profileIla'              , 'str' ),

    # Modelsim
    Property('modelsimMramFile'        , 'str' ),
    Property('modelsimWorkDir'         , 'str' ),
    Property('modelsimExecPath'        , 'str' ),
    Property('modelsimTclScript'       , 'str' ),
    Property('modelsimEnableGui'       , 'bool'),
    Property('modelsimBypassMramSwitch', 'bool'),

    # FPGA
    Property('reportFileName'          , 'str' ),
    Property('analyzerEnabled'         , 'bool'),
    Property('analyzerFilteringEnabled', 'bool'),
    Property('mramBypassEnabled'       , 'bool'),
    Property('mramEmulateRefresh'      , 'int' ),
    Property('cycleAccurate'           , 'bool'),

    # HW
    Property('regionMode'              , None  ),
    Property('rankPath'                , 'str' ),
    Property('ignoreVersion'           , 'bool'),
    Property('tryRepairIram'           , 'bool'),
    Property('tryRepairWram'           , 'bool'),
    Property('ignoreVpd'               , 'bool'),

    # Xeon sp
    Property('nrThreadPerPool'         , 'int' ),
    Property('poolThreshold1Thread'    , 'int' ),
    Property('poolThreshold2Threads'   , 'int' ),
    Property('poolThreshold4Threads'   , 'int' ),

    # Backup SPI
    Property('usbSerial'               , 'str' ),
    Property('chipSelect'              , 'u32' ),
]

COPYRIGHT_HEADER = '''/* Copyright 2021 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'''

def gen_property_macros(output):
    dir = os.path.dirname(output)
    filename = os.path.basename(output)
    with open(os.path.join(dir, filename), 'w') as f:
        f.write(COPYRIGHT_HEADER)
        guard_name = filename.replace('.', '_').upper()
        f.write(f'#ifndef {guard_name}\n')
        f.write(f'#define {guard_name}\n')
        f.write('\n')

        for property in PROPERTIES:
            name = property.name
            suffix = ''.join(['_' + i.lower() if i.isupper() or i.isdigit() else i for i in name]).lstrip('_')
            f.write(f'#define DPU_PROFILE_PROPERTY_{suffix.upper()} "{property.name}"\n')

        f.write('\n')
        f.write(f'#endif /* {guard_name} */')


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='sub-command help', dest='mode')
    subparsers.required=True

    defs_parser = subparsers.add_parser('defs', help='defs help')
    defs_parser.add_argument('output')

    args = parser.parse_args()

    if args.mode == 'defs':
        gen_property_macros(args.output)


if __name__ == '__main__':
    main()
