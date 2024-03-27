/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __DPU_CHIP_H
#define __DPU_CHIP_H

/**
 * @file dpu_chip_id.h
 * @brief Definition of the different DPU chip IDs.
 */

/**
 * @brief The different valid DPU chip IDs.
 */
typedef enum _dpu_chip_id_e {
    vD = 0,
    vD_cas = 1,
    vD_fun = 2,
    vD_asic1 = 3,
    vD_asic8 = 4,
    vD_fpga1 = 5,
    vD_fpga8 = 6,
    vD_asic4 = 7,
    vD_fpga4 = 8,

    NEXT_DPU_CHIP_ID = 9
} dpu_chip_id_e;

#endif /* __DPU_CHIP_H */
