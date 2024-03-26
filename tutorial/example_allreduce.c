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
#include <dpu_error.h>
#include <dpu_management.h>
#include <dpu_program.h>

#include <pthread.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <sys/sysinfo.h>
#include <getopt.h>
#include <omp.h>
#include <sys/time.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

//It is necessary to load a DPU binary file into the DPU prior to data transfer.
#ifndef DPU_BINARY_USER
#define DPU_BINARY_USER "./bin/dpu_user"
#endif

//For supported communication primitives.
#include <pidcomm.h>

int main(){
    struct dpu_set_t dpu, set, set_array[16];
    uint32_t each_dpu, alloc_type;

    //Hypercube Configuration
    uint32_t nr_dpus = 32; //the number of DPUs
    uint32_t dimension=3;
    uint32_t axis_len[3]; //The number of DPUs for each axis of the hypercube
    uint32_t comm_axis[3]; //Communicator(Communication Group) Configuration

    //Set the hypercube configuration
    printf("nr_dpus: "); scanf("%d", &nr_dpus);
    printf("axis_len[0]: "); scanf("%d", axis_len+0);
    printf("axis_len[1]: "); scanf("%d", axis_len+1);
    printf("axis_len[2]: "); scanf("%d", axis_len+2);

    //Set the variables for the PID-Comm.
    comm_axis[0]=1; comm_axis[1]=0; comm_axis[2]=0;
    uint32_t start_offset=0; //Offset of source.
    uint32_t target_offset=0; //Offset of destination.
    uint32_t buffer_offset=1024*1024*32; //To ensure effective communication, PID-Comm required buffer. Please ensure that the offset of the buffer is larger than the data size.
    dpu_arguments_comm_t dpu_argument[nr_dpus];


    uint32_t data_size_per_dpu = 64*axis_len[0]; //data size for each nodes
    uint32_t data_num_per_dpu = data_size_per_dpu/sizeof(uint32_t);
    
    //You must allocate and load a DPU bianry file.
    DPU_ASSERT(dpu_alloc_comm(nr_dpus, NULL, &set, 1));
    DPU_ASSERT(dpu_load(set, DPU_BINARY_USER, NULL));

    //Randomly set the data
    uint32_t *original_data = (uint32_t*)calloc(data_num_per_dpu*nr_dpus, sizeof(uint32_t));
    #pragma omp parallel for
    for(uint32_t i=0; i<data_num_per_dpu*nr_dpus; i++){
        original_data[i] = rand() % 16;
    }

    //Perform conventional AllReduce in the host CPU.
    uint32_t *conv_data = (uint32_t*)calloc(data_num_per_dpu*nr_dpus, sizeof(uint32_t));
    #pragma omp parallel for collapse(2)
    for(uint32_t jk=0; jk<axis_len[1]*axis_len[2]; jk++){
        for(uint32_t dst_dpu=0; dst_dpu<axis_len[0]; dst_dpu++){
            for(uint32_t src_dpu=0; src_dpu<axis_len[0]; src_dpu++){
                uint32_t dst_dpu_id = jk * axis_len[0] + dst_dpu;
                uint32_t src_dpu_id = jk * axis_len[0] + src_dpu;

                for(uint32_t data_index=0; data_index<data_num_per_dpu; data_index++){
                    conv_data[(dst_dpu_id * data_num_per_dpu + data_index)] += \
                    original_data[(src_dpu_id * data_num_per_dpu + data_index)];
                }
            }
        }
    }

    //Perform Scatter
    DPU_FOREACH_ROTATE_GROUP(set, dpu, each_dpu, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, original_data+each_dpu*data_num_per_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, data_size_per_dpu, DPU_XFER_DEFAULT));

    //Perform AllReduce by utilizing PID-Comm
    all_reduce_CPU(set, data_size_per_dpu, start_offset, target_offset, buffer_offset, dimension, axis_len, comm_axis, sizeof(T), 0);

    //Perform Gather
    DPU_FOREACH_ROTATE_GROUP(set, dpu, each_dpu, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, original_data+each_dpu*data_num_per_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, data_size_per_dpu, DPU_XFER_DEFAULT));

    //Functionality check
    uint32_t flag=1;
    for(uint32_t i=0; i<nr_dpus; i++){
        for(uint32_t j=0; j<data_num_per_dpu; j++){
            uint32_t check_index = i * data_num_per_dpu + j;
            if(original_data[check_index] != conv_data[check_index] && flag>0){
                flag-=1;
                printf("error exist! i=%d, j=%d, orig_value=%d, conv_value=%d\n", i, j, original_data[check_index], conv_data[check_index]);
            }
        }
    }
    if(flag==1) printf("Functionality check success~!\n");
    pidcomm_broadcast(set, 8, 0, original_data+1);

    //Print reduced result
    // for(int element=0; element<4; element++){
    //     for(int i=0; i<nr_dpus; i++){
    //         printf("dpu%d: %2d", i, original_data[i*data_num_per_dpu + element]);
    //         if(i!=nr_dpus-1) printf(", ");
    //     }
    //     printf("\n");
    // }

    free(original_data);
    free(conv_data);
    DPU_ASSERT(dpu_free(set));
    return 0;
}