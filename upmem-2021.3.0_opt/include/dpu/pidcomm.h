#ifndef _COMMLIB_H_
#define _COMMLIB_H_

#include <dpu.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <dpu_types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <dpu_transfer_matrix.h>
#include <dpu.h>
#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_management.h>
#include <dpu_program.h>
#include <dpu_memory.h>

#include <pthread.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <support.h>

#ifndef DPU_BINARY_AR_2
#define DPU_BINARY_AR_2 "./bin/dpu_ar_2"
#endif

#ifndef DPU_BINARY_RELOCATE_2
#define DPU_BINARY_RELOCATE_2 "./bin/dpu_relocate_inplace"
#endif

#ifndef DPU_BINARY_AR_2_Y
#define DPU_BINARY_AR_2_Y "./bin/dpu_ar_2_y"
#endif

#ifndef DPU_ALLTOALL_X_2
#define DPU_ALLTOALL_X_2 "./bin/alltoall_x_2"
#endif

#ifndef DPU_ALLTOALL_22
#define DPU_ALLTOALL_22 "./bin/alltoall_22"
#endif

#ifndef DPU_RS_24
#define DPU_RS_24 "./bin/rs_24"
#endif

#ifndef DPU_RS_22
#define DPU_RS_22 "./bin/rs_22"
#endif

#ifndef DPU_AR_24
#define DPU_AR_24 "./bin/ar_24"
#endif

#ifndef DPU_BINARY_AR_2_INT8
#define DPU_BINARY_AR_2_INT8 "./bin/dpu_ar_2_int8"
#endif

#ifndef DPU_BINARY_RELOCATE_2_INT8
#define DPU_BINARY_RELOCATE_2_INT8 "./bin/dpu_relocate_inplace_int8"
#endif

#ifndef DPU_BINARY_AR_2_Y_INT8
#define DPU_BINARY_AR_2_Y_INT8 "./bin/dpu_ar_2_y_int8"
#endif

#ifndef DPU_ALLTOALL_X_2_INT8
#define DPU_ALLTOALL_X_2_INT8 "./bin/alltoall_x_2_int8"
#endif

#ifndef DPU_ALLTOALL_22_INT8
#define DPU_ALLTOALL_22_INT8 "./bin/alltoall_22_int8"
#endif

#ifndef DPU_RS_24_INT8
#define DPU_RS_24_INT8 "./bin/rs_24_int8"
#endif

#ifndef DPU_RS_22_INT8
#define DPU_RS_22_INT8 "./bin/rs_22_int8"
#endif

#ifndef DPU_AR_24_INT8
#define DPU_AR_24_INT8 "./bin/ar_24_int8"
#endif

#ifndef DPU_BINARY_AR_2_INT32
#define DPU_BINARY_AR_2_INT32 "./bin/dpu_ar_2_int32"
#endif

#ifndef DPU_BINARY_RELOCATE_2_INT32
#define DPU_BINARY_RELOCATE_2_INT32 "./bin/dpu_relocate_inplace_int32"
#endif

#ifndef DPU_BINARY_AR_2_Y_INT32
#define DPU_BINARY_AR_2_Y_INT32 "./bin/dpu_ar_2_y_int32"
#endif

#ifndef DPU_ALLTOALL_X_2_INT32
#define DPU_ALLTOALL_X_2_INT32 "./bin/alltoall_x_2_int32"
#endif

#ifndef DPU_ALLTOALL_22_INT32
#define DPU_ALLTOALL_22_INT32 "./bin/alltoall_22_int32"
#endif

#ifndef DPU_RS_24_INT32
#define DPU_RS_24_INT32 "./bin/rs_24_int32"
#endif

#ifndef DPU_RS_22_INT32
#define DPU_RS_22_INT32 "./bin/rs_22_int32"
#endif

#ifndef DPU_AR_24_INT32
#define DPU_AR_24_INT32 "./bin/ar_24_int32"
#endif

//Hypercube manager
typedef struct {
    struct dpu_set_t dpu_set;
    uint32_t dimension;
    uint32_t* axis_len;
} hypercube_manager;

/**
 * @brief Initialize the hypercube manager
 * @param dpu_set the identifier of the DPU set
 * @param dimension the number of dimensions of the hypercube
 * @param axis_len the array that contains the length of each side in the hypercube
 */
hypercube_manager* init_hypercube_manager(struct dpu_set_t dpu_set, uint32_t dimension, uint32_t* axis_len);

/**
 * @brief broadcast() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param total_data_size the number of bytes for each DPUs
 * @param target_offset the byte offset from the DPU's MRAM address where to copy the data
 * @param data the host buffer containing the data to copy
 */
void
pidcomm_broadcast(hypercube_manager* manager, uint32_t total_data_size, uint32_t target_offset, void* data);

/**
 * @brief alltoall() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param target_offset the byte offset from the DPU's MRAM address where to copy the data
 * @param buffer_offset the size of the buffer in bytes
 */
void
pidcomm_alltoall(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, uint32_t buffer_offset);

/**
 * @brief reduce_scatter() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param target_offset the byte offset from the DPU's MRAM address where to copy the data
 * @param buffer_offset the size of the buffer in bytes
 * @param size the size of the datatype
 */
void
pidcomm_reduce_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, uint32_t buffer_offset, uint32_t size);

/**
 * @brief all_reduce() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param target_offset the byte offset from the DPU's MRAM address where to copy the data
 * @param buffer_offset the size of the buffer in bytes
 * @param size the size of the datatype
 * @param reduce_type the type of reduction operation. 1 for sum operation and 2 for max operation
 */
void
pidcomm_all_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, uint32_t buffer_offset, uint32_t size, uint32_t reduce_type);

/**
 * @brief allgather() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param target_offset the byte offset from the DPU's MRAM address where to copy the data
 * @param buffer_offset the size of the buffer in bytes
 */
void
pidcomm_allgather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, uint32_t buffer_offset);

/**
 * @brief reduce() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param buffer_offset the size of the buffer in bytes
 * @param size the size of the datatype
 * @param host_buffer the host buffer containing the data to copy
 */
void
pidcomm_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t buffer_offset, uint32_t size, void** host_buffer);

/**
 * @brief gather() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data is copied
 * @param buffer_offset the size of the buffer in bytes
 * @param host_buffer the host buffer containing the data to copy
 */
void
pidcomm_gather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t buffer_offset, void** host_buffer);

/**
 * @brief scatter() for PID-Comm
 * @param manager the hypercube manager that contains information about the hypercube
 * @param comm the bitmap string that contains the target dimensions
 * @param total_data_size the number of bytes for each DPUs
 * @param start_offset the byte offset from the DPU's MRAM address where the data to copy
 * @param buffer_offset the size of the buffer in bytes
 * @param host_buffer the host buffer where the data is copied
 */
void
pidcomm_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t buffer_offset, void** host_buffer);

#endif