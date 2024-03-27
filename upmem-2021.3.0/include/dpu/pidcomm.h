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

//common.h
#ifdef UINT32
#define T uint32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT32
#define T int32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT16
#define T int16_t
#define byte_dt 2
#define DIV 1 // Shift right to divide by sizeof(T)
#elif INT8
#define T int8_t
#define byte_dt 1
#define DIV 0 // Shift right to divide by sizeof(T)
#elif INT64
#define T int64_t
#define byte_dt 8
#define DIV 3 // Shift right to divide by sizeof(T)
#endif

typedef struct {
    uint32_t start_offset;
    uint32_t target_offset;
    uint32_t total_data_size;
    uint32_t num_comm_dpu;
    uint32_t each_dpu;
    bool no_rotate;
    uint32_t num_row;
    uint32_t comm_type;
    uint32_t a_length;
    uint32_t num_comm_rg;
} dpu_arguments_comm_t;

//Timer.h
typedef struct Timer{
    struct timeval startTime[12];
    struct timeval stopTime[12];
    double         time[12];

} Timer;

static void resetTimer(Timer *timer){
    for(int i=0; i<12; i++){
        timer->time[i] = 0.0;
    }
}

static void startTimer(Timer *timer, int i) {
    timer->time[i]=0.0;
    gettimeofday(&timer->startTime[i], NULL);
}

static void stopTimer(Timer *timer, int i) {
    gettimeofday(&timer->stopTime[i], NULL);
    timer->time[i] += (timer->stopTime[i].tv_sec - timer->startTime[i].tv_sec) * 1000000.0 +
        (timer->stopTime[i].tv_usec - timer->startTime[i].tv_usec);
}

static void printTimer(Timer *timer, int i) { 
    if (i == 0 || i == 2)
        printf("Time (ms): \t\t\t%f\n", timer->time[i] / (1000)); 
    else if (i == 1)
        printf("Time (ms): \t\t%f\n", timer->time[i] / (1000)); 
    else
        printf("Time (ms): \t%f\n", timer->time[i] / (1000)); 
}


void reduce_scatter_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c, int size){

                            Timer timer;

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        //result[i] = (T*) calloc(total_data_size/sizeof(T), sizeof(T));
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));

    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset + buffer_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
        dpu_argument[i].a_length = a;
    }

    int i;
        
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));


    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));  

    reduce_scatter_cpu_x(&dpu_set, start_offset, target_offset, total_data_size/num_comm_dpu, a, b, c, 0, buffer_offset, size);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    return;
}

void reduce_scatter_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c, uint32_t size){


                            Timer timer;

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));

    //load binary file : relocate data from start offset to target offset
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset + buffer_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 1;
        dpu_argument[i].comm_type = 1;
        dpu_argument[i].a_length = a;
    }

    int i;

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset + buffer_offset, 8, DPU_XFER_DEFAULT));
    

    reduce_scatter_cpu_y(&dpu_set, start_offset, target_offset, total_data_size/num_comm_dpu, a, b, c, 1, buffer_offset, size);

    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    return;
}

void all_reduce_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c, uint32_t size, uint32_t reduce_type){


                            Timer timer;

    //load binary file : relocate data from start offset to target offset
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));
    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
        dpu_argument[i].a_length = a;
    }

    int i;

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));

    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset, 8, DPU_XFER_DEFAULT));


    all_reduce_x(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 0, buffer_offset, size, reduce_type);

    
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2, NULL));
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    return;
}



void all_reduce_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c, uint32_t size, uint32_t reduce_type){

    int i;

    /**********************************************************************************************/

    all_reduce_y(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 1, buffer_offset, size, reduce_type);

    /**********************************************************************************************/

    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_Y, NULL));

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    return;
}

void all_to_all_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c){
    Timer timer;

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        //result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;
    
    
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
        dpu_argument[i].a_length = a;
    }

    
        
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));     
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));


    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));


    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));
    //DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset, total_data_size, DPU_XFER_DEFAULT));



    all_to_all_x(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 0, buffer_offset);



    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_X_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
    }

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));


    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
}

void all_to_all_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c){
    Timer timer;

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
int i;
    



    /***********************************************************************/

    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset, 8, DPU_XFER_DEFAULT));




    all_to_all_y(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 1, buffer_offset);


    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset+ buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 1;
        dpu_argument[i].comm_type = 1;
        dpu_argument[i].a_length = a;
    }

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
}

void all_gather_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c){
    Timer timer;

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
int i;

    all_gather_x(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 0, buffer_offset);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_X_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
    }
        
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
}

void all_gather_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c){
    Timer timer;

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));

    int i;

    //printTimer(&timer, 1);


    all_gather_y(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 1, buffer_offset);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 1;
        dpu_argument[i].comm_type = 1;
        dpu_argument[i].a_length = a;
    }

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
}


void all_to_all_xz_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t target_offset, uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c){
    Timer timer;

    //for testing
    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;
    
    
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
        dpu_argument[i].a_length = a;
    }

    
        
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));     
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));


    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));


    //retrieve results
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    all_to_all_xz(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, a, b, c, 0, buffer_offset);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    DPU_ASSERT(dpu_load(dpu_set, DPU_ALLTOALL_X_2, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].each_dpu = i;
        dpu_argument[i].start_offset = start_offset + buffer_offset;
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = total_data_size;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].comm_type = 0;
    }

    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));


    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
}

void gather_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, int a, int b, int c, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    gather_x(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 0, buffer_offset, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

void gather_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, int a, int b, int c, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    gather_y(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 1, buffer_offset, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

void scatter_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, int a, int b, int c, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    scatter_x(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 0, buffer_offset, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

void scatter_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, int a, int b, int c, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    scatter_y(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 1, buffer_offset, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

void reduce_x_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, uint32_t num_comm_dpu, int a, int b, int c, int size, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

    for(i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = start_offset;
        dpu_argument[i].target_offset = start_offset;
        dpu_argument[i].total_data_size = total_data_size/num_comm_dpu;
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 0;
        dpu_argument[i].a_length = a;
    }
        
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
            
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    // Run kernel on DPUs
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));



    reduce_x(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 0, buffer_offset, size, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

void reduce_y_CPU(struct dpu_set_t dpu_set, struct dpu_set_t dpu, dpu_arguments_comm_t* dpu_argument, uint32_t total_data_size, uint32_t start_offset,
                        uint32_t buffer_offset, uint32_t nr_dpus, int a, int b, int c, int size, void** host_buffer){

    T** result = (T**) calloc(nr_dpus, sizeof(T*));
    for(int i=0; i<nr_dpus; i++)
        result[i] = (T*) calloc(8/sizeof(T), sizeof(T));
    int i;

    reduce_y(&dpu_set, start_offset, start_offset, total_data_size, a, b, c, 1, buffer_offset, size, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}



//Hypercube manager
typedef struct {
    struct dpu_set_t dpu_set;
    uint32_t dimension;
    uint32_t* axis_len;
} hypercube_manager;


hypercube_manager* init_hypercube_manager(struct dpu_set_t dpu_set, uint32_t dimension, uint32_t* axis_len){
    hypercube_manager* manager = malloc(sizeof(hypercube_manager));

    manager->dpu_set = dpu_set;
    manager->dimension = dimension;
    manager->axis_len = axis_len;

    return manager;
}

//Supported Communication Primitives
void pidcomm_broadcast(hypercube_manager* manager, uint32_t total_data_size, uint32_t target_offset, void* data){
    struct dpu_set_t dpu_set = manager -> dpu_set;
    DPU_ASSERT(dpu_broadcast_to(dpu_set, DPU_MRAM_HEAP_POINTER_NAME, target_offset, data, total_data_size, DPU_XFER_DEFAULT));
}

void all_to_all_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));     
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

    all_to_all(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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

        

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

void reduce_scatter_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
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
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22, NULL));

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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    //relocate before kernel
    else if(!comm_type){

        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset + buffer_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
        }

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, start_offset + buffer_offset, total_data_size, DPU_XFER_DEFAULT));

    reduce_scatter(&dpu_set, start_offset, target_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size);
    

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}

void all_reduce_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
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
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22, NULL));

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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

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
            dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //kernel function of All-Reduce
    all_reduce(&dpu_set, start_offset, start_offset, total_data_size/num_comm_dpu, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, reduce_type);
    

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));


    //relocate before kernel
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
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
                
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        DPU_ASSERT(dpu_load(dpu_set, DPU_AR_24, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
    else{
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_AR_2_Y, NULL));

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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}

void all_gather_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset,
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
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }
}


void gather_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, 
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

    gather_x(&dpu_set, start_offset, start_offset, total_data_size, axis_len[0], axis_len[1], axis_len[2], 0, buffer_offset, host_buffer);

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));   

}

/*void reduce_CPU(hypercube_manager* manager, char* comm, uint32_t total_data_size, uint32_t start_offset, uint32_t target_offset, \
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
    if(axis_len[0]==2 && axis_len[1]==2 && ( ((comm_axis[0]==0) && (comm_axis[1]==1) && (comm_axis[2]==0)) || ((comm_axis[0]==1) && (comm_axis[1]==0) && (comm_axis[2]==1)))){
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_22, NULL));

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

        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    else if(axis_len[0] < 8 && ((num_comm_rg < 8) && (num_comm_rg > 1) )){
        DPU_ASSERT(dpu_load(dpu_set, DPU_RS_24, NULL));

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
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    }
    else if(!comm_type){

        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY_RELOCATE_2, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].each_dpu = i;
            dpu_argument[i].start_offset = start_offset;
            dpu_argument[i].target_offset = start_offset;
            dpu_argument[i].total_data_size = total_data_size;
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 0;
            //dpu_argument[i].comm_type = comm_type;
            dpu_argument[i].a_length = axis_len[0];
            //dpu_argument[i].num_comm_rg = num_comm_rg;
        }
            
        DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        // Run kernel on DPUs
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    // i=0;
    // DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
    //     DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    // }
    // DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT)); 


    //kernel function of All-Reduce
    reduce(&dpu_set, start_offset, start_offset, total_data_size, comm_type, buffer_offset, dimension, axis_len, comm_axis, size, host_buffer);
    

    i=0;
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, i, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, result[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, 8, DPU_XFER_DEFAULT));

}*/

#endif