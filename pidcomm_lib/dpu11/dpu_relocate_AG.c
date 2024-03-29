#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <seqread.h>

#include "../support/common.h"

#define COMM_UNIT_SIZE 8

__host dpu_arguments_comm_t DPU_INPUT_ARGUMENTS_RS1;

//initialize barrier barrier for 8 tasklets. 
BARRIER_INIT(tasklet_8_barrier, NR_TASKLETS/2);

uint32_t* offset_per_dpu;

/*
 * In this function we aim to reorder the target data we are using for
 * communication. The data will be ordered in the following order
 * DPU 0's Word#1, DPU 1's Word#1, ... DPU N-1's Word#1, DPU 0's word#2 ...  for DPU 0,
 * DPU 1's word#1, DPU 2's Word#1, ... DPU 7's Word#1, DPU 0's Word#1, ... for DPU 1 
 * and so on. Missing words will be filled with a 0 for alignment.
 *
 * We are going to 
 * 
 */

int main(){

    uint32_t tasklet_id = me();

    if(tasklet_id >= 8) goto PASS;

    if(tasklet_id == 0) mem_reset();

    barrier_wait(&tasklet_8_barrier);

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
            //address to read the word from
            original_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (start_offset + (iter * (NR_TASKLETS/2) + tasklet_id) * (comm_size * num_row)) + row * comm_size;

            mram_read((__mram_ptr void const *) (original_addr), word_cache, comm_size);

            target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (target_offset + (iter * (NR_TASKLETS/2) + tasklet_id) * comm_size) + row * (comm_size * num_comm_dpu);

            mram_write(word_cache, (__mram_ptr void*) target_addr, comm_size);

            barrier_wait(&tasklet_8_barrier);
        }
    }

PASS:
    
    printf("\n");
    return 0;
}