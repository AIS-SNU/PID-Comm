/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */
#ifndef DPU_VPD_STRUCTURES_H
#define DPU_VPD_STRUCTURES_H

/**
 * @file dpu_vpd_structures.h
 * @brief Definition of VPD data structures
 */

/**
 * @brief The maximum number of ranks per DIMM.
 * @internal Currently support up to quad-rank DIMMs
 * @hideinitializer
 */
#define VPD_MAX_RANK 4

/**
 * @brief The magic used in the VPD structure for UPMEM DIMMs.
 * @hideinitializer
 */
#define VPD_STRUCT_ID "UPMV"

/**
 * @brief The current version in the VPD structure for UPMEM DIMMs.
 * @hideinitializer
 */
#define VPD_STRUCT_VERSION (0x0004000)

/**
 * @brief Maximum size for the VPD structure.
 * @hideinitializer
 */
#define VPD_MAX_SIZE 2048

/**
 * @brief Special value used for undefined repair entry.
 * @hideinitializer
 */
#define VPD_UNDEFINED_REPAIR_COUNT ((uint16_t)-1)

/**
 * @brief Type of repair entry
 */
enum dpu_vpd_repair_type {
    /** IRAM repair type. */
    DPU_VPD_REPAIR_IRAM,
    /** WRAM repair type. */
    DPU_VPD_REPAIR_WRAM
};

/**
 * @brief Holds information about DPUs that contain repairs or that are disabled
 */
struct dpu_vpd_rank_data {
    /** Bitmap of disabled DPUs. */
    uint64_t dpu_disabled;
    /** Bitmap of WRAM requiring repairs (1 bit per DPU).*/
    uint64_t wram_repair;
    /** Bitmap of IRAM requiring repairs (1 bit per DPU).*/
    uint64_t iram_repair;
};

/**
 * @brief Description of a repair entry
 */
struct dpu_vpd_repair_entry {
    /** VPD_REPAIR_IRAM or VPD_REPAIR_WRAM. */
    uint8_t iram_wram;
    /** Rank ID. */
    uint8_t rank;
    /** Control Interface index. */
    uint8_t ci;
    /** DPU member ID. */
    uint8_t dpu;
    /** Bank number. */
    uint8_t bank;
    /** Padding byte to align the following fields. */
    uint8_t __padding;
    /** Address to repair. */
    uint16_t address;
    /** Bits to repair. */
    uint64_t bits;
};

/**
 * @brief UPMEM DIMM Vital Product Data
 */
struct dpu_vpd_header {
    /** Contains 'U','P','M', 'V' for UPMEM VPD. */
    char struct_id[4];
    /** The VPD structure version. */
    uint32_t struct_ver;
    /** The VPD structure size. */
    uint16_t struct_size;
    /** Total number of ranks on the DIMM. */
    uint8_t rank_count;
    /** Padding byte to align the following fields. */
    uint8_t __padding_0;
    /** Number of entries in the SRAM repairs list: -1 means no repair done yet. */
    uint16_t repair_count;
    /** Padding byte to align the following fields. */
    uint16_t __padding_1;
    /** Repair information for each DPU rank of the DIMM. */
    struct dpu_vpd_rank_data ranks[VPD_MAX_RANK];
    /* followed by the SRAM repairs list with `repair_count` entries */
};

/**
 * @brief The maximum number of repair entries in the VPD structure.
 * @hideinitializer
 */
#define NB_MAX_REPAIR ((VPD_MAX_SIZE - sizeof(struct dpu_vpd_header)) / sizeof(struct dpu_vpd_repair_entry))

/**
 * @brief Specify data type
 */
enum vpd_data_type {
    VPD_TYPE_STRING = 0,
    VPD_TYPE_BYTE = 1,
    VPD_TYPE_SHORT = 2,
    VPD_TYPE_INT = 4,
    VPD_TYPE_LONG = 8,
    VPD_TYPE_BYTEARRAY = 9,
};

/**
 * @brief Structure for VPD handling
 */
struct dpu_vpd {
    /** VPD Header. */
    struct dpu_vpd_header vpd_header;
    /** Repair information entries. */
    struct dpu_vpd_repair_entry repair_entries[NB_MAX_REPAIR];
};

/**
 * @brief Container data types
 */
struct dpu_vpd_string_pair {
    /** Key for this VPD entry, as a string. */
    uint8_t *key;
    /** Value for this VPD entry, as a string. */
    uint8_t *value;
    /** Size of the value in bytes. */
    int value_len;
    /** Type of the value. */
    int value_type;
    /** Next VPD entry in the database. */
    struct dpu_vpd_string_pair *next;
};

/**
 * @brief Structure for VPD database handling
 */
struct dpu_vpd_database {
    /** First VPD entry in the database. */
    struct dpu_vpd_string_pair *first;
};

/**
 * @brief Status returned by the VPD API
 */
enum dpu_vpd_error {
    /** The operation was successful. */
    DPU_VPD_OK,
    DPU_VPD_ERR_HEADER_FORMAT,
    DPU_VPD_ERR_HEADER_VERSION,
    DPU_VPD_ERR_REPAIR_ENTRIES,
    DPU_VPD_ERR_NB_MAX_REPAIR,
    DPU_VPD_ERR_FLASH,
    DPU_VPD_ERR_DPU_ALREADY_ENABLED,
    DPU_VPD_ERR_DPU_ALREADY_DISABLED,
    DPU_VPD_ERR_OVERFLOW,
    /** The operation failed. */
    DPU_VPD_ERR,
};

#endif /* DPU_VPD_H */
