/* Copyright 2024 AISys. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
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


//Hypercube manager
typedef struct {
    struct dpu_set_t dpu_set;
    uint32_t dimension;
    uint32_t* axis_len;
} hypercube_manager;

__API_SYMBOL__
hypercube_manager* init_hypercube_manager(struct dpu_set_t dpu_set, uint32_t dimension, uint32_t* axis_len){
    hypercube_manager* manager = malloc(sizeof(hypercube_manager));

    manager->dpu_set = dpu_set;
    manager->dimension = dimension;
    manager->axis_len = axis_len;

    return manager;
}

//Supported Communication Primitives
__API_SYMBOL__
void pidcomm_broadcast(hypercube_manager* manager, uint32_t total_data_size, uint32_t target_offset, void* data){
    struct dpu_set_t dpu_set = manager -> dpu_set;
    DPU_ASSERT(dpu_broadcast_to(dpu_set, DPU_MRAM_HEAP_POINTER_NAME, target_offset, data, total_data_size, DPU_XFER_DEFAULT));
}

__API_SYMBOL__
void pidcomm_alltoall(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }


    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //relocate before kernel
    if(!comm_type){

        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));     
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    all_to_all(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset + buffer_offset, total_data_size, DPU_XFER_DEFAULT));


    //relocate after kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_22, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else if(!comm_type  || (axis_len[0]<8 && comm_axis[1]==1) || (axis_len[0]*axis_len[1]==4 && (comm_axis[1] == 1 || comm_axis[2] == 1)) ){
        DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_X_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset+ buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

__API_SYMBOL__
void pidcomm_reduce_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t size){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
    int i;

    printf("nr_dpu : %d, num_comm_dpu : %d, num_comm_rg : %d\n", nr_dpus, num_comm_dpu, num_comm_rg);

    if(axis_len[0]==2 && axis_len[1]==2 && ( ((comm_axis[0]==0) && (comm_axis[1]==1) && (comm_axis[2]==0)) || ((comm_axis[0]==1) && (comm_axis[1]==0) && (comm_axis[2]==1)))){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = 2;
            dpu_argument[i].num_comm_rg = 2;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    //relocate before kernel
    else if(!comm_type){

        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset + buffer_offset, total_data_size, DPU_XFER_DEFAULT));

    reduce_scatter(&dpu_set, start_offset, target_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}

__API_SYMBOL__
void pidcomm_all_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
                    uint32_t buffer_offset, uint32_t size, uint32_t reduce_type){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    printf("nr_dpu : %d, num_comm_dpu : %d, num_comm_rg : %d\n", nr_dpus, num_comm_dpu, num_comm_rg);

    //relocate before kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ( ((comm_axis[0]==0) && (comm_axis[1]==1) && (comm_axis[2]==0)) || ((comm_axis[0]==1) && (comm_axis[1]==0) && (comm_axis[2]==1)))){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = 2;
            dpu_argument[i].num_comm_rg = 2;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //kernel function of All-Reduce
    all_reduce(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, reduce_type);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //relocate before kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_22_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_22_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_AR_24_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_AR_24_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset+buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_Y_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_Y_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

__API_SYMBOL__
void pidcomm_allgather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }


    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }
    printf("nr_dpu : %d, num_comm_rg : %d\n", nr_dpus, num_comm_rg);

    

    //relocate before kernel

    all_gather(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    //relocate after kernel
    if(axis_len[0]==2 && axis_len[1]==2 && ((comm_axis[0]==1 && comm_axis[1]==0 && comm_axis[2]==1) || (comm_axis[0]==0 && comm_axis[1]==1 && comm_axis[2]==0))){
        DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_22, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = 2;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else if(!comm_type  || (axis_len[0]<8 && comm_axis[1]==1) || (axis_len[0]*axis_len[1]==4 && (comm_axis[1] == 1 || comm_axis[2] == 1))){
        DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_X_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset + buffer_offset;
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

__API_SYMBOL__
void pidcomm_gather(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, 
                                                        uint32_t buffer_offset, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    gather(&dpu_set, start_offset, start_offset, total_data_size, 0, buffer_offset, dimension, axis_len, comm_axis, host_buffer);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

__API_SYMBOL__
void pidcomm_reduce(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
                    uint32_t buffer_offset, uint32_t size, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //relocate before kernel
    if(!comm_type){

        if(size==1) DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT8, NULL));
        else DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2_INT32, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            dpu_argument[i].a_length = axis_len[0];
        }
            
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, total_data_size, DPU_XFER_DEFAULT));


    //kernel function of All-Reduce
    reduce(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, total_data_size, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, host_buffer);
    

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}

//total data size is size of data each dpu will receive
__API_SYMBOL__
void pidcomm_scatter(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
                    uint32_t buffer_offset, void** host_buffer){

    struct dpu_set_t dpu_set = manager->dpu_set;
    uint32_t dimension = manager->dimension;
    uint32_t* axis_len = manager->axis_len;

    uint32_t* comm_axis = malloc(sizeof(uint32_t) * dimension);

    for(uint32_t dim=0; dim<dimension; dim++){
        comm_axis[dim] = (int)(*(comm+dim))-48;
    }

    struct dpu_set_t dpu;
    uint32_t nr_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);
    uint32_t num_comm_dpu = 1;
    uint32_t comm_type;

    if(comm_axis[0] == 1){
        comm_type = 0;
    }
    else comm_type = 1;

    for(uint32_t dim=0; dim<dimension; dim++){
        if(comm_axis[dim]==1){
            num_comm_dpu *= axis_len[dim];
        }
    }

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
    int i;

    uint32_t num_comm_rg = 1;
    for(uint32_t dim=0, len = 1; dim<dimension, len<8; len*=axis_len[dim], dim++){
        if(comm_axis[dim] == 1){
            if (axis_len[dim] <= (8/len)) num_comm_rg *= axis_len[dim];
            else num_comm_rg *= (8/len);
        }
        if(num_comm_rg >= 8) num_comm_rg = 8;
    }

    //kernel function of All-Reduce
    scatter(&dpu_set, start_offset, start_offset, total_data_size, comm_type, buffer_offset, dimension, axis_len, comm_axis, host_buffer);

    i=0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));
}

#endif