/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef DPU_HW_DESCRIPTION_H
#define DPU_HW_DESCRIPTION_H

/**
 * @file dpu_hw_description.h
 * @brief Hardware DPU rank description.
 */

/**
 * @brief Description of the hardware characteristics of a DPU rank
 */
struct dpu_hw_description_t {
    /** Signature of the chip. */
    struct {
        /** Chip ID. */
        dpu_chip_id_e chip_id;
    } signature;

    /** Timing configuration. */
    struct {
        /** Carousel configuration. */
        struct dpu_carousel_config carousel;
        /** Clock frequency (in MHz). */
        uint32_t fck_frequency_in_mhz;
        /** Reset duration (in cycles). */
        uint8_t reset_wait_duration;
        /** Temperature threshold. */
        uint8_t std_temperature;
        /** Clock frequency division. */
        uint8_t clock_division;
    } timings;

    /** DPU rank topology. */
    struct {
        /** The number of Control Interfaces in a DPU rank. */
        uint8_t nr_of_control_interfaces;
        /** The number of DPUs per Control Interface. */
        uint8_t nr_of_dpus_per_control_interface;
    } topology;

    /** Memory information. */
    struct {
        /** MRAM size in bytes. */
        mram_size_t mram_size;
        /** WRAM size in words. */
        wram_size_t wram_size;
        /** IRAM size in instructions. */
        iram_size_t iram_size;
    } memories;

    /** DPU information. */
    struct {
        /** Bit shuffling configuration. */
        struct dpu_bit_config pcb_transformation;
        /** Number of hardware threads per DPU. */
        uint8_t nr_of_threads;
        /** Number of atomic bit per DPU. */
        uint32_t nr_of_atomic_bits;
        /** Number of notify bit per DPU. */
        uint32_t nr_of_notify_bits;
        /** Number of work registers per DPU thread. */
        uint8_t nr_of_work_registers_per_thread;
    } dpu;
};

#endif
