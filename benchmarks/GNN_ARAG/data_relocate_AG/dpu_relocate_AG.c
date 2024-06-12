#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <seqread.h>

#include "../../../pidcomm_lib/support/common.h"

#define COMM_UNIT_SIZE 8

__host dpu_arguments_comm_t DPU_INPUT_ARGUMENTS_RS1;

BARRIER_INIT(my_barrier, NR_TASKLETS/2);

/*
 * In this function we aim to reorder target data chunks so that
 * we can execute collective communication in one go
 * we are transposing(?) the matrix to make data being sent to
 * the same PE be side by side in the MRAM memory space
 * 
 *     1 2 3 4        1 1 1 1
 *     1 2 3 4   =>   2 2 2 2
 *     1 2 3 4        3 3 3 3
 *     1 2 3 4        4 4 4 4
 */

int main(){

    uint32_t tasklet_id = me();

    if(tasklet_id >= 8) goto PASS;

    if(tasklet_id == 0) mem_reset();

    barrier_wait(&my_barrier);

    //set arguments for use
    uint32_t start_offset = DPU_INPUT_ARGUMENTS_RS1.start_offset;
    uint32_t target_offset = DPU_INPUT_ARGUMENTS_RS1.target_offset;
    uint32_t total_data_size = DPU_INPUT_ARGUMENTS_RS1.total_data_size; // size of data from one DPU
    uint32_t num_comm_dpu = DPU_INPUT_ARGUMENTS_RS1.num_comm_dpu;
    uint32_t dpu_num = DPU_INPUT_ARGUMENTS_RS1.each_dpu;
    uint32_t no_rotate = DPU_INPUT_ARGUMENTS_RS1.no_rotate;
    uint32_t num_row = DPU_INPUT_ARGUMENTS_RS1.num_row; // number of rows for data from one DPU

    uint32_t original_addr; // address for word to move
    uint32_t target_addr; //address for word to write

    uint32_t num_col = total_data_size/(num_row*num_comm_dpu*sizeof(T));

    uint32_t comm_size = num_col*sizeof(T);

    //cache is used to move one word at a time to the right place
    T* word_cache = (T*) mem_alloc(comm_size);


    uint32_t row_index, col_index, leftover_num;
    uint32_t chunk_num, offset;

    //for each target dpu data block, relocate them all somewhere else
    for(int row = 0; row < num_row; row++){
        for(int iter=0; iter < num_comm_dpu / (NR_TASKLETS/2); iter++){

            //read the data from the original address
            original_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (start_offset + (iter * (NR_TASKLETS/2) + tasklet_id) * (comm_size * num_row)) + row * comm_size;
            mram_read((__mram_ptr void const *) (original_addr), word_cache, comm_size);

            //write the data back to the target address
            target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (target_offset + (iter * (NR_TASKLETS/2) + tasklet_id) * comm_size) + row * (comm_size * num_comm_dpu);
            mram_write(word_cache, (__mram_ptr void*) target_addr, comm_size);

            barrier_wait(&my_barrier);
        }
    }

PASS:
    
    printf("\n");
    return 0;
}