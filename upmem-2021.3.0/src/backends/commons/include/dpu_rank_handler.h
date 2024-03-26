/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_RANK_HANDLER_H
#define DPU_RANK_HANDLER_H

#include <stdint.h>
#include <dpu_description.h>
#include <dpu_target.h>
#include <dpu_custom.h>
#include <dpu_properties.h>
#include <dpu_types.h>


typedef enum _dpu_rank_status_e {
    DPU_RANK_SUCCESS = 0,
    DPU_RANK_COMMUNICATION_ERROR,
    DPU_RANK_BACKEND_ERROR,
    DPU_RANK_SYSTEM_ERROR,
    DPU_RANK_INVALID_PROPERTY_ERROR,
    DPU_RANK_ENODEV,
} dpu_rank_status_e;

typedef uint64_t *dpu_rank_buffer_t;

typedef struct dpu_rank_handler {
    dpu_rank_status_e (*allocate)(struct dpu_rank_t *rank, dpu_description_t description);
    dpu_rank_status_e (*free)(struct dpu_rank_t *rank);

    dpu_rank_status_e (*commit_commands)(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer);
    dpu_rank_status_e (*update_commands)(struct dpu_rank_t *rank, dpu_rank_buffer_t buffer);

    dpu_rank_status_e (*copy_to_rank)(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);
    dpu_rank_status_e (*copy_from_rank)(struct dpu_rank_t *rank, struct dpu_transfer_matrix *transfer_matrix);
    
    dpu_rank_status_e (*copy_rank_RNS)(struct dpu_rank_t *rank_src, struct dpu_rank_t *rank_dst, 
        struct dpu_transfer_matrix *transfer_matrix_src, struct dpu_transfer_matrix *transfer_matrix_dst);
    
    dpu_rank_status_e (*do_rnc)(RNS_Job_Queue_t*);

    dpu_rank_status_e (*gather_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);
    dpu_rank_status_e (*gather_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);
    dpu_rank_status_e (*gather_z_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void ** host_buffer);
    dpu_rank_status_e (*gather_xz_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t target_dpu_index, void ** host_buffer);

    dpu_rank_status_e (*reduce_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void ** host_buffer);
    dpu_rank_status_e (*reduce_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t data_type, void ** host_buffer);

    dpu_rank_status_e (*scatter_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);
    dpu_rank_status_e (*scatter_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, void ** host_buffer);

    dpu_rank_status_e (*reduce_scatter_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size);
    dpu_rank_status_e (*reduce_scatter_cpu_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size);
    dpu_rank_status_e (*reduce_scatter_cpu_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size);

    dpu_rank_status_e (*all_reduce_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis, uint32_t size, uint32_t reduce_type);
    dpu_rank_status_e (*all_reduce_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type);
    dpu_rank_status_e (*all_reduce_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset, uint32_t size, uint32_t reduce_type);

    dpu_rank_status_e (*all_gather_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis);
    dpu_rank_status_e (*all_gather_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_gather_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_gather_z_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_gather_xz_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);

    dpu_rank_status_e (*all_to_all_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t comm_type, uint32_t communication_buffer_offset, uint32_t dimension, uint32_t* axis_len, uint32_t* comm_axis);
    dpu_rank_status_e (*all_to_all_x_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_to_all_xz_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_to_all_y_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);
    dpu_rank_status_e (*all_to_all_z_rns)(struct dpu_set_t *comm_dpu_set, uint32_t src_start_offset, uint32_t dst_start_offset, uint32_t byte_length, uint32_t a, uint32_t b, uint32_t c, uint32_t alltoall_comm_type, uint32_t communication_buffer_offset);

    struct {
#define FEATURE(feature, ...) dpu_error_t (*feature)(__VA_ARGS__);
#include <rank_features.def>
#undef FEATURE
    } features;

    dpu_rank_status_e (*custom_operation)(struct dpu_rank_t *rank,
        dpu_slice_id_t slice_id,
        dpu_member_id_t member_id,
        dpu_custom_command_t command,
        dpu_custom_command_args_t args);
    dpu_rank_status_e (*fill_description_from_profile)(dpu_properties_t properties, dpu_description_t description);

    dpu_rank_status_e (*get_nr_dpu_ranks)(uint32_t *nr_ranks);
} * dpu_rank_handler_t;

#define RANK_FEATURE(rank, feature) ((rank)->handler_context->handler->features.feature)

/* We need to keep a global handler for further rank allocation: the first 'allocates' it, the others get it. */
typedef struct _dpu_rank_handler_context_t {
    dpu_rank_handler_t handler;
    int handler_refcount;
    void *library;
} * dpu_rank_handler_context_t;

bool
dpu_rank_handler_instantiate(dpu_type_t type, dpu_rank_handler_context_t *ret_handler_context, bool verbose);
void
dpu_rank_handler_release(dpu_rank_handler_context_t handler_context);

bool
dpu_rank_handler_get_rank(struct dpu_rank_t *rank, dpu_rank_handler_context_t handler_context, dpu_properties_t properties);
void
dpu_rank_handler_free_rank(struct dpu_rank_t *rank, dpu_rank_handler_context_t handler_context);

void
dpu_rank_handler_start_iteration(struct dpu_rank_t ***ranks, unsigned int *nb_ranks);
void
dpu_rank_handler_end_iteration(void);

#endif // DPU_RANK_HANDLER_H
