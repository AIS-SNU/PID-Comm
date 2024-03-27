/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

#include "dpu_region_address_translation.h"

void
fpga_aws_write_to_rank(__attribute__((unused)) struct dpu_region_address_translation *tr,
    __attribute__((unused)) void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    __attribute__((unused)) struct dpu_transfer_matrix *xfer_matrix)
{
}

void
fpga_aws_read_from_rank(__attribute__((unused)) struct dpu_region_address_translation *tr,
    __attribute__((unused)) void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    __attribute__((unused)) struct dpu_transfer_matrix *xfer_matrix)
{
}

void
fpga_aws_write_to_cis(struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    void *block_data,
    __attribute__((unused)) uint32_t block_size)
{
    uint8_t nb_cis;
    int i;

    nb_cis = tr->interleave->nb_ci;

    for (i = 0; i < nb_cis; ++i) {
        uint64_t *command = &((uint64_t *)block_data)[i];
        uint64_t off_in_bar = i * 0x1000;

        if (*command == 0ULL)
            continue;

        *(volatile uint64_t *)((uint8_t *)base_region_addr + off_in_bar) = *command;
#ifdef __x86_64__
        __asm__ volatile("sfence");
#endif
    }
}

void
fpga_aws_read_from_cis(struct dpu_region_address_translation *tr,
    void *base_region_addr,
    __attribute__((unused)) uint8_t channel_id,
    void *block_data,
    __attribute__((unused)) uint32_t block_size)
{
    uint8_t nb_cis;
    int i;

    nb_cis = tr->interleave->nb_ci;

    for (i = 0; i < nb_cis; ++i) {
        uint64_t *result = &((uint64_t *)block_data)[i];
        uint64_t result_tmp;
        uint64_t off_in_bar = i * 0x1000;

        if (*result == 0ULL)
            continue;

        result_tmp = *(volatile uint64_t *)((uint8_t *)base_region_addr + off_in_bar);

        if (result_tmp == 0ULL)
            continue;

        *result = result_tmp;
    }
}

void
fpga_aws_destroy_rank(__attribute__((unused)) struct dpu_region_address_translation *tr,
    __attribute__((unused)) uint8_t channel_id)
{
}

int
fpga_aws_init_rank(__attribute__((unused)) struct dpu_region_address_translation *tr, __attribute__((unused)) uint8_t channel_id)
{
    return 0;
}

struct dpu_region_interleaving fpga_aws_interleave = {
    .nb_ci = 4,
    .nb_dpus_per_ci = 8,
    .mram_size = 64 * 1024 * 1024,
};

struct dpu_region_address_translation fpga_aws_translate = {
    .interleave = &fpga_aws_interleave,
    .backend_id = DPU_BACKEND_FPGA_AWS,
    .capabilities = CAP_HYBRID_CONTROL_INTERFACE | CAP_SAFE,
    .hybrid_mmap_size = 4 /* nb_cis */ * 4 * 1024,
    .write_to_rank = fpga_aws_write_to_rank,
    .read_from_rank = fpga_aws_read_from_rank,
    .write_to_cis = fpga_aws_write_to_cis,
    .read_from_cis = fpga_aws_read_from_cis,
};
