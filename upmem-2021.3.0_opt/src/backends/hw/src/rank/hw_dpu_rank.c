/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dpu_chip_config.h>
#include <string.h>
#include <dpu_profile.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/user.h>
#include <dpu_description.h>
#include <dpu_types.h>
#include <dpu_mask.h>
#include <dpu_log_utils.h>
#include <dpu_vpd.h>
#include <dpu_internals.h>
#include <dpu_target_macros.h>

#include "dpu_attributes.h"
// TODO: will conflict with driver header
#include "dpu_rank.h"
#include "dpu_mask.h"

/* Header shared with driver */
#include "dpu_region_address_translation.h"
#include "dpu_region_constants.h"
#include "hw_dpu_sysfs.h"
#include "dpu_rank_ioctl.h"
#include "dpu_fpga_ila.h"

#include "dpu_module_compatibility.h"

#include "static_verbose.h"
#include <pthread.h>

const char *
get_rank_path(dpu_description_t description);

static struct verbose_control *this_vc;
static struct verbose_control *
__vc()
{
    if (this_vc == NULL) {
        this_vc = get_verbose_control_for("hw");
    }
    return this_vc;
}

extern struct dpu_region_address_translation power9_translate;
extern struct dpu_region_address_translation xeon_sp_translate;
extern struct dpu_region_address_translation fpga_aws_translate;

struct dpu_region_address_translation *backend_translate[] = {
#ifdef __x86_64__
    &xeon_sp_translate,
#else
    0,
#endif
    0, /* fpga_kc705 has no user backend */
    &fpga_aws_translate,
#ifdef __powerpc64__
    &power9_translate,
#else
    0,
#endif
    0, /* devicetree user backend not yet implemented */
};

static dpu_rank_status_e
hw_allocate(struct dpu_rank_t *rank, dpu_description_t description);
static dpu_rank_status_e
hw_free(struct dpu_rank_t *rank);
static dpu_rank_status_e
hw_commit_commands(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer);
static dpu_rank_status_e
hw_update_commands(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer);
static dpu_rank_status_e
hw_copy_to_rank(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);
static dpu_rank_status_e
hw_copy_from_rank(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);

static dpu_rank_status_e
hw_all_to_all_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis);
static dpu_rank_status_e
hw_all_to_all_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_to_all_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_to_all_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_to_all_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);

static dpu_rank_status_e
hw_all_gather_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis);
static dpu_rank_status_e
hw_all_gather_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_gather_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_gather_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
static dpu_rank_status_e
hw_all_gather_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);

static dpu_rank_status_e
hw_reduce_scatter_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size);
static dpu_rank_status_e
hw_reduce_scatter_cpu_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size);
static dpu_rank_status_e
hw_reduce_scatter_cpu_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size);

static dpu_rank_status_e
hw_all_reduce_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, uint32_t reduce_type);
static dpu_rank_status_e
hw_all_reduce_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type);
static dpu_rank_status_e
hw_all_reduce_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type);

//static dpu_rank_status_e
//hw_gather_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer);
static dpu_rank_status_e
hw_gather_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);
static dpu_rank_status_e
hw_gather_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer);
static dpu_rank_status_e
hw_gather_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void ** host_buffer);
static dpu_rank_status_e
hw_gather_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void ** host_buffer);

static dpu_rank_status_e
hw_reduce_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t total_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, void **host_buffer);
//static dpu_rank_status_e
//hw_reduce_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void **host_buffer);
//static dpu_rank_status_e
//hw_reduce_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void **host_buffer);

static dpu_rank_status_e
hw_scatter_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer);
static dpu_rank_status_e
hw_scatter_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);
static dpu_rank_status_e
hw_scatter_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer);


static dpu_rank_status_e
hw_fill_description_from_profile(dpu_properties_t properties, dpu_description_t description);
static dpu_rank_status_e
hw_custom_operation(struct dpu_rank_t *rank,
    dpu_slice_id_t slice_id,
    dpu_member_id_t member_id,
    dpu_custom_command_t command,
    dpu_custom_command_args_t args);
static dpu_rank_status_e
hw_get_nr_dpu_ranks(uint32_t *nr_ranks);

__API_SYMBOL__ struct dpu_rank_handler hw_dpu_rank_handler = {
    .allocate = hw_allocate,
    .free = hw_free,
    .commit_commands = hw_commit_commands,
    .update_commands = hw_update_commands,
    .copy_to_rank = hw_copy_to_rank,
    .copy_from_rank = hw_copy_from_rank,
    .all_to_all_rns = hw_all_to_all_rns,
    .all_to_all_x_rns = hw_all_to_all_x_rns,
    .all_to_all_xz_rns = hw_all_to_all_xz_rns,
    .all_to_all_y_rns = hw_all_to_all_y_rns,
    .all_to_all_z_rns = hw_all_to_all_z_rns,
    .all_gather_rns = hw_all_gather_rns,
    .all_gather_x_rns = hw_all_gather_x_rns,
    .all_gather_y_rns = hw_all_gather_y_rns,
    .all_gather_z_rns = hw_all_gather_z_rns,
    .all_gather_xz_rns = hw_all_gather_xz_rns,
    .reduce_scatter_rns = hw_reduce_scatter_rns,
    .reduce_scatter_cpu_x_rns = hw_reduce_scatter_cpu_x_rns,
    .reduce_scatter_cpu_y_rns = hw_reduce_scatter_cpu_y_rns,
    .all_reduce_rns = hw_all_reduce_rns,
    .all_reduce_x_rns = hw_all_reduce_x_rns,
    .all_reduce_y_rns = hw_all_reduce_y_rns,
    //.gather_rns = hw_gather_rns,
    .gather_x_rns = hw_gather_x_rns,
    .gather_y_rns = hw_gather_y_rns,
    .gather_z_rns = hw_gather_z_rns,
    .gather_xz_rns = hw_gather_xz_rns,
    .reduce_rns = hw_reduce_rns,
    //.reduce_x_rns = hw_reduce_x_rns,
    //.reduce_y_rns = hw_reduce_y_rns,
    .scatter_rns = hw_scatter_rns,
    .scatter_x_rns = hw_scatter_x_rns,
    .scatter_y_rns = hw_scatter_y_rns,
    .fill_description_from_profile = hw_fill_description_from_profile,
    .custom_operation = hw_custom_operation,
    .get_nr_dpu_ranks = hw_get_nr_dpu_ranks,
};

typedef struct _hw_dpu_rank_context_t {
    /* Hybrid mode: Address of control interfaces when memory mapped
     * Perf mode:   Base region address, mappings deal with offset to target control interfaces
     * Safe mode:   Buffer handed to the driver
     */
    uint64_t *control_interfaces;
} * hw_dpu_rank_context_t;

typedef struct _fpga_allocation_parameters_t {
    bool activate_ila;
    bool activate_filtering_ila;
    bool activate_mram_bypass;
    bool activate_mram_refresh_emulation;
    unsigned int mram_refresh_emulation_period;
    char *report_path;
    bool cycle_accurate;
} fpga_allocation_parameters_t;

typedef struct _hw_dpu_rank_allocation_parameters_t {
    struct dpu_rank_fs rank_fs;
    struct dpu_region_address_translation translate;
    struct dpu_region_interleaving interleave;
    uint64_t region_size;
    uint8_t mode, dpu_chip_id, backend_id;
    uint8_t channel_id;
    uint8_t *ptr_region;
    bool bypass_module_compatibility;
    /* Backends specific */
    fpga_allocation_parameters_t fpga;
} * hw_dpu_rank_allocation_parameters_t;

static inline hw_dpu_rank_context_t
_this(struct dpu_rank_t *rank)
{
    return (hw_dpu_rank_context_t)(rank->_internals);
}

static inline hw_dpu_rank_allocation_parameters_t
_this_params(dpu_description_t description)
{
    return (hw_dpu_rank_allocation_parameters_t)(description->_internals.data);
}

static inline bool
fill_description_with_default_values_for(dpu_chip_id_e chip_id, dpu_description_t description)
{
    switch (chip_id) {
        default:
            return false;
        case vD_asic1:
        case vD_asic4:
        case vD_asic8:
        case vD_fpga1:
        case vD_fpga4:
        case vD_fpga8:
        case vD:
            break;
    }

    dpu_description_t default_description;

    if ((default_description = default_description_for_chip(chip_id)) == NULL) {
        return false;
    }

    memcpy(description, default_description, sizeof(*description));

    return true;
}

static bool
fill_dpu_region_interleaving_values(dpu_description_t description)
{
    hw_dpu_rank_allocation_parameters_t params = _this_params(description);

    params->interleave.mram_size = description->hw.memories.mram_size;
    params->interleave.nb_dpus_per_ci = description->hw.topology.nr_of_dpus_per_control_interface;
    params->interleave.nb_ci = description->hw.topology.nr_of_control_interfaces;

    return true;
}

static bool
fill_address_translation_backend(hw_dpu_rank_allocation_parameters_t params)
{
    params->backend_id = dpu_sysfs_get_backend_id(&params->rank_fs);
    if (params->backend_id >= DPU_BACKEND_NUMBER)
        return false;

    if (!backend_translate[params->backend_id]) {
        LOG_FN(WARNING, "No perf mode is available for the backend %d", params->backend_id);
        return false;
    }
    struct dpu_transfer_thread_configuration xfer_thread_conf = params->translate.xfer_thread_conf;
    memcpy(&params->translate, backend_translate[params->backend_id], sizeof(struct dpu_region_address_translation));
    params->translate.xfer_thread_conf = xfer_thread_conf;
    params->translate.interleave = &params->interleave;
    params->translate.one_read = false;

    return true;
}

__attribute__((used)) static void
hw_set_debug_mode(struct dpu_rank_t *rank, uint8_t mode)
{
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);
    int ret;

    ret = ioctl(params->rank_fs.fd_rank, DPU_RANK_IOCTL_DEBUG_MODE, mode);
    if (ret)
        LOG_RANK(WARNING, rank, "Failed to change debug mode (%s)", strerror(errno));
}

static void
add_faulty_memory_address(struct dpu_memory_repair_t *repair_info, uint16_t address, uint64_t faulty_bits)
{
    uint32_t previous_nr_of_corrupted_addr = repair_info->nr_of_corrupted_addresses++;
    uint32_t index = previous_nr_of_corrupted_addr;
    uint32_t current_max_index
        = (previous_nr_of_corrupted_addr > NB_MAX_REPAIR_ADDR) ? NB_MAX_REPAIR_ADDR : previous_nr_of_corrupted_addr;

    for (uint32_t each_known_corrupted_addr_idx = 0; each_known_corrupted_addr_idx < current_max_index;
         ++each_known_corrupted_addr_idx) {
        if (repair_info->corrupted_addresses[each_known_corrupted_addr_idx].address == address) {
            index = each_known_corrupted_addr_idx;
            repair_info->nr_of_corrupted_addresses--;
            break;
        }
    }
    if (index < NB_MAX_REPAIR_ADDR) {
        repair_info->corrupted_addresses[index].address = address;
        repair_info->corrupted_addresses[index].faulty_bits |= faulty_bits;
    }
}

static dpu_error_t
fill_sram_repairs_and_update_enabled_dpus(struct dpu_rank_t *rank)
{
    int status;

    const char *rank_path = get_rank_path(rank->description);
    char vpd_path[512];
    int rank_index;
    struct dpu_vpd vpd;

    status = dpu_sysfs_get_rank_index(rank_path, &rank_index);
    if (status != 0) {
        LOG_RANK(DEBUG, rank, "unable to get rank index");
        return DPU_ERR_SYSTEM;
    }

    if (dpu_vpd_get_vpd_path(rank_path, (char *)vpd_path) != DPU_VPD_OK) {
        LOG_RANK(DEBUG, rank, "unable to get vpd path");
        return DPU_ERR_SYSTEM;
    }

    if (dpu_vpd_init(vpd_path, &vpd) != DPU_VPD_OK) {
        LOG_RANK(DEBUG, rank, "unable to open sysfs VPD file");
        return DPU_ERR_VPD_INVALID_FILE;
    }

    if (vpd.vpd_header.repair_count == VPD_UNDEFINED_REPAIR_COUNT)
        return DPU_ERR_VPD_NO_REPAIR;

    uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus = rank->description->hw.topology.nr_of_dpus_per_control_interface;

    uint64_t disabled_mask = 0;
    uint16_t repair_cnt = 0;

    bool repair_requested
        = (rank->description->configuration.do_iram_repair) || (rank->description->configuration.do_wram_repair);

    if (repair_requested) 
    {
        int i;

        for (i = 0; i < vpd.vpd_header.repair_count; ++i) 
        {
            repair_cnt++;

            struct dpu_vpd_repair_entry *entry = &vpd.repair_entries[i];

            if (entry->rank != rank_index)
                continue;

            struct dpu_t *dpu = DPU_GET_UNSAFE(rank, entry->ci, entry->dpu);

            struct dpu_memory_repair_t *repair_info;

            if (entry->iram_wram == DPU_VPD_REPAIR_IRAM) 
            {
                repair_info = &dpu->repair.iram_repair;
            } 
            else 
            {
                repair_info = &dpu->repair.wram_repair[entry->bank];
            }

            add_faulty_memory_address(repair_info, entry->address, entry->bits);

            /* Temporary HACK: disable DPUs which have SRAM defects */
            disabled_mask |= (1UL << ((entry->ci * nr_dpus) + entry->dpu));
        }

        if (repair_cnt != vpd.vpd_header.repair_count) {
            LOG_RANK(WARNING, rank, "malformed VPD file");
            return DPU_ERR_VPD_INVALID_FILE;
        }
    }

    disabled_mask |= vpd.vpd_header.ranks[rank_index].dpu_disabled;

    for (uint8_t each_ci = 0; each_ci < nr_cis; ++each_ci) 
    {
        dpu_selected_mask_t disabled_mask_for_ci = (dpu_selected_mask_t)((disabled_mask >> (nr_dpus * each_ci)) & 0xFFl);

        if (disabled_mask_for_ci)
        {
            LOG_CI(VERBOSE, rank, each_ci, "Disabled mask: %x\n", disabled_mask_for_ci);
        }

        rank->runtime.control_interface.slice_info[each_ci].enabled_dpus
            &= dpu_mask_difference(dpu_mask_all(nr_dpus), disabled_mask_for_ci);
        rank->runtime.control_interface.slice_info[each_ci].all_dpus_are_enabled
            = dpu_mask_intersection(rank->runtime.control_interface.slice_info[each_ci].enabled_dpus, dpu_mask_all(nr_dpus))
            == dpu_mask_all(nr_dpus);
    }

    return DPU_OK;
}

/* Function used in dpu-diag */
__API_SYMBOL__ bool
is_kernel_module_compatible(void)
{
    /* 1/ Get module version */
    unsigned int major, minor;
    int ret = dpu_sysfs_get_kernel_module_version(&major, &minor);
    if (ret) {
        LOG_FN(WARNING, "Failed to get dpu kernel module version");
        return false;
    }

    /* 2/ Check compatibility */
    /* Do not use DPU_MODULE_MIN_MINOR directly in the comparison as the compiler
     * complains if DPU_MODULE_MIN_MINOR = 0 (comparison is always false)
     */
    unsigned int min_minor = DPU_MODULE_MIN_MINOR;
    if ((major != DPU_MODULE_EXPECTED_MAJOR) || (minor < min_minor)) {
        LOG_FN(WARNING,
            "SDK is not compatible with dpu kernel module (expected at least '%u.%u', got '%u.%u')",
            DPU_MODULE_EXPECTED_MAJOR,
            DPU_MODULE_MIN_MINOR,
            major,
            minor);
        return false;
    }

    return true;
}

/* In perf script that measures memory bandwidth, we need for per-rank
 * statistics to get the equivalence rank pointer <=> rank path: use
 * this function for perf to probe and get the rank path from the rank
 * pointer.
 */
__PERF_PROFILING_SYMBOL__ __API_SYMBOL__ void
log_rank_path(struct dpu_rank_t *rank, char *path)
{
    LOG_RANK(DEBUG, rank, "rank path is: %s", path);
}

/* Function used in dpu-diag */
__API_SYMBOL__ const char *
get_rank_path(dpu_description_t description)
{
    hw_dpu_rank_allocation_parameters_t params = _this_params(description);
    return params->rank_fs.rank_path;
}

static dpu_rank_status_e
get_byte_order(struct dpu_rank_t *rank, hw_dpu_rank_allocation_parameters_t params, uint8_t nr_cis)
{
#define BYTE_ORDER_STR_LEN strlen("0xFFFFFFFFFFFFFFFF ")
    const char *sysfs_byte_order = dpu_sysfs_get_byte_order(&params->rank_fs);
    char byte_order[BYTE_ORDER_STR_LEN * nr_cis + 1];
    uint8_t each_ci;

    strncpy(byte_order, sysfs_byte_order, BYTE_ORDER_STR_LEN * nr_cis);
    byte_order[BYTE_ORDER_STR_LEN * nr_cis] = 0;

    for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
        char *next_ptr = strtok(!each_ci ? byte_order : NULL, " ");
        if (!next_ptr)
            return DPU_RANK_SYSTEM_ERROR;

        sscanf(next_ptr, "%lx", &rank->runtime.control_interface.slice_info[each_ci].byte_order);
    }

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_allocate(struct dpu_rank_t *rank, dpu_description_t description)
{
    dpu_rank_status_e status;
    hw_dpu_rank_allocation_parameters_t params = _this_params(description);
    hw_dpu_rank_context_t rank_context;
    int ret;
    uint8_t nr_cis;

    /* 1/ Make sure SDK is compatible with the kernel module */
    static bool compatibility_checked = false;
    if ((params->bypass_module_compatibility == false) && (compatibility_checked == false)) 
    {
        if (is_kernel_module_compatible() == false) 
        {
            status = DPU_RANK_SYSTEM_ERROR;
            goto end;
        }
        compatibility_checked = true;
    }

    /* 2/ Find an available rank whose mode is compatible with the one asked
     * by the user.
     * TODO: Maybe user wants to have a specific dpu_chip_id passed as argument,
     * so we could enforce the allocation for this specific id.
     */
    ret = dpu_sysfs_get_available_rank(params->rank_fs.rank_path, &params->rank_fs);
    if (ret) {
        LOG_FN(INFO,
            "Failed to find available rank with mode %u%s",
            params->mode,
            ret == -EACCES ? ", you don't have permissions for existing devices" : "");
        status = DPU_RANK_SYSTEM_ERROR;
        goto end;
    }

    rank->rank_id = (rank->rank_id & ~DPU_TARGET_MASK) | dpu_sysfs_get_rank_id(&params->rank_fs);
    rank->numa_node = dpu_sysfs_get_numa_node(&params->rank_fs);
    params->channel_id = dpu_sysfs_get_channel_id(&params->rank_fs);

    /* 3/ dpu_rank_handler initialization */
    if ((rank_context = malloc(sizeof(*rank_context))) == NULL) {
        status = DPU_RANK_SYSTEM_ERROR;
        goto free_physical_rank;
    }

    rank->_internals = rank_context;

    params->dpu_chip_id = dpu_sysfs_get_dpu_chip_id(&params->rank_fs);

    if (params->dpu_chip_id != description->hw.signature.chip_id) {
        LOG_RANK(
            WARNING, rank, "Unexpected chip id %u (description is %u)", params->dpu_chip_id, description->hw.signature.chip_id);
        status = DPU_RANK_SYSTEM_ERROR;
        goto free_rank_context;
    }

    rank->description = description;

    // TODO: When driver safe mode is fully implemented, this must be set at false in this case.
    rank->description->configuration.api_must_switch_mram_mux = true;
    rank->description->configuration.init_mram_mux = true;

    nr_cis = description->hw.topology.nr_of_control_interfaces;

    /* Get byte order values from the driver */
    status = get_byte_order(rank, params, nr_cis);
    if (status != DPU_RANK_SUCCESS)
        goto free_rank_context;

    if (params->mode == DPU_REGION_MODE_SAFE) {
        rank_context->control_interfaces = malloc(nr_cis * sizeof(uint64_t));
        if (!rank_context->control_interfaces) {
            LOG_RANK(WARNING, rank, "Failed to allocate memory for control interfaces %u", params->dpu_chip_id);
            status = DPU_RANK_SYSTEM_ERROR;
            goto free_rank_context;
        }

    } else if (params->mode == DPU_REGION_MODE_PERF || params->mode == DPU_REGION_MODE_HYBRID) {
        /* 4/ Retrieve interleaving infos */
        if (!fill_dpu_region_interleaving_values(description)) {
            LOG_RANK(WARNING, rank, "Failed to retrieve interleaving info");
            status = DPU_RANK_SYSTEM_ERROR;
            goto free_rank_context;
        }

        /* 5/ Initialize CPU backend for this rank */
        ret = fill_address_translation_backend(params);
        if (!ret) {
            LOG_RANK(WARNING, rank, "Failed to retrieve backend");
            status = DPU_RANK_SYSTEM_ERROR;
            goto free_rank_context;
        }

        if (params->translate.init_rank) {
            ret = params->translate.init_rank(&params->translate, params->channel_id);
            if (ret < 0) 
            {
                LOG_RANK(WARNING, rank, "Failed to init rank: %s", strerror(errno));
                status = DPU_RANK_SYSTEM_ERROR;
                goto free_rank_context;
            }
        }

        if (params->mode == DPU_REGION_MODE_HYBRID && (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) == 0) {
            rank_context->control_interfaces = malloc(nr_cis * sizeof(uint64_t));
            if (!rank_context->control_interfaces) {
                LOG_RANK(WARNING, rank, "Failed to allocate memory for control interfaces %u", params->dpu_chip_id);
                status = DPU_RANK_SYSTEM_ERROR;
                goto free_rank;
            }
        } else if (params->mode == DPU_REGION_MODE_HYBRID && (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE)) {
            rank_context->control_interfaces
                = mmap(NULL, params->translate.hybrid_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, params->rank_fs.fd_rank, 0);
            if (rank_context->control_interfaces == MAP_FAILED) {
                LOG_RANK(WARNING, rank, "Failed to mmap control interfaces %u", params->dpu_chip_id);
                status = DPU_RANK_SYSTEM_ERROR;
                goto free_rank;
            }
        }

        if (params->mode == DPU_REGION_MODE_PERF) {
            /* 6/ Mmap the whole physical region */
            params->region_size = dpu_sysfs_get_region_size(&params->rank_fs);

            /* mmap does not guarantee (at all) that the address will be aligned on hugepage size (1GB) but the driver does. */
            params->ptr_region = mmap(0, params->region_size, PROT_READ | PROT_WRITE, MAP_SHARED, params->rank_fs.fd_dax, 0);
            if (params->ptr_region == MAP_FAILED) {
                LOG_RANK(WARNING, rank, "Failed to mmap dax region: %s", strerror(errno));
                status = DPU_RANK_SYSTEM_ERROR;
                goto free_ci;
            }

            rank_context->control_interfaces = (uint64_t *)params->ptr_region;
        }
    }

    /* 7/ Inform DPUs about their SRAM defects, and update CIs runtime configuration */
    /* Do not check SRAM defects in case of FPGA */
    if ((params->dpu_chip_id != vD_fpga1) && (params->dpu_chip_id != vD_fpga8) && (params->dpu_chip_id != vD_fpga4)) {
        if (!description->configuration.ignore_vpd) {
            dpu_error_t repair_status = fill_sram_repairs_and_update_enabled_dpus(rank);
            if (repair_status != DPU_OK) {
                if (repair_status == DPU_ERR_VPD_INVALID_FILE) {
                    LOG_RANK(WARNING,
                        rank,
                        "VPD can't be read, cannot allocate the rank.\n"
                        "VPD must be created using the following command:\n"
                        "dpu-diag --sram-gen-vpd --force");
                } else if (repair_status == DPU_ERR_VPD_NO_REPAIR) {
                    LOG_RANK(WARNING,
                        rank,
                        "VPD does not contain repairs, it must be created\n"
                        "using the following command:\n"
                        "dpu-diag --sram-gen-vpd");
                }
                status = DPU_RANK_SYSTEM_ERROR;
                goto free_ptr_region;
            }
        }
    }

    log_rank_path(rank, params->rank_fs.rank_path);

    return DPU_RANK_SUCCESS;

free_ptr_region:
    if (params->mode == DPU_REGION_MODE_PERF || params->mode == DPU_REGION_MODE_HYBRID)
        if (params->mode == DPU_REGION_MODE_PERF)
            munmap(params->ptr_region, params->region_size);
free_ci:
    if (params->mode == DPU_REGION_MODE_SAFE
        || (params->mode == DPU_REGION_MODE_HYBRID && (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) == 0))
        free(rank_context->control_interfaces);
    else if (params->mode == DPU_REGION_MODE_HYBRID && (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE))
        munmap(rank_context->control_interfaces, params->translate.hybrid_mmap_size);
free_rank:
    if (params->mode == DPU_REGION_MODE_PERF || params->mode == DPU_REGION_MODE_HYBRID)
        if (params->translate.destroy_rank)
            params->translate.destroy_rank(&params->translate, params->channel_id);
free_rank_context:
    free(rank_context);
free_physical_rank:
    dpu_sysfs_free_rank(&params->rank_fs);
end:
    return status;
}

static dpu_rank_status_e
hw_free(struct dpu_rank_t *rank)
{
    hw_dpu_rank_context_t rank_context = _this(rank);
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);

    if (params->mode == DPU_REGION_MODE_PERF) {
        munmap(params->ptr_region, params->region_size);
        if (params->translate.destroy_rank)
            params->translate.destroy_rank(&params->translate, params->channel_id);
    } else if (params->mode == DPU_REGION_MODE_HYBRID && (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE)) {
        munmap(rank_context->control_interfaces, params->translate.hybrid_mmap_size);
    } else
        free(rank_context->control_interfaces);

    dpu_sysfs_free_rank(&params->rank_fs);

    free(rank_context);

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_commit_commands(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer)
{
    hw_dpu_rank_context_t rank_context = _this(rank);
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);
    dpu_rank_buffer_t ptr_buffer = buffer;
    int ret;

    switch (params->mode) {
        case DPU_REGION_MODE_PERF:
            params->translate.write_to_cis(&params->translate,
                rank_context->control_interfaces,
                params->channel_id,
                ptr_buffer,
                rank->description->hw.topology.nr_of_control_interfaces * sizeof(uint64_t));
            break;
        case DPU_REGION_MODE_HYBRID:
            if (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) {
                params->translate.write_to_cis(&params->translate,
                    rank_context->control_interfaces,
                    params->channel_id,
                    ptr_buffer,
                    rank->description->hw.topology.nr_of_control_interfaces * sizeof(uint64_t));
                break;
            }
            /* fall through */
        case DPU_REGION_MODE_SAFE:
            ret = ioctl(params->rank_fs.fd_rank, DPU_RANK_IOCTL_COMMIT_COMMANDS, ptr_buffer);
            if (ret) {
                LOG_RANK(WARNING, rank, "%s", strerror(errno));
                return DPU_RANK_SYSTEM_ERROR;
            }
            break;
        default:
            return DPU_RANK_SYSTEM_ERROR;
    }

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_update_commands(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer)
{
    hw_dpu_rank_context_t rank_context = _this(rank);
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);
    dpu_rank_buffer_t ptr_buffer = buffer;
    int ret;

    switch (params->mode) {
        case DPU_REGION_MODE_PERF:
            params->translate.read_from_cis(&params->translate,
                rank_context->control_interfaces,
                params->channel_id,
                ptr_buffer,
                rank->description->hw.topology.nr_of_control_interfaces * sizeof(uint64_t));
            break;
        case DPU_REGION_MODE_HYBRID:
            if (params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) {
                params->translate.read_from_cis(&params->translate,
                    rank_context->control_interfaces,
                    params->channel_id,
                    ptr_buffer,
                    rank->description->hw.topology.nr_of_control_interfaces * sizeof(uint64_t));
                break;
            }
            /* fall through */
        case DPU_REGION_MODE_SAFE:
            ret = ioctl(params->rank_fs.fd_rank, DPU_RANK_IOCTL_UPDATE_COMMANDS, ptr_buffer);
            if (ret) {
                LOG_RANK(WARNING, rank, "%s", strerror(errno));
                return DPU_RANK_SYSTEM_ERROR;
            }

            break;
        default:
            return DPU_RANK_SYSTEM_ERROR;
    }

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_copy_to_rank(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);
    struct dpu_transfer_matrix *ptr_transfer_matrix = transfer_matrix;
    int ret;
    
    switch (params->mode) {
        case DPU_REGION_MODE_PERF:
            params->translate.write_to_rank(&params->translate, params->ptr_region, params->channel_id, ptr_transfer_matrix);

            break;
        case DPU_REGION_MODE_HYBRID:
            if ((params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) == 0) {
                params->translate.write_to_rank(&params->translate, params->ptr_region, params->channel_id, ptr_transfer_matrix);

                break;
            }
            /* fall through */
        case DPU_REGION_MODE_SAFE:
            ret = ioctl(params->rank_fs.fd_rank, DPU_RANK_IOCTL_WRITE_TO_RANK, ptr_transfer_matrix);
            if (ret) {
                LOG_RANK(WARNING, rank, "%s", strerror(errno));
                return DPU_RANK_SYSTEM_ERROR;
            }

            break;
        default:
            return DPU_RANK_SYSTEM_ERROR;
    }

    return DPU_RANK_SUCCESS;
}

typedef struct {
    uint32_t p_thread_id;
    struct dpu_set_t *p_comm_dpu_set;
    uint32_t p_src_start_offset;
    uint32_t p_dst_start_offset;
    uint32_t p_dpu_byte_length;
    uint32_t p_a;
    uint32_t p_b;
    uint32_t p_c;
    uint32_t p_alltoall_comm_type;
    uint32_t p_row_length;
    uint32_t p_communication_buffer_offset;
    uint32_t size;
    uint32_t thread_num;
    uint32_t reduce_type;
    void** p_host_buffer;
    uint32_t p_target_dpu_index;
    uint32_t p_num_thread;

}st_thread_all_to_all_x_parameter;

void *thread_all_to_all_xz_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*c*b*(a/8)*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_b=i/(c*c*(a/8)*(a/8));
        uint32_t remain_b=i-iter_b*(c*c*(a/8)*(a/8));
        uint32_t iter_src_ac=remain_b/(c*(a/8));
        uint32_t iter_dst_ac=remain_b%(c*(a/8));

        uint32_t iter_src_a=iter_src_ac%(a/8);
        uint32_t iter_dst_a=iter_dst_ac%(a/8);
        uint32_t iter_src_c=iter_src_ac/(a/8);
        uint32_t iter_dst_c=iter_dst_ac/(a/8);

        uint32_t src_rank_id=iter_src_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
        uint32_t src_rg_id = (iter_src_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        uint32_t dst_rg_id = (iter_dst_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_ac * 8 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_ac * 8 * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_to_all_z_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_b=i/((a/8)*c*c);
        uint32_t remain_a=i-iter_b*(a/8)*c*c;
        uint32_t iter_a=remain_a/(c*c);
        uint32_t remain_c=remain_a-iter_a*c*c;
        uint32_t iter_src_c=remain_c/(c);
        uint32_t iter_dst_c=remain_c%(c);

        uint32_t src_rank_id=iter_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        uint32_t dst_rank_id=iter_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
        uint32_t src_rg_id = (iter_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        uint32_t dst_rg_id = (iter_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_c * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_c * 1 * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_to_all_z_rns_24(void *thread_parameter){ //assumes a*b>=8
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*c*(b*a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_b = i / (c*c);
        uint32_t remain_c = i - iter_b * (c*c);
        uint32_t iter_src_c = remain_c / (c);
        uint32_t iter_dst_c = remain_c % (c);

        uint32_t src_rank_id=(iter_b*8 + iter_src_c*a*b)/64;
        uint32_t dst_rank_id=(iter_b*8 + iter_dst_c*a*b)/64;
        uint32_t src_rg_id = (iter_b + iter_src_c*(a*b/8)) % 8;
        uint32_t dst_rg_id = (iter_b + iter_dst_c*(a*b/8)) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_c * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_c * 1 * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_to_all_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*b*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_c=i/(b*b*(a/8));
        uint32_t remain_c=i-iter_c*b*b*(a/8);
        uint32_t iter_a=remain_c/(b*b);
        uint32_t remain_a=remain_c-iter_a*b*b;
        uint32_t iter_src_b=remain_a/(b);
        uint32_t iter_dst_b=remain_a%(b);

        uint32_t src_rank_id=iter_a/8 + iter_src_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rank_id=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
        uint32_t src_rg_id = (iter_a + iter_src_b*(a/8) + iter_c*(a*b/8)) % 8;
        uint32_t dst_rg_id = (iter_a + iter_dst_b*(a/8) + iter_c*(a*b/8)) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_b * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * 1 * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_to_all_y_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c * (b/(8/a)) * (b/(8/a));
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_c=i / ((b/(8/a)) * (b/(8/a)));
        uint32_t remain_c=i-iter_c*((b/(8/a)) * (b/(8/a)));
        uint32_t iter_src_b=remain_c / (b/(8/a));
        uint32_t iter_dst_b=remain_c % (b/(8/a));

        uint32_t src_rank_id = iter_c*a*b/64 + iter_src_b/8;
        uint32_t dst_rank_id = iter_c*a*b/64 + iter_dst_b/8;

        uint32_t src_rg_id = (iter_c*a*b/8 + iter_src_b) % 8;
        uint32_t dst_rg_id = (iter_c*a*b/8 + iter_dst_b) % 8;
        
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_b * (8/a) * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * (8/a) * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, a);
    }
    return 0;
}

void *thread_all_to_all_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    /* uint32_t total_iter_num=c*b*(a/8)*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    } */

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*b*(a/8)*(a/8);
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t iter_c=i/(b*(a/8)*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8)*(a/8);
        uint32_t iter_b=remain_c/((a/8)*(a/8));
        uint32_t remain_b=remain_c-iter_b*(a/8)*(a/8);

        uint32_t iter_src_a=remain_b/(a/8);
        uint32_t iter_dst_a=remain_b%(a/8);

        uint32_t src_rank_id=iter_src_a/8 + iter_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;

        uint32_t src_rg_id = (iter_src_a + iter_b*(a/8) + iter_c*(a*b/8)) % 8;
        uint32_t dst_rg_id = (iter_dst_a + iter_b*(a/8) + iter_c*(a*b/8)) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        src_start_offset_iter = src_start_offset + iter_dst_a * 8 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_a * 8 * dpu_byte_length;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_to_all_x_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b/(8/a);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t src_dst_rg_id=i%8;
        uint32_t src_dst_rank_id=i/8;

        hw_dpu_rank_allocation_parameters_t params_src_dst = _this_params(comm_dpu_set->list.ranks[src_dst_rank_id]->description);
        uint8_t *rank_base_address_src_dst = params_src_dst->ptr_region;

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;

        params_src_dst->translate.trans_all_to_all_rg_24(rank_base_address_src_dst, rank_base_address_src_dst, src_dst_rg_id, src_dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, a);
    }
    return 0;
}



static dpu_rank_status_e
hw_all_to_all_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_x_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_x_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_all_to_all_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){
    
    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num]; //={0, rank, start_offset, dpu_byte_length, a, b, c}
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_xz_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_all_to_all_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){
    
    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num]; //={0, rank, start_offset, dpu_byte_length, a, b, c}
    int status;
    pthread_t array_thread[thread_num];

    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_y_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_y_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_all_to_all_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){
    
    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num]; //={0, rank, start_offset, dpu_byte_length, a, b, c}
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_z_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_z_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

typedef struct {
    uint32_t p_thread_id;
    struct dpu_set_t *p_comm_dpu_set;
    uint32_t p_src_start_offset;
    uint32_t p_dst_start_offset;
    uint32_t p_dpu_byte_length;
    uint32_t p_total_length;
    uint32_t p_comm_type;
    uint32_t p_row_length;
    uint32_t p_communication_buffer_offset;
    uint32_t size;
    uint32_t thread_num;
    uint32_t reduce_type;
    void** p_host_buffer;
    uint32_t p_target_dpu_index;
    uint32_t p_num_thread;

    uint32_t dimension;
    uint32_t* axis_len;
    uint32_t* comm_axis;

}st_thread_parameter;

void *thread_all_to_all_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t i=0; i<dimension; i++){
        total_iter_num *= axis_len[i];
        if(comm_axis[i] == 1) {
            total_axis_product *= axis_len[i];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8;
    total_iter_num *= total_axis_product;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain / total_axis_product;
        cur_iter_dst = cur_remain % total_axis_product;

        if(!comm_type){
            src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id=0; 
        uint32_t dst_rank_id=0; 
        uint32_t src_rg_id=0;
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product=1;
        for(uint32_t dim=0; dim < dimension; dim++){
            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];
            src_rank_id += iter_src[dim]*temp_total_product/8;
            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;
            dst_rg_id += iter_dst[dim]*temp_total_product;
        }
        src_rg_id = src_rg_id % 8;
        dst_rg_id = dst_rg_id % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;

        params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread);
        
    }
    return 0;
}

//for when the product of the length of the first 2 dimensions is a muliutple of 8
void *thread_all_to_all_24_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 1) total_axis_product *= (axis_len[0]*axis_len[1]/8);
            else if(dim>1) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    total_iter_num *= total_axis_product;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //set src & dst rgs and their offsets
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 1) cur_iter_num /= (axis_len[0]*axis_len[1]/8);
                else if (dim>1) cur_iter_num /= axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain / total_axis_product;
        cur_iter_dst = cur_remain % total_axis_product;

        //depends on how many dpus related to the given comm pattern are in the src rg
        if(!comm_type){
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * axis_len[0] * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * axis_len[0] * dpu_byte_length;
            }
        }
        else{
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * (8/axis_len[0]) * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * (8/axis_len[0]) * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
            }
        }

        for(uint32_t dim = 1; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (axis_len[0]*axis_len[1]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[0]*axis_len[1]/8);
                    cur_iter_src = cur_iter_src / (axis_len[0]*axis_len[1]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[0]*axis_len[1]/8);
                }
                else if(dim>1){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id=0; 
        uint32_t dst_rank_id=0; 
        uint32_t src_rg_id=0;
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product=1;
        for(uint32_t dim=1; dim < dimension; dim++){
            if(dim == 2) temp_total_product *= (axis_len[0]*axis_len[1]/8);
            else if(dim>2) temp_total_product *= axis_len[dim-1];
            src_rank_id += iter_src[dim]*temp_total_product/8;
            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;
            dst_rg_id += iter_dst[dim]*temp_total_product;
        }
        src_rg_id = src_rg_id % 8;
        dst_rg_id = dst_rg_id % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;

        if(comm_axis[0] == comm_axis[1]) params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread);
        else params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, axis_len[0]);// num_inter_thread, thread_id%num_inter_thread);
        
    }
    return 0;
}

//for when axis_len[0] = 2 and axis_len[1] = 2
void *thread_all_to_all_22_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 2) total_axis_product *= (4 * axis_len[2]/8);
            else if(dim>2) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    total_iter_num *= total_axis_product;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //set src & dst rgs and their offsets
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 2) cur_iter_num /= (4*axis_len[2]/8);
                else if (dim>2) cur_iter_num /= axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain / total_axis_product;
        cur_iter_dst = cur_remain % total_axis_product;

        //depends on how many dpus related to the given comm pattern are in the src rg
        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
                }
            }
        }

        for(uint32_t dim = 2; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==2){
                    iter_src[dim] = cur_iter_src % (4*axis_len[2]/8);
                    iter_dst[dim] = cur_iter_dst % (4*axis_len[2]/8);
                    cur_iter_src = cur_iter_src / (4*axis_len[2]/8);
                    cur_iter_dst = cur_iter_dst / (4*axis_len[2]/8);
                }
                else if(dim>2){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id=0; 
        uint32_t dst_rank_id=0; 
        uint32_t src_rg_id=0;
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product=1;
        for(uint32_t dim=2; dim < dimension; dim++){
            if(dim == 3) temp_total_product *= (4*axis_len[2]/8);
            else if(dim>3) temp_total_product *= axis_len[dim-1];
            src_rank_id += iter_src[dim]*temp_total_product/8;
            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;
            dst_rg_id += iter_dst[dim]*temp_total_product;
        }
        src_rg_id = src_rg_id % 8;
        dst_rg_id = dst_rg_id % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        uint8_t *rank_base_address_src=params_src->ptr_region;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;

        
        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, 4);// num_inter_thread, thread_id%num_inter_thread);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_to_all_rg_22(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset);
                else params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, 2);// num_inter_thread, thread_id%num_inter_thread);
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, 2);// num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_all_to_all_rg_22(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_to_all_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, 4);// num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_all_to_all_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, num_inter_thread, thread_id%num_inter_thread);
            }
        }
    }
    return 0;
}


static dpu_rank_status_e
hw_all_to_all_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis){
    
    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        
        if(axis_len[0] >=8) pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_rns, (void *) &thread_params[iter_thread]);
        else if(axis_len[0]*axis_len[1] >= 8) pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_24_rns, (void *) &thread_params[iter_thread]);
        else pthread_create(&array_thread[iter_thread], NULL, thread_all_to_all_22_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}


void *thread_reduce_scatter_cpu_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;

    /* uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    } */

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/(thread_num/num_inter_thread);
    uint32_t remainder=total_iter_num%(thread_num/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){

        uint32_t i = k * (a/8);

        uint32_t iter_c=i/(b*(a/8)*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8)*(a/8);
        uint32_t iter_b=remain_c/((a/8)*(a/8));
        uint32_t remain_b=remain_c-iter_b*(a/8)*(a/8);
        uint32_t iter_src_a=remain_b%(a/8);
        uint32_t iter_dst_a=remain_b/(a/8);
        uint32_t num_iter_src = (a/8);
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;
        for(uint32_t j=0; j<(a/8); j++){
            uint32_t src_rank_id= j/8 + iter_b*a/64 + iter_c*a*b/64;
            src_rg_id[j]= (j + iter_b*a/8 + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[iter_b*a/64 + iter_c*a*b/64] ->description);
        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_dst_a + iter_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;

        src_start_offset_iter = src_start_offset + iter_dst_a * 8 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_a * 8 * dpu_byte_length;

        params_src->translate.trans_reduce_scatter_cpu_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, size, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }

    return 0;
}

void *thread_reduce_scatter_cpu_x_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;

    uint32_t total_iter_num=c*b/(8/a);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){
        uint32_t src_dst_rg_id=k%8;
        uint32_t src_dst_rank_id=k/8;

        hw_dpu_rank_allocation_parameters_t params_src_dst = _this_params(comm_dpu_set->list.ranks[src_dst_rank_id]->description);
        uint8_t *rank_base_address_src_dst = params_src_dst->ptr_region;

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;

        uint32_t num_iter_dst=1;
        void* rank_base_address_dst_array[1];
        uint32_t dst_rg_id_array[1];
        rank_base_address_dst_array[0]=rank_base_address_src_dst;
        dst_rg_id_array[0]=src_dst_rg_id;

        params_src_dst->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_src_dst, rank_base_address_dst_array, src_dst_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, a, size);
    }
    return 0;
}

static dpu_rank_status_e
hw_reduce_scatter_cpu_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].size = size;
        thread_params[iter_thread].thread_num = thread_num;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_cpu_x_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_cpu_x_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

void *thread_reduce_scatter_cpu_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;


    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k=k+1){

        uint32_t i = k*b;

        uint32_t iter_c=i/(b*b*(a/8));
        uint32_t remain_c=i-iter_c*b*b*(a/8);
        uint32_t iter_a=remain_c/(b*b);
        uint32_t remain_a=remain_c-iter_a*b*b;
        uint32_t iter_src_b=remain_a%(b);
        uint32_t iter_dst_b=remain_a/(b);

        uint32_t num_iter_src = b;
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;

        for(uint32_t j=0; j<b; j++){
            uint32_t src_rank_id= iter_a/8 + j*a/(64) + iter_c*a*b/64;
            src_rg_id[j]=(iter_a + j*(a/8) + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }

        params_src  = _this_params(comm_dpu_set->list.ranks[iter_a/8 + iter_c*a*b/64] ->description);

        uint32_t dst_rank_id=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_a + iter_dst_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        
        src_start_offset_iter = src_start_offset + iter_dst_b * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * 1 * dpu_byte_length;

        params_src->translate.trans_reduce_scatter_cpu_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, size); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_reduce_scatter_cpu_y_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;
    
    uint32_t total_iter_num=c * (b/(8/a));
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    {
        for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){
            uint32_t iter_c = k / (b/(8/a));
            uint32_t iter_dst_b = k % (b/(8/a));
            uint32_t dst_rank_id = iter_dst_b/8 + iter_c*a*b/64;
            uint32_t dst_rg_id = (iter_dst_b + iter_c*a*b/8) % 8;
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id]->description);
            void *rank_base_address_dst = params_dst->ptr_region;
            
            uint32_t num_iter_src = b / (8/a);
            void* rank_base_address_src[num_iter_src];
            uint32_t src_rg_id[num_iter_src];

            hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[0] ->description);;
            for(uint32_t j=0; j<num_iter_src; j++){
                uint32_t src_rank_id= j/8 + iter_c*a*b/64;
                src_rg_id[j]=(j + iter_c*a*b/8)%8;
                params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
                rank_base_address_src[j]=params_src->ptr_region;
            }
            
            src_start_offset_iter = src_start_offset + iter_dst_b * (8/a) * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset;
            params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_src, a, size);
        }
    }

    return 0;
}

static dpu_rank_status_e
hw_reduce_scatter_cpu_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].size=size;
        thread_params[iter_thread].thread_num = thread_num;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_cpu_y_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_cpu_y_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_reduce_scatter_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            total_axis_product *= axis_len[dim];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=0; dim < dimension; dim++){

            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 0){
                        src_rank_id[j] += (j % (axis_len[0]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]/8) ) * temp_total_product;
                    }
                    else{
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 0) temp_comm_product *= (axis_len[0]/8);
                else if(dim>0) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(!comm_type) params_src->translate.trans_reduce_scatter_cpu_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, num_inter_thread, thread_id%num_inter_thread);
        else params_src->translate.trans_reduce_scatter_cpu_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size);//, num_inter_thread, thread_id%num_inter_thread);
        
    }
    return 0;
}

void *thread_reduce_scatter_24_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 1) total_axis_product *= (axis_len[0]*axis_len[1]/8);
            else if (dim>1) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=1; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 1) cur_iter_num /= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        //depends on how many dpus related to the given comm pattern are in the src rg
        if(!comm_type){
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * axis_len[0] * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * axis_len[0] * dpu_byte_length;
            }
        }
        else{
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * (8/axis_len[0]) * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * (8/axis_len[0]) * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
            }
        }

        for(uint32_t dim = 1; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (axis_len[0]*axis_len[1]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[0]*axis_len[1]/8);
                    cur_iter_src = cur_iter_src / (axis_len[0]*axis_len[1]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[0]*axis_len[1]/8);
                }
                else if(dim>1){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=1; dim < dimension; dim++){

            if(dim == 2) temp_total_product *= (axis_len[0]*axis_len[1]/8);
            else if(dim>2) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 1){
                        src_rank_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product;
                    }
                    else if (dim>1){
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 1) temp_comm_product *= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(comm_axis[0] == comm_axis[1]){
            if(!comm_type) params_src->translate.trans_reduce_scatter_cpu_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, num_inter_thread, thread_id%num_inter_thread);
            else params_src->translate.trans_reduce_scatter_cpu_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size);//, num_inter_thread, thread_id%num_inter_thread);
        }
        else params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, axis_len[0], size);
        
    }
    return 0;
}

void *thread_reduce_scatter_22_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 2) total_axis_product *= (4 * axis_len[2]/8);
            else if (dim>2) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=2; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 2) cur_iter_num /= (4*axis_len[2]/8);
                else if(dim>2) cur_iter_num/=axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        //depends on how many dpus related to the given comm pattern are in the src rg
        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
                }
            }
        }

        for(uint32_t dim = 2; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (4*axis_len[2]/8);
                    iter_dst[dim] = cur_iter_dst % (4*axis_len[2]/8);
                    cur_iter_src = cur_iter_src / (4*axis_len[2]/8);
                    cur_iter_dst = cur_iter_dst / (4*axis_len[2]/8);
                }
                else if(dim>1){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=2; dim < dimension; dim++){

            if(dim == 3) temp_total_product *= (4*axis_len[2]/8);
            else if(dim>3) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 1){
                        src_rank_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product;
                    }
                    else if (dim>1){
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 2) temp_comm_product *= (4*axis_len[2]/8);
                else if(dim>2) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_reduce_scatter_cpu_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4, size);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_reduce_scatter_cpu_rg_22(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product, size);
                else params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2, size);
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2, size);
                else params_src->translate.trans_reduce_scatter_cpu_rg_22(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product, size);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_reduce_scatter_cpu_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4, size);
                else params_src->translate.trans_reduce_scatter_cpu_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size);//, num_inter_thread, thread_id%num_inter_thread);
            }
        }
    }
    return 0;
}

static dpu_rank_status_e
hw_reduce_scatter_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size){
    
    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        thread_params[iter_thread].size = size;
        
        if(axis_len[0] >=8) pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_rns, (void *) &thread_params[iter_thread]);
        else if(axis_len[0]*axis_len[1] >= 8) pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_24_rns, (void *) &thread_params[iter_thread]);
        else pthread_create(&array_thread[iter_thread], NULL, thread_reduce_scatter_22_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_all_reduce_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;
    uint32_t reduce_type = each_thread_all_to_all_x_parameter->reduce_type;


    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/(thread_num/num_inter_thread);
    uint32_t remainder=total_iter_num%(thread_num/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){

        uint32_t i = k*(a/8);

        uint32_t iter_c=i/(b*(a/8)*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8)*(a/8);
        uint32_t iter_b=remain_c/((a/8)*(a/8));
        uint32_t remain_b=remain_c-iter_b*(a/8)*(a/8);
        uint32_t iter_src_a=remain_b%(a/8);
        uint32_t iter_dst_a=remain_b/(a/8);

        uint32_t num_iter_src = (a/8);
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;
        for(uint32_t j=0; j<(a/8); j++){
            uint32_t src_rank_id= j/8 + iter_b*a/64 + iter_c*a*b/64;
            src_rg_id[j]= (j + iter_b*a/8 + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[iter_b*a/64 + iter_c*a*b/64] ->description);

        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id= (iter_dst_a + iter_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        

        src_start_offset_iter = src_start_offset + iter_dst_a * 8 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_a * 8 * dpu_byte_length;

        params_src->translate.trans_all_reduce_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, iter_dst_a, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, size, reduce_type, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_reduce_x_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;

    uint32_t total_iter_num=c*b/(8/a);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){
        uint32_t src_dst_rg_id=k%8;
        uint32_t src_dst_rank_id=k/8;

        hw_dpu_rank_allocation_parameters_t params_src_dst = _this_params(comm_dpu_set->list.ranks[src_dst_rank_id]->description);
        uint8_t *rank_base_address_src_dst = params_src_dst->ptr_region;

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;

        uint32_t num_iter_dst=1;
        void* rank_base_address_dst_array[1];
        uint32_t dst_rg_id_array[1];
        rank_base_address_dst_array[0]=rank_base_address_src_dst;
        dst_rg_id_array[0]=src_dst_rg_id;

        params_src_dst->translate.trans_all_reduce_rg_24(rank_base_address_src_dst, rank_base_address_dst_array, src_dst_rg_id, dst_rg_id_array, 0, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, a, size);
    }
    return 0;
}

static dpu_rank_status_e
hw_all_reduce_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].size = size;
        thread_params[iter_thread].thread_num = thread_num;
        thread_params[iter_thread].reduce_type = reduce_type;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_x_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_x_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_all_reduce_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;
    uint32_t reduce_type = each_thread_all_to_all_x_parameter->reduce_type;
    
    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){

        uint32_t i = k*b;

        uint32_t iter_c=i/(b*b*(a/8));
        uint32_t remain_c=i-iter_c*b*b*(a/8);
        uint32_t iter_a=remain_c/(b*b);
        uint32_t remain_a=remain_c-iter_a*b*b;
        uint32_t iter_src_b=remain_a%b;
        uint32_t iter_dst_b=remain_a/b;

        uint32_t num_iter_src = b;
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;

        for(uint32_t j=0; j<b; j++){
            uint32_t src_rank_id= iter_a/8 + j*a/64 + iter_c*a*b/64;
            src_rg_id[j]=(iter_a + j*a/8 + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }

        params_src  = _this_params(comm_dpu_set->list.ranks[iter_a/8 + iter_c*a*b/64] ->description);

        uint32_t dst_rank_id=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_a + iter_dst_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        

        src_start_offset_iter = src_start_offset + iter_dst_b * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * 1 * dpu_byte_length;

        params_src->translate.trans_all_reduce_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, iter_dst_b, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, size, reduce_type); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_reduce_y_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t size = each_thread_all_to_all_x_parameter->size;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->thread_num;

    uint32_t total_iter_num=c;
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    if(thread_id<c){
        for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){
            uint32_t iter_c = k;
            uint32_t num_iter_src = b / (8/a);
            void* rank_base_address_src[num_iter_src];
            uint32_t src_rg_id[num_iter_src];

            hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[0] ->description);;
            for(uint32_t j=0; j<num_iter_src; j++){
                uint32_t src_rank_id= j/8 + iter_c*a*b/64;
                src_rg_id[j]=(j + iter_c*a*b/8)%8;
                params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
                rank_base_address_src[j]=params_src->ptr_region;
            }
            
            src_start_offset_iter = src_start_offset;
            dst_start_offset_iter = dst_start_offset;
            params_src->translate.trans_all_reduce_rg_24(rank_base_address_src[0], rank_base_address_src, src_rg_id[0], src_rg_id, 0, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_src, a, size);
        }
    }
    
    return 0;
}

static dpu_rank_status_e
hw_all_reduce_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].size = size;
        thread_params[iter_thread].thread_num = thread_num;
        thread_params[iter_thread].reduce_type = reduce_type;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_y_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_y_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_all_reduce_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;
    uint32_t reduce_type = each_thread_comm_parameter->reduce_type;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            total_axis_product *= axis_len[dim];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8;

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=0; dim < dimension; dim++){

            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 0){
                        src_rank_id[j] += (j % (axis_len[0]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]/8) ) * temp_total_product;
                    }
                    else{
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 0) temp_comm_product *= (axis_len[0]/8);
                else if(dim>0) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(!comm_type) params_src->translate.trans_all_reduce_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type, num_inter_thread, thread_id%num_inter_thread);
        else params_src->translate.trans_all_reduce_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type);//, num_inter_thread, thread_id%num_inter_thread);
    }
    return 0;
}

void *thread_all_reduce_rns_24(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;
    uint32_t reduce_type = each_thread_comm_parameter->reduce_type;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 1) total_axis_product *= (axis_len[0]*axis_len[1]/8);
            else if (dim>1) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=1; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 1) cur_iter_num /= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) cur_iter_num/=axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * axis_len[0] * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * axis_len[0] * dpu_byte_length;
            }
        }
        else{
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * (8/axis_len[0]) * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * (8/axis_len[0]) * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
            }
        }

        for(uint32_t dim = 1; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (axis_len[0]*axis_len[1]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[0]*axis_len[1]/8);
                    cur_iter_src = cur_iter_src / (axis_len[0]*axis_len[1]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[0]*axis_len[1]/8);
                }
                else if(dim>1){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=1; dim < dimension; dim++){

            if(dim == 2) temp_total_product *= (axis_len[0]*axis_len[1]/8);
            else if(dim>2) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 1){
                        src_rank_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product;
                    }
                    else if (dim>1){
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 1) temp_comm_product *= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(comm_axis[0] == comm_axis[1]){
            if(!comm_type) params_src->translate.trans_all_reduce_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type, num_inter_thread, thread_id%num_inter_thread);
            else params_src->translate.trans_all_reduce_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type);//, num_inter_thread, thread_id%num_inter_thread);
        }
        else params_src->translate.trans_all_reduce_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, axis_len[0], size);
    }
    return 0;
}

void *thread_all_reduce_rns_22(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;
    uint32_t reduce_type = each_thread_comm_parameter->reduce_type;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 2) total_axis_product *= (4 * axis_len[2]/8);
            else if (dim>2) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8;

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=2; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 2) cur_iter_num /= (4*axis_len[2]/8);
                else if(dim>2) cur_iter_num/=axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
                }
            }
        }

        for(uint32_t dim = 2; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (4*axis_len[2]/8);
                    iter_dst[dim] = cur_iter_dst % (4*axis_len[2]/8);
                    cur_iter_src = cur_iter_src / (4*axis_len[2]/8);
                    cur_iter_dst = cur_iter_dst / (4*axis_len[2]/8);
                }
                else if(dim>1){
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=2; dim < dimension; dim++){

            if(dim == 3) temp_total_product *= (4*axis_len[2]/8);
            else if(dim>3) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 1){
                        src_rank_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product;
                    }
                    else if (dim>1){
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 2) temp_comm_product *= (4*axis_len[2]/8);
                else if(dim>2) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_reduce_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type, num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_all_reduce_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4, size);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_reduce_rg_22(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product, size);
                else params_src->translate.trans_all_reduce_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2, size);
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_reduce_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2, size);
                else params_src->translate.trans_all_reduce_rg_22(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product, size);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_reduce_rg_24(rank_base_address_dst, rank_base_address_src, dst_rg_id, src_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4, size);
                else params_src->translate.trans_all_reduce_y_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, cur_remain / total_axis_product, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, size, reduce_type);//, num_inter_thread, thread_id%num_inter_thread);
            }
        }

    }
    return 0;
}

static dpu_rank_status_e
hw_all_reduce_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, uint32_t reduce_type){
    
    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        thread_params[iter_thread].size = size;
        thread_params[iter_thread].reduce_type = reduce_type;
        
        if(axis_len[0] >=8) pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_rns, (void *) &thread_params[iter_thread]);
        else if(axis_len[0]*axis_len[1] >= 8) pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_rns_24, (void *) &thread_params[iter_thread]);
        else pthread_create(&array_thread[iter_thread], NULL, thread_all_reduce_rns_22, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_gather_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;

    void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k = k + 1){

        uint32_t i = k * (a/8);

        uint32_t iter_c=i/(b*(a/8)*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8)*(a/8);
        uint32_t iter_b=remain_c/((a/8)*(a/8));
        uint32_t remain_b=remain_c-iter_b*(a/8)*(a/8);
        uint32_t iter_src_a=remain_b%(a/8);
        uint32_t iter_dst_a=remain_b/(a/8);
        uint32_t num_iter_src = (a/8);
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        
        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_dst_a + iter_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;

        
        iter_src_a+=0;

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;
        
        {params_dst->translate.trans_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, host_buffer, k);}
        
    }
    return 0;
}

static dpu_rank_status_e
hw_gather_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer){
    
    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=host_buffer;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_gather_x_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}


void *thread_gather_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;

    //void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->p_num_thread;
    
    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k++){
        uint32_t i = k*b;

        uint32_t iter_c=i/(b*b*(a/8));
        uint32_t remain_c=i-iter_c*b*b*(a/8);
        uint32_t iter_a=remain_c/(b*b);
        uint32_t remain_a=remain_c-iter_a*b*b;
        uint32_t iter_src_b=remain_a%(b);
        uint32_t iter_dst_b=remain_a/(b);

        uint32_t num_iter_src = b;
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;
        for(uint32_t j=0; j<b; j++){
            uint32_t src_rank_id= iter_a/8 + j*a/(64) + iter_c*a*b/64;
            src_rg_id[j]=(iter_a + j*(a/8) + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[iter_a/8 + iter_c*a*b/64] ->description);

        uint32_t dst_rank_id=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_a + iter_dst_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        
        src_start_offset_iter = src_start_offset + iter_dst_b * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * 1 * dpu_byte_length;

        dst_rg_id+=0; rank_base_address_dst+=0; src_start_offset_iter+=0; src_rg_id[0]+=0; communication_buffer_offset+=0; alltoall_comm_type+=0; rank_base_address_src[0]+=0; dst_start_offset_iter+=0; 
        //params_src->translate.trans_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, host_buffer, k);
    }
    return 0;
}

static dpu_rank_status_e
hw_gather_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=(void **)host_buffer;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_gather_y_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}

void *thread_gather_z_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t target_dpu_index=each_thread_all_to_all_x_parameter->p_target_dpu_index;

    //void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->p_num_thread;
    
    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst=c;

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_b=i/(c*(a/8));
        uint32_t remain_b=i-iter_b*c*(a/8);
        uint32_t iter_a=remain_b/(c);
        uint32_t iter_src_c=remain_b-iter_a*(c);

        src_rank_id=iter_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        src_rg_id = (iter_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;
        uint32_t cal_target_dpu_rg=0;
        uint32_t cal_x_rg_index=0;

        for(uint32_t iter_dst_c=0; iter_dst_c<c; iter_dst_c++){
            dst_rank_id_array[iter_dst_c]=iter_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
            dst_rg_id_array[iter_dst_c] = (iter_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;

            if((target_dpu_index)==iter_dst_c){
                cal_target_dpu_rg=iter_dst_c;
                cal_x_rg_index=0;
            }

            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_c]] ->description);
            rank_base_address_dst_array[iter_dst_c]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_c * 1 * dpu_byte_length;
        }
        


        cal_target_dpu_rg+=0; cal_x_rg_index+=0; dst_start_offset_iter_array+=0; dst_rg_id_array[0]+=0; rank_base_address_dst_array[0]+=0; src_start_offset_iter+=0; src_rg_id+=0; communication_buffer_offset+=0; alltoall_comm_type+=0; rank_base_address_src+=0;
        //params_src->translate.trans_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, cal_target_dpu_rg, cal_x_rg_index, temp_host_buffer);
    }
    return 0;
}

static dpu_rank_status_e
hw_gather_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void **host_buffer){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=(void **)host_buffer;
        thread_params[iter_thread].p_target_dpu_index=target_dpu_index;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_gather_z_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

void *thread_gather_xz_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t target_dpu_index=each_thread_all_to_all_x_parameter->p_target_dpu_index;

    //void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t thread_num = each_thread_all_to_all_x_parameter->p_num_thread;
    
    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/thread_num;
    uint32_t remainder=total_iter_num%thread_num;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst=c*(a/8);

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_b=i/(c*(a/8));
        uint32_t iter_src_ac=i-iter_b*c*(a/8);

        uint32_t iter_src_a=iter_src_ac%(a/8);
        uint32_t iter_src_c=iter_src_ac/(a/8);

        src_rank_id=iter_src_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        src_rg_id = (iter_src_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;
        uint32_t cal_target_dpu_rg=0;
        uint32_t cal_x_rg_index=0;

        for(uint32_t iter_dst_ac=0; iter_dst_ac<c*(a/8); iter_dst_ac++){
            uint32_t iter_dst_a=iter_dst_ac%(a/8);
            uint32_t iter_dst_c=iter_dst_ac/(a/8);

            dst_rank_id_array[iter_dst_ac]=iter_dst_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
            dst_rg_id_array[iter_dst_ac] = (iter_dst_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;
            if((target_dpu_index/8)==iter_dst_ac){
                cal_target_dpu_rg=iter_dst_ac;
                cal_x_rg_index=target_dpu_index%8;
            }

            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_ac]] ->description);
            rank_base_address_dst_array[iter_dst_ac]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_ac * 8 * dpu_byte_length;
        }

        cal_target_dpu_rg+=0; cal_x_rg_index+=0; dst_start_offset_iter_array+=0; dst_rg_id_array[0]+=0; rank_base_address_dst_array[0]+=0; src_start_offset_iter+=0; src_rg_id+=0; communication_buffer_offset+=0; alltoall_comm_type+=0; rank_base_address_src+=0;
        //params_src->translate.trans_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, cal_target_dpu_rg, cal_x_rg_index, temp_host_buffer);
    }
    return 0;
}

static dpu_rank_status_e
hw_gather_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void **host_buffer){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=(void **)host_buffer;
        thread_params[iter_thread].p_target_dpu_index=target_dpu_index;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_gather_xz_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

//scatter
void *thread_scatter_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k++){
        uint32_t i = k * (a/8);

        uint32_t iter_c=i/(b*(a/8)*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8)*(a/8);
        uint32_t iter_b=remain_c/((a/8)*(a/8));
        uint32_t remain_b=remain_c-iter_b*(a/8)*(a/8);
        uint32_t iter_src_a=remain_b%(a/8);
        uint32_t iter_dst_a=remain_b/(a/8);

        uint32_t num_iter_src = (a/8);
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];

        uint32_t dst_rank_id=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_dst_a + iter_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;
        iter_src_a+=0;

        params_dst->translate.trans_scatter_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, host_buffer, k);
    }
    return 0;
}

static dpu_rank_status_e
hw_scatter_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=host_buffer;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_scatter_x_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}

void *thread_scatter_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    //void **host_buffer=each_thread_all_to_all_x_parameter->p_host_buffer;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;
    
    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t k=start_point; k<(start_point+(share+remain_iter)); k++){
        uint32_t i = k*b;

        uint32_t iter_c=i/(b*b*(a/8));
        uint32_t remain_c=i-iter_c*b*b*(a/8);
        uint32_t iter_a=remain_c/(b*b);
        uint32_t remain_a=remain_c-iter_a*b*b;
        uint32_t iter_src_b=remain_a%(b);
        uint32_t iter_dst_b=remain_a/(b);

        uint32_t num_iter_src = b;
        void* rank_base_address_src[num_iter_src];
        uint32_t src_rg_id[num_iter_src];
        hw_dpu_rank_allocation_parameters_t params_src;

        for(uint32_t j=0; j<b; j++){
            uint32_t src_rank_id= iter_a/8 + j*a/(64) + iter_c*a*b/64;
            src_rg_id[j]=(iter_a + j*(a/8) + iter_c*a*b/8)%8;
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
            rank_base_address_src[j]=params_src->ptr_region;
        }

        params_src  = _this_params(comm_dpu_set->list.ranks[iter_a/8 + iter_c*a*b/64] ->description);

        uint32_t dst_rank_id=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
        uint32_t dst_rg_id=(iter_a + iter_dst_b*a/8 + iter_c*a*b/8)%8;
        hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        uint8_t *rank_base_address_dst=params_dst->ptr_region;
        
        
        src_start_offset_iter = src_start_offset + iter_dst_b * 1 * dpu_byte_length;
        dst_start_offset_iter = dst_start_offset + iter_src_b * 1 * dpu_byte_length;

        dst_rg_id+=0; rank_base_address_dst+=0; src_start_offset_iter+=0; src_rg_id[0]+=0; communication_buffer_offset+=0; alltoall_comm_type+=0; rank_base_address_src[0]+=0; dst_start_offset_iter+=0;
        //params_src->translate.trans_scatter_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, num_iter_src, alltoall_comm_type, communication_buffer_offset, host_buffer, k);
    }
    return 0;
}

static dpu_rank_status_e
hw_scatter_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void **host_buffer){

    uint32_t thread_num=32;
    st_thread_all_to_all_x_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_host_buffer=(void **)host_buffer;
        thread_params[iter_thread].p_num_thread=thread_num;
        pthread_create(&array_thread[iter_thread], NULL, thread_scatter_y_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}

void *thread_scatter_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    void** host_buffer = each_thread_comm_parameter->p_host_buffer;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            total_axis_product *= axis_len[dim];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8;

    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            src_start_offset_iter = src_start_offset;
            dst_start_offset_iter = dst_start_offset;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=0; dim < dimension; dim++){

            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 0){
                        src_rank_id[j] += (j % (axis_len[0]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]/8) ) * temp_total_product;
                    }
                    else{
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 0) temp_comm_product *= (axis_len[0]/8);
                else if(dim>0) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;

        params_src->translate.trans_scatter_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_axis_product, comm_type, communication_buffer_offset, host_buffer, i);
        
    }
    return 0;
}

static dpu_rank_status_e
hw_scatter_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, void ** host_buffer){

    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        thread_params[iter_thread].p_host_buffer=host_buffer;
        pthread_create(&array_thread[iter_thread], NULL, thread_scatter_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    
    return DPU_RANK_SUCCESS;
}

void *thread_reduce_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t total_length=each_thread_comm_parameter->p_total_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;
    uint32_t size = each_thread_comm_parameter->size;

    void **host_buffer=each_thread_comm_parameter->p_host_buffer;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            total_axis_product *= axis_len[dim];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8;

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread);
    uint32_t remainder=total_iter_num%(num_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id)<remainder){
        remain_iter=1;
        start_point+=(thread_id);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num*total_axis_product;
        cur_remain = i * total_axis_product;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain % total_axis_product;
        cur_iter_dst = cur_remain / total_axis_product;

        if(!comm_type){
            src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        
        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t* src_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rank_id=0; 
        uint32_t* src_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t dst_rg_id=0;
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=0; dim < dimension; dim++){

            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];

            dst_rank_id += iter_dst[dim]*temp_total_product/8;
            dst_rg_id += iter_dst[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    src_rank_id[j] += iter_src[dim] * temp_total_product/8;
                    src_rg_id[j] += iter_src[dim] * temp_total_product;
                }
                else{
                    if(dim == 0){
                        src_rank_id[j] += (j % (axis_len[0]/8) ) * temp_total_product/8;
                        src_rg_id[j] += (j % (axis_len[0]/8) ) * temp_total_product;
                    }
                    else{
                        src_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        src_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 0) temp_comm_product *= (axis_len[0]/8);
                else if(dim>0) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) src_rg_id[j] = src_rg_id[j] % 8;
        dst_rg_id = dst_rg_id % 8;

        void* rank_base_address_src[total_axis_product];
        uint8_t* rank_base_address_dst;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id[j]] ->description);
            rank_base_address_src[j] = params_src->ptr_region;
        }
        params_src  = _this_params(comm_dpu_set->list.ranks[src_rank_id[0]] ->description);
        params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id] ->description);
        rank_base_address_dst=params_dst->ptr_region;


        params_src->translate.trans_reduce_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, total_length, total_axis_product, comm_type, communication_buffer_offset, host_buffer, i, size);

    }
    return 0;
}

static dpu_rank_status_e
hw_reduce_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t total_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, void ** host_buffer){
    
    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_total_length=total_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        thread_params[iter_thread].size = size;
        thread_params[iter_thread].p_host_buffer = host_buffer;
        
        if(axis_len[0] >=8) pthread_create(&array_thread[iter_thread], NULL, thread_reduce_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

//All-Gather
void *thread_all_gather_x_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    /* uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/(num_thread/4);
    uint32_t remainder=total_iter_num%(num_thread/4);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/4);
    if((thread_id/4)<remainder){
        remain_iter=1;
        start_point+=(thread_id/4);
    }
    else{
        start_point+=remainder;
    } */

    uint32_t num_inter_thread = 4;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        //number of destination entangled groups
        uint32_t num_iter_dst=(a/8);

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_c=i/(b*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8);
        uint32_t iter_b=remain_c/((a/8));
        uint32_t iter_src_a=remain_c-iter_b*(a/8);

        src_rank_id=iter_src_a/8 + iter_b*a/64 + iter_c*a*b/64;
        src_rg_id = (iter_src_a + iter_b*(a/8) + iter_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;

        for(uint32_t iter_dst_a=0; iter_dst_a<a/8; iter_dst_a++){
            dst_rank_id_array[iter_dst_a]=iter_dst_a/8 + iter_b*a/64 + iter_c*a*b/64;
            dst_rg_id_array[iter_dst_a] = (iter_dst_a + iter_b*(a/8) + iter_c*(a*b/8)) % 8;
            
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_a]] ->description);
            rank_base_address_dst_array[iter_dst_a]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_a * 8 * dpu_byte_length;
        }

        params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_gather_x_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b/(8/a);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    uint32_t src_start_offset_iter, dst_start_offset_iter;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint32_t src_dst_rg_id=i%8;
        uint32_t src_dst_rank_id=i/8;

        hw_dpu_rank_allocation_parameters_t params_src_dst = _this_params(comm_dpu_set->list.ranks[src_dst_rank_id]->description);
        uint8_t *rank_base_address_src_dst = params_src_dst->ptr_region;

        src_start_offset_iter = src_start_offset;
        dst_start_offset_iter = dst_start_offset;

        uint32_t num_iter_dst=1;
        void* rank_base_address_dst_array[1];
        uint32_t dst_rg_id_array[1];
        rank_base_address_dst_array[0]=rank_base_address_src_dst;
        dst_rg_id_array[0]=src_dst_rg_id;

        params_src_dst->translate.trans_all_gather_rg_24(rank_base_address_src_dst, rank_base_address_dst_array, src_dst_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, a);
    }
    return 0;
}

static dpu_rank_status_e
hw_all_gather_x_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){

    uint32_t num_thread=32;

    st_thread_all_to_all_x_parameter thread_params[32];
    int status;
    pthread_t array_thread[32];
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=num_thread;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_x_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_x_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

void *thread_all_gather_y_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    /* uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    } */

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst=b;

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_c=i/(b*(a/8));
        uint32_t remain_c=i-iter_c*b*(a/8);
        uint32_t iter_a=remain_c/b;
        uint32_t iter_src_b=remain_c-iter_a*b;

        src_rank_id=iter_a/8 + iter_src_b*a/64 + iter_c*a*b/64;
        src_rg_id = (iter_a + iter_src_b*(a/8) + iter_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;

        for(uint32_t iter_dst_b=0; iter_dst_b<b; iter_dst_b++){
            dst_rank_id_array[iter_dst_b]=iter_a/8 + iter_dst_b*a/64 + iter_c*a*b/64;
            dst_rg_id_array[iter_dst_b] = (iter_a + iter_dst_b*(a/8) + iter_c*(a*b/8)) % 8;
            
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_b]] ->description);
            rank_base_address_dst_array[iter_dst_b]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_b * 1 * dpu_byte_length;
        }

        params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, num_inter_thread, thread_id%num_inter_thread); //rank_base_addr, offset, length
    }
    return 0;
}

void *thread_all_gather_y_rns_24(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c * (b/(8/a));
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst= b/(8/a);

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_c=i / (b/(8/a));
        uint32_t iter_src_b = i - iter_c * (b/(8/a));

        src_rank_id = iter_c*a*b/64 + iter_src_b/8;
        src_rg_id = (iter_c*a*b/8 + iter_src_b) % 8;

        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;

        for(uint32_t iter_dst_b=0; iter_dst_b<(b/(8/a)); iter_dst_b++){
            dst_rank_id_array[iter_dst_b]= iter_dst_b/8 + iter_c*a*b/64;
            dst_rg_id_array[iter_dst_b] = (iter_dst_b + iter_c*(a*b/8)) % 8;
            
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_b]] ->description);
            rank_base_address_dst_array[iter_dst_b]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_b * (8/a) * dpu_byte_length;
        }

        params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, a);
    }
    return 0;
}

static dpu_rank_status_e
hw_all_gather_y_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){

    uint32_t num_thread=32;
    st_thread_all_to_all_x_parameter thread_params[num_thread];
    int status;
    pthread_t array_thread[num_thread];
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=num_thread;
        if(a>4){
            pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_y_rns, (void *) &thread_params[iter_thread]);
        }
        else{
            pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_y_rns_24, (void *) &thread_params[iter_thread]);
        }
    }
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

void *thread_all_gather_z_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst=c;

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_b=i/(c*(a/8));
        uint32_t remain_b=i-iter_b*c*(a/8);
        uint32_t iter_a=remain_b/(c);
        uint32_t iter_src_c=remain_b-iter_a*(c);

        src_rank_id=iter_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        src_rg_id = (iter_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;

        for(uint32_t iter_dst_c=0; iter_dst_c<c; iter_dst_c++){
            dst_rank_id_array[iter_dst_c]=iter_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
            dst_rg_id_array[iter_dst_c] = (iter_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;
            
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_c]] ->description);
            rank_base_address_dst_array[iter_dst_c]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_c * 1 * dpu_byte_length;
        }

        params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, 1, 0); //rank_base_addr, offset, length
    }
    return 0;
}

static dpu_rank_status_e
hw_all_gather_z_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){

    uint32_t num_thread=32;
    st_thread_all_to_all_x_parameter thread_params[num_thread];
    int status;
    pthread_t array_thread[num_thread];
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=num_thread;
        pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_z_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }

    return DPU_RANK_SUCCESS;
}

void *thread_all_gather_xz_rns(void *thread_parameter){
    st_thread_all_to_all_x_parameter *each_thread_all_to_all_x_parameter = (st_thread_all_to_all_x_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_all_to_all_x_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_all_to_all_x_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_all_to_all_x_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_all_to_all_x_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_all_to_all_x_parameter->p_dpu_byte_length;
    uint32_t a=each_thread_all_to_all_x_parameter->p_a;
    uint32_t b=each_thread_all_to_all_x_parameter->p_b;
    uint32_t c=each_thread_all_to_all_x_parameter->p_c;
    uint32_t alltoall_comm_type=each_thread_all_to_all_x_parameter->p_alltoall_comm_type;
    uint32_t communication_buffer_offset=each_thread_all_to_all_x_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_all_to_all_x_parameter->p_num_thread;

    uint32_t total_iter_num=c*b*(a/8);
    uint32_t share=total_iter_num/num_thread;
    uint32_t remainder=total_iter_num%num_thread;
    uint32_t remain_iter=0;
    uint32_t start_point = share*thread_id;
    if(thread_id<remainder){
        remain_iter=1;
        start_point+=thread_id;
    }
    else{
        start_point+=remainder;
    }
    
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){
        uint8_t* rank_base_address_src;
        uint32_t src_rank_id;
        uint32_t src_rg_id=0;
        uint32_t src_start_offset_iter;

        uint32_t num_iter_dst=c*(a/8);

        void* rank_base_address_dst_array[num_iter_dst];
        uint32_t dst_rank_id_array[num_iter_dst];
        uint32_t dst_rg_id_array[num_iter_dst];
        uint32_t dst_start_offset_iter_array=0;

        uint32_t iter_b=i/(c*(a/8));
        uint32_t iter_src_ac=i-iter_b*c*(a/8);

        uint32_t iter_src_a=iter_src_ac%(a/8);
        uint32_t iter_src_c=iter_src_ac/(a/8);

        src_rank_id=iter_src_a/8 + iter_b*a/64 + iter_src_c*a*b/64;
        src_rg_id = (iter_src_a + iter_b*(a/8) + iter_src_c*(a*b/8)) % 8;
        hw_dpu_rank_allocation_parameters_t params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;
        src_start_offset_iter = src_start_offset;

        for(uint32_t iter_dst_ac=0; iter_dst_ac<c*(a/8); iter_dst_ac++){
            uint32_t iter_dst_a=iter_dst_ac%(a/8);
            uint32_t iter_dst_c=iter_dst_ac/(a/8);

            dst_rank_id_array[iter_dst_ac]=iter_dst_a/8 + iter_b*a/64 + iter_dst_c*a*b/64;
            dst_rg_id_array[iter_dst_ac] = (iter_dst_a + iter_b*(a/8) + iter_dst_c*(a*b/8)) % 8;
            
            hw_dpu_rank_allocation_parameters_t params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id_array[iter_dst_ac]] ->description);
            rank_base_address_dst_array[iter_dst_ac]=params_dst->ptr_region;
            
            dst_start_offset_iter_array = dst_start_offset + iter_src_ac * 8 * dpu_byte_length;
        }

        params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst_array, src_rg_id, dst_rg_id_array, src_start_offset_iter, dst_start_offset_iter_array, dpu_byte_length, alltoall_comm_type, communication_buffer_offset, num_iter_dst, 1, 0); //rank_base_addr, offset, length
    }
    return 0;
}

static dpu_rank_status_e
hw_all_gather_xz_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset){

    uint32_t num_thread=32;
    st_thread_all_to_all_x_parameter thread_params[num_thread];
    int status;
    pthread_t array_thread[num_thread];
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_a=a;
        thread_params[iter_thread].p_b=b;
        thread_params[iter_thread].p_c=c;
        thread_params[iter_thread].p_alltoall_comm_type=alltoall_comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=num_thread;
        pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_xz_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<num_thread; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

void *thread_all_gather_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            total_axis_product *= axis_len[dim];
        }
    }
    if(comm_type == 0) total_axis_product /= 8;

    total_iter_num /=8; 

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=0; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 0) cur_iter_num /= (axis_len[0]/8);
                else cur_iter_num/=axis_len[dim];
                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain;
        cur_iter_dst = cur_remain;

        if(!comm_type){
            src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
        }
        else{
            src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
            dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
        }

        for(uint32_t dim = 0; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==0){
                    iter_src[dim] = cur_iter_src % (axis_len[dim]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[dim]/8);
                    cur_iter_src = cur_iter_src / (axis_len[dim]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[dim]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id = 0;
        uint32_t* dst_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t src_rg_id = 0;
        uint32_t* dst_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=0; dim < dimension; dim++){

            if(dim == 1) temp_total_product *= (axis_len[0]/8);
            else if(dim>1) temp_total_product *= axis_len[dim-1];

            src_rank_id += iter_src[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    dst_rank_id[j] += iter_dst[dim] * temp_total_product/8;
                    dst_rg_id[j] += iter_dst[dim] * temp_total_product;
                }
                else{
                    if(dim == 0){
                        dst_rank_id[j] += (j % (axis_len[0]/8) ) * temp_total_product/8;
                        dst_rg_id[j] += (j % (axis_len[0]/8) ) * temp_total_product;
                    }
                    else{
                        dst_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        dst_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 0) temp_comm_product *= (axis_len[0]/8);
                else if(dim>0) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) dst_rg_id[j] = dst_rg_id[j] % 8;
        src_rg_id = src_rg_id % 8;

        void* rank_base_address_dst[total_axis_product];
        uint8_t* rank_base_address_src;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id[j]] ->description);
            rank_base_address_dst[j] = params_dst->ptr_region;
        }
        params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;

        params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, num_inter_thread, thread_id%num_inter_thread);
    }
    return 0;
}

void *thread_all_gather_24_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 1) total_axis_product *= (axis_len[0]*axis_len[1]/8);
            else if (dim>1) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8; 

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=1; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 1) cur_iter_num /= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) cur_iter_num/=axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain;
        cur_iter_dst = cur_remain;

        if(!comm_type){
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * axis_len[0] * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * axis_len[0] * dpu_byte_length;
            }
        }
        else{
            if(comm_axis[1]==1){
                src_start_offset_iter = src_start_offset + cur_iter_dst * (8/axis_len[0]) * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * (8/axis_len[0]) * dpu_byte_length;
            }
            else {
                src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
            }
        }

        for(uint32_t dim = 1; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==1){
                    iter_src[dim] = cur_iter_src % (axis_len[0]*axis_len[1]/8);
                    iter_dst[dim] = cur_iter_dst % (axis_len[0]*axis_len[1]/8);
                    cur_iter_src = cur_iter_src / (axis_len[0]*axis_len[1]/8);
                    cur_iter_dst = cur_iter_dst / (axis_len[0]*axis_len[1]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id = 0;
        uint32_t* dst_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t src_rg_id = 0;
        uint32_t* dst_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=1; dim < dimension; dim++){

            if(dim == 2) temp_total_product *= (axis_len[0]*axis_len[1]/8);
            else if(dim>2) temp_total_product *= axis_len[dim-1];

            src_rank_id += iter_src[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    dst_rank_id[j] += iter_dst[dim] * temp_total_product/8;
                    dst_rg_id[j] += iter_dst[dim] * temp_total_product;
                }
                else{
                    if(dim == 1){
                        dst_rank_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product/8;
                        dst_rg_id[j] += (j % (axis_len[0]*axis_len[1]/8) ) * temp_total_product;
                    }
                    else if (dim>1){
                        dst_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        dst_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 1) temp_comm_product *= (axis_len[0]*axis_len[1]/8);
                else if(dim>1) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) dst_rg_id[j] = dst_rg_id[j] % 8;
        src_rg_id = src_rg_id % 8;

        void* rank_base_address_dst[total_axis_product];
        uint8_t* rank_base_address_src;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id[j]] ->description);
            rank_base_address_dst[j] = params_dst->ptr_region;
        }
        params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;

        if(comm_axis[0] == comm_axis[1]) params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, num_inter_thread, thread_id%num_inter_thread);
        else params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, axis_len[0]);
    }
    return 0;
}

void *thread_all_gather_22_rns(void *thread_parameter){
    st_thread_parameter *each_thread_comm_parameter = (st_thread_parameter *)thread_parameter;
    uint32_t thread_id = each_thread_comm_parameter->p_thread_id;
    struct dpu_set_t *comm_dpu_set=each_thread_comm_parameter->p_comm_dpu_set;
    uint32_t src_start_offset=each_thread_comm_parameter->p_src_start_offset;
    uint32_t dst_start_offset=each_thread_comm_parameter->p_dst_start_offset;
    uint32_t dpu_byte_length=each_thread_comm_parameter->p_dpu_byte_length;
    uint32_t comm_type=each_thread_comm_parameter->p_comm_type;
    uint32_t communication_buffer_offset=each_thread_comm_parameter->p_communication_buffer_offset;
    uint32_t num_thread=each_thread_comm_parameter->p_num_thread;

    uint32_t dimension = each_thread_comm_parameter->dimension;
    uint32_t* axis_len = each_thread_comm_parameter->axis_len;
    uint32_t* comm_axis = each_thread_comm_parameter->comm_axis;

    uint32_t num_inter_thread = 1;

    uint32_t total_iter_num=1;
    uint32_t total_axis_product=1;

    for(uint32_t dim=0; dim<dimension; dim++){
        total_iter_num *= axis_len[dim];
        if(comm_axis[dim] == 1) {
            if(dim == 2) total_axis_product *= (4 * axis_len[2]/8);
            else if (dim>2) total_axis_product *= axis_len[dim];
        }
    }

    total_iter_num /=8; 

    //ditribute workload among the threads
    uint32_t share=total_iter_num/(num_thread/num_inter_thread);
    uint32_t remainder=total_iter_num%(num_thread/num_inter_thread);
    uint32_t remain_iter=0;
    uint32_t start_point = share*(thread_id/num_inter_thread);
    uint32_t src_start_offset_iter, dst_start_offset_iter;

    if((thread_id/num_inter_thread)<remainder){
        remain_iter=1;
        start_point+=(thread_id/num_inter_thread);
    }
    else{
        start_point+=remainder;
    }

    uint32_t* iter_src = calloc(dimension, sizeof(uint32_t));
    uint32_t* iter_dst = calloc(dimension, sizeof(uint32_t));
    uint32_t cur_iter_num, cur_remain, cur_iter_src, cur_iter_dst;

    //for each thread, set src rotate groups, dst rotate groups and offset of src and target data 
    for(uint32_t i=start_point; i<(start_point+(share+remain_iter)); i++){

        cur_iter_num = total_iter_num;
        cur_remain = i;
        for(int dim = (int)dimension-1; dim>=2; dim--){
            if(comm_axis[dim] == 0){
                if(dim == 2) cur_iter_num /= (4*axis_len[2]/8);
                else if(dim>2) cur_iter_num/=axis_len[dim];

                iter_src[dim] = cur_remain / cur_iter_num;
                iter_dst[dim] = cur_remain / cur_iter_num;
                cur_remain -= iter_src[dim] * cur_iter_num;
            }
        }

        cur_iter_src = cur_remain;
        cur_iter_dst = cur_remain;

        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 8 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 8 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 4 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 4 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
            }
            else {
                if(comm_axis[2]==1){
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 2 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 2 * dpu_byte_length;
                }
                else{
                    src_start_offset_iter = src_start_offset + cur_iter_dst * 1 * dpu_byte_length;
                    dst_start_offset_iter = dst_start_offset + cur_iter_src * 1 * dpu_byte_length;
                }
            }
        }

        for(uint32_t dim = 2; dim<dimension; dim++){
            if(comm_axis[dim] == 1){
                if(dim==2){
                    iter_src[dim] = cur_iter_src % (4*axis_len[2]/8);
                    iter_dst[dim] = cur_iter_dst % (4*axis_len[2]/8);
                    cur_iter_src = cur_iter_src / (4*axis_len[2]/8);
                    cur_iter_dst = cur_iter_dst / (4*axis_len[2]/8);
                }
                else{
                    iter_src[dim] = cur_iter_src % axis_len[dim];
                    iter_dst[dim] = cur_iter_dst % axis_len[dim];
                    cur_iter_src = cur_iter_src / axis_len[dim];
                    cur_iter_dst = cur_iter_dst / axis_len[dim];
                }
            }
        }

        uint32_t src_rank_id = 0;
        uint32_t* dst_rank_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t src_rg_id = 0;
        uint32_t* dst_rg_id = calloc(total_axis_product, sizeof(uint32_t)); 
        uint32_t temp_total_product = 1;
        uint32_t temp_comm_product = 1;
        for(uint32_t dim=2; dim < dimension; dim++){

            if(dim == 3) temp_total_product *= (4*axis_len[2]/8);
            else if(dim>3) temp_total_product *= axis_len[dim-1];

            src_rank_id += iter_src[dim]*temp_total_product/8;
            src_rg_id += iter_src[dim]*temp_total_product;

            for(uint32_t j=0; j<total_axis_product; j++){
                if(comm_axis[dim] == 0){
                    dst_rank_id[j] += iter_dst[dim] * temp_total_product/8;
                    dst_rg_id[j] += iter_dst[dim] * temp_total_product;
                }
                else{
                    if(dim == 2){
                        dst_rank_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product/8;
                        dst_rg_id[j] += (j % (4*axis_len[2]/8) ) * temp_total_product;
                    }
                    else if (dim>2){
                        dst_rank_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product/8;
                        dst_rg_id[j] += ((j/temp_comm_product) % axis_len[dim]) * temp_total_product;
                    }
                }
            }

            if(comm_axis[dim] == 1){
                if(dim == 2) temp_comm_product *= (4*axis_len[2]/8);
                else if(dim>2) temp_comm_product *= axis_len[dim];
            }
        }
        for(uint32_t j=0; j<total_axis_product; j++) dst_rg_id[j] = dst_rg_id[j] % 8;
        src_rg_id = src_rg_id % 8;

        void* rank_base_address_dst[total_axis_product];
        uint8_t* rank_base_address_src;
        hw_dpu_rank_allocation_parameters_t params_src;
        hw_dpu_rank_allocation_parameters_t params_dst;

        for(uint32_t j=0; j<total_axis_product; j++){
            params_dst = _this_params(comm_dpu_set->list.ranks[dst_rank_id[j]] ->description);
            rank_base_address_dst[j] = params_dst->ptr_region;
        }
        params_src = _this_params(comm_dpu_set->list.ranks[src_rank_id] ->description);
        rank_base_address_src=params_src->ptr_region;

        if(comm_axis[0] == comm_axis[1]) params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, num_inter_thread, thread_id%num_inter_thread);
        else params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, axis_len[0]);

        if(!comm_type){
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, num_inter_thread, thread_id%num_inter_thread);
                else params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_gather_rg_22(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product);
                else params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2);
            }
        }
        else{
            if(comm_axis[1]==1){
                if(comm_axis[2]==1) params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 2);
                else params_src->translate.trans_all_gather_rg_22(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_axis[0], comm_axis[1], comm_axis[2], communication_buffer_offset, total_axis_product);
            }
            else {
                if(comm_axis[2]==1) params_src->translate.trans_all_gather_rg_24(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, 4);
                else params_src->translate.trans_all_gather_rg(rank_base_address_src, rank_base_address_dst, src_rg_id, dst_rg_id, src_start_offset_iter, dst_start_offset_iter, dpu_byte_length, comm_type, communication_buffer_offset, total_axis_product, num_inter_thread, thread_id%num_inter_thread);
            }
        }
    }
    return 0;
}

static dpu_rank_status_e
hw_all_gather_rns(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t dpu_byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis){
    
    uint32_t thread_num=32;
    st_thread_parameter thread_params[thread_num];
    int status;
    pthread_t array_thread[thread_num];
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        thread_params[iter_thread].p_thread_id=iter_thread;
        thread_params[iter_thread].p_comm_dpu_set=comm_dpu_set;
        thread_params[iter_thread].p_src_start_offset=src_start_offset;
        thread_params[iter_thread].p_dst_start_offset=dst_start_offset;
        thread_params[iter_thread].p_dpu_byte_length=dpu_byte_length;
        thread_params[iter_thread].p_comm_type = comm_type;
        thread_params[iter_thread].p_communication_buffer_offset=communication_buffer_offset;
        thread_params[iter_thread].p_num_thread=thread_num;
        thread_params[iter_thread].dimension=dimension;
        thread_params[iter_thread].axis_len=axis_len;
        thread_params[iter_thread].comm_axis=comm_axis;
        
        if(axis_len[0] >=8) pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_rns, (void *) &thread_params[iter_thread]);
        else if(axis_len[0]*axis_len[1] >= 8) pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_24_rns, (void *) &thread_params[iter_thread]);
        else pthread_create(&array_thread[iter_thread], NULL, thread_all_gather_22_rns, (void *) &thread_params[iter_thread]);
    }
    for(uint32_t iter_thread=0; iter_thread<thread_num; iter_thread++){
        pthread_join(array_thread[iter_thread], (void **)&status);
    }
    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_copy_from_rank(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix)
{
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);
    struct dpu_transfer_matrix *ptr_transfer_matrix = transfer_matrix;
    int ret;

    switch (params->mode) {
        case DPU_REGION_MODE_PERF:
            params->translate.read_from_rank(&params->translate, params->ptr_region, params->channel_id, ptr_transfer_matrix);

            break;
        case DPU_REGION_MODE_HYBRID:
            if ((params->translate.capabilities & CAP_HYBRID_CONTROL_INTERFACE) == 0) {
                params->translate.read_from_rank(&params->translate, params->ptr_region, params->channel_id, ptr_transfer_matrix);

                break;
            }
            /* fall through */
        case DPU_REGION_MODE_SAFE:
            ret = ioctl(params->rank_fs.fd_rank, DPU_RANK_IOCTL_READ_FROM_RANK, ptr_transfer_matrix);
            if (ret) {
                LOG_RANK(WARNING, rank, "%s", strerror(errno));
                return DPU_RANK_SYSTEM_ERROR;
            }

            break;
        default:
            return DPU_RANK_SYSTEM_ERROR;
    }

    return DPU_RANK_SUCCESS;
}

#define validate(p)                                                                                                              \
    do {                                                                                                                         \
        if (!(p))                                                                                                                \
            return DPU_RANK_INVALID_PROPERTY_ERROR;                                                                              \
    } while (0)

static void
free_hw_parameters(void *description)
{
    hw_dpu_rank_allocation_parameters_t params = description;

    if (params->fpga.report_path)
        free(params->fpga.report_path);

    free(params);
}

static dpu_rank_status_e
hw_fill_description_from_profile(dpu_properties_t properties, dpu_description_t description)
{
    hw_dpu_rank_allocation_parameters_t parameters;
    uint32_t clock_division, refresh_emulation_period, fck_frequency;
    int ret;
    char *report_path, *rank_path, *region_mode_input = NULL;
    bool activate_ila = false, activate_filtering_ila = false, activate_mram_bypass = false, cycle_accurate = false;
    bool mram_access_by_dpu_only;
    uint8_t chip_id, capabilities_mode;
    bool bypass_module_compatibility;

    parameters = malloc(sizeof(*parameters));
    if (!parameters) {
        return DPU_RANK_SYSTEM_ERROR;
    }

    ret = dpu_sysfs_get_hardware_chip_id(&chip_id);
    if (ret == -1) {
        free(parameters);
        return DPU_RANK_SYSTEM_ERROR;
    }

    validate(fill_description_with_default_values_for((dpu_chip_id_e)chip_id, description));

    ret = dpu_sysfs_get_hardware_description(description, &capabilities_mode);
    if (ret == -1) {
        free(parameters);
        return DPU_RANK_SYSTEM_ERROR;
    }

    validate(fetch_string_property(properties, DPU_PROFILE_PROPERTY_REGION_MODE, &region_mode_input, NULL));
    validate(fetch_string_property(properties, DPU_PROFILE_PROPERTY_RANK_PATH, &rank_path, NULL));
    validate(fetch_integer_property(
        properties, DPU_PROFILE_PROPERTY_CLOCK_DIVISION, &clock_division, description->hw.timings.clock_division));
    validate((clock_division & ~0xFF) == 0);
    validate(fetch_integer_property(
        properties, DPU_PROFILE_PROPERTY_FCK_FREQUENCY, &fck_frequency, description->hw.timings.fck_frequency_in_mhz));
    validate(fetch_boolean_property(
        properties, DPU_PROFILE_PROPERTY_TRY_REPAIR_IRAM, &(description->configuration.do_iram_repair), true));
    validate(fetch_boolean_property(
        properties, DPU_PROFILE_PROPERTY_TRY_REPAIR_WRAM, &(description->configuration.do_wram_repair), true));
    validate(
        fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_IGNORE_VPD, &(description->configuration.ignore_vpd), false));

    /* FPGA specific */
    validate(fetch_string_property(properties, DPU_PROFILE_PROPERTY_REPORT_FILE_NAME, &report_path, "/tmp/fpga_ila_report.csv"));
    validate(fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_ANALYZER_ENABLED, &activate_ila, activate_ila));
    validate(fetch_boolean_property(
        properties, DPU_PROFILE_PROPERTY_ANALYZER_FILTERING_ENABLED, &activate_filtering_ila, activate_filtering_ila));
    validate(fetch_boolean_property(
        properties, DPU_PROFILE_PROPERTY_MRAM_BYPASS_ENABLED, &activate_mram_bypass, activate_mram_bypass));
    validate(fetch_integer_property(properties, DPU_PROFILE_PROPERTY_MRAM_EMULATE_REFRESH, &refresh_emulation_period, 0));
    validate(fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_CYCLE_ACCURATE, &cycle_accurate, cycle_accurate));

    /* XEON SP specific*/
    {
        struct dpu_transfer_thread_configuration xfer_thread_conf;
        validate(fetch_integer_property(properties,
            DPU_PROFILE_PROPERTY_NR_THREAD_PER_POOL,
            &xfer_thread_conf.nb_thread_per_pool,
            DPU_XFER_THREAD_CONF_DEFAULT));
        validate(fetch_integer_property(properties,
            DPU_PROFILE_PROPERTY_POOL_THRESHOLD_1_THREAD,
            &xfer_thread_conf.threshold_1_thread,
            DPU_XFER_THREAD_CONF_DEFAULT));
        validate(fetch_integer_property(properties,
            DPU_PROFILE_PROPERTY_POOL_THRESHOLD_2_THREADS,
            &xfer_thread_conf.threshold_2_threads,
            DPU_XFER_THREAD_CONF_DEFAULT));
        validate(fetch_integer_property(properties,
            DPU_PROFILE_PROPERTY_POOL_THRESHOLD_4_THREADS,
            &xfer_thread_conf.threshold_4_threads,
            DPU_XFER_THREAD_CONF_DEFAULT));
        parameters->translate.xfer_thread_conf = xfer_thread_conf;
    }

    validate(fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_MRAM_ACCESS_BY_DPU_ONLY, &mram_access_by_dpu_only, false));

    validate(fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_IGNORE_VERSION, &bypass_module_compatibility, false));

    memset(&parameters->rank_fs, 0, sizeof(struct dpu_rank_fs));
    if (rank_path) {
        strcpy(parameters->rank_fs.rank_path, rank_path);
        free(rank_path);
    }

    if (region_mode_input) {
        if (!strcmp(region_mode_input, "safe"))
            parameters->mode = (uint8_t)DPU_REGION_MODE_SAFE;
        else if (!strcmp(region_mode_input, "perf"))
            parameters->mode = (uint8_t)DPU_REGION_MODE_PERF;
        else if (!strcmp(region_mode_input, "hybrid"))
            parameters->mode = (uint8_t)DPU_REGION_MODE_HYBRID;
        else {
            LOG_FN(WARNING,
                "Provided region mode (%s) is unknown, switching to default (%s)",
                region_mode_input,
                (capabilities_mode & CAP_PERF) ? "perf" : "safe");
            parameters->mode = (capabilities_mode & CAP_PERF) ? (uint8_t)DPU_REGION_MODE_PERF : (uint8_t)DPU_REGION_MODE_SAFE;
        }

        free(region_mode_input);
    } else {
        LOG_FN(DEBUG, "Region mode not specified, switching to default (%s)", (capabilities_mode & CAP_PERF) ? "perf" : "safe");
        parameters->mode = (capabilities_mode & CAP_PERF) ? (uint8_t)DPU_REGION_MODE_PERF : (uint8_t)DPU_REGION_MODE_SAFE;
    }

    description->configuration.mram_access_by_dpu_only = mram_access_by_dpu_only;

    /* FPGA specific */
    parameters->fpga.activate_ila = activate_ila;
    parameters->fpga.activate_filtering_ila = activate_filtering_ila;
    parameters->fpga.activate_mram_bypass = activate_mram_bypass;
    parameters->fpga.activate_mram_refresh_emulation = refresh_emulation_period != 0;
    parameters->fpga.mram_refresh_emulation_period = refresh_emulation_period;
    parameters->fpga.report_path = report_path;

    parameters->bypass_module_compatibility = bypass_module_compatibility;

    description->configuration.ila_control_refresh = activate_ila;

    description->configuration.enable_cycle_accurate_behavior = cycle_accurate;
    description->hw.timings.clock_division = clock_division;
    description->hw.timings.fck_frequency_in_mhz = fck_frequency;
    description->_internals.data = parameters;
    description->_internals.free = free_hw_parameters;

    return DPU_RANK_SUCCESS;
}

static dpu_rank_status_e
hw_custom_operation(struct dpu_rank_t *rank,
    __attribute__((unused)) dpu_slice_id_t slice_id,
    __attribute__((unused)) dpu_member_id_t member_id,
    dpu_custom_command_t command,
    __attribute__((unused)) dpu_custom_command_args_t args)
{
    dpu_rank_status_e status = DPU_RANK_SUCCESS;
    hw_dpu_rank_allocation_parameters_t params = _this_params(rank->description);

    // Important Note: fpga with ILA support implements only one DPU behind the control interface
    // => that's why we use the DPU_COMMAND_DPU* versions of the custom commands, the operations on ILA
    // would normally apply on the control interface.
    switch (command) {
        case DPU_COMMAND_ALL_SOFT_RESET:
        case DPU_COMMAND_DPU_SOFT_RESET:
            if (params->fpga.activate_ila) {
                if (!reset_ila(&params->rank_fs)) {
                    status = DPU_RANK_SYSTEM_ERROR;
                    break;
                }

                if (params->fpga.activate_filtering_ila) {
                    if (!activate_filter_ila(&params->rank_fs)) {
                        status = DPU_RANK_SYSTEM_ERROR;
                        break;
                    }
                } else {
                    if (!deactivate_filter_ila(&params->rank_fs)) {
                        status = DPU_RANK_SYSTEM_ERROR;
                        break;
                    }
                }

                set_mram_bypass_to(&params->rank_fs, params->fpga.activate_mram_bypass);

                if (params->fpga.activate_mram_refresh_emulation) {
                    if (!enable_refresh_emulation(&params->rank_fs, params->fpga.mram_refresh_emulation_period)) {
                        status = DPU_RANK_SYSTEM_ERROR;
                        break;
                    }
                } else {
                    if (!disable_refresh_emulation(&params->rank_fs)) {
                        status = DPU_RANK_SYSTEM_ERROR;
                        break;
                    }
                }
            }

            break;
        case DPU_COMMAND_ALL_PREEXECUTION:
        case DPU_COMMAND_DPU_PREEXECUTION:
            if (params->fpga.activate_ila) {
                if (!activate_ila(&params->rank_fs)) {
                    status = DPU_RANK_SYSTEM_ERROR;
                    break;
                }
            }

            break;
        case DPU_COMMAND_ALL_POSTEXECUTION:
        case DPU_COMMAND_DPU_POSTEXECUTION:
            if (params->fpga.activate_ila) {
                if (!deactivate_ila(&params->rank_fs)) {
                    status = DPU_RANK_SYSTEM_ERROR;
                    break;
                }

                if (!dump_ila_report(&params->rank_fs, params->fpga.report_path)) {
                    status = DPU_RANK_SYSTEM_ERROR;
                    break;
                }
            }

            break;
        default:
            break;
    }

    return status;
}

static dpu_rank_status_e
hw_get_nr_dpu_ranks(uint32_t *nr_ranks)
{
    *nr_ranks = dpu_sysfs_get_nb_physical_ranks();
    return DPU_RANK_SUCCESS;
}
