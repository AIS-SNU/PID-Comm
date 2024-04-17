#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <seqread.h>

//#include "../support/common.h"
#include "../../../pidcomm_lib/support/common.h"

__host dpu_arguments_comm_t DPU_INPUT_ARGUMENTS_RS1;

//initialize barrier barrier for 8 tasklets. 
BARRIER_INIT(tasklet_8_barrier, NR_TASKLETS/2);

uint32_t* offset_per_dpu;
uint32_t max_words_per_dpu;

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

    barrier_wait(&tasklet_8_barrier);

    //set arguments for use
    uint32_t start_offset = DPU_INPUT_ARGUMENTS_RS1.start_offset;
    uint32_t target_offset = DPU_INPUT_ARGUMENTS_RS1.target_offset;
    uint32_t total_data_size = DPU_INPUT_ARGUMENTS_RS1.total_data_size;
    uint32_t num_comm_dpu = DPU_INPUT_ARGUMENTS_RS1.num_comm_dpu;
    uint32_t dpu_num = DPU_INPUT_ARGUMENTS_RS1.each_dpu;
    uint32_t no_rotate = DPU_INPUT_ARGUMENTS_RS1.no_rotate;
    uint32_t num_row = DPU_INPUT_ARGUMENTS_RS1.num_row;


    uint32_t original_addr; // address for word to move
    uint32_t target_addr; //address for word to write

    max_words_per_dpu = (total_data_size / (num_comm_dpu * sizeof(T)));


    //cache is used to move one word at a time to the right place
    T* word_cache = (T*) mem_alloc(2048);

    if(tasklet_id==0){
        offset_per_dpu = (uint32_t*) mem_alloc(num_comm_dpu * sizeof(uint32_t));
        for(int i=0; i<num_comm_dpu; i++){
            offset_per_dpu[i] = ( (total_data_size*num_row)/num_comm_dpu ) * i;
        }
    }
    barrier_wait(&tasklet_8_barrier);

    //num of words each tasklet should move
    uint32_t iter;
    if((max_words_per_dpu * sizeof(T)) % 2048 == 0) iter = (max_words_per_dpu * sizeof(T)) / 2048;
    else iter = (max_words_per_dpu * sizeof(T)) / 2048 + 1;

    uint32_t row_index, col_index, leftover_num;
    uint32_t chunk_num, offset;

    //for each target dpu data block, relocate them all somewhere else
    for(int row = 0; row < num_row; row++){
        for(int target_dpu_num = tasklet_id; target_dpu_num < num_comm_dpu; target_dpu_num = target_dpu_num + 8){
            for(int iteration =0; iteration < iter; iteration++){
                //address to read the word from
                original_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (start_offset + row * total_data_size) + max_words_per_dpu * target_dpu_num *sizeof(T) + 2048*iteration;

                //if there are zero_padded words
                if(iteration == iter - 1){
                    leftover_num = (max_words_per_dpu) - (2048/sizeof(T))*iteration;
                    mram_read((__mram_ptr void const *) (original_addr), word_cache, leftover_num * sizeof(T));
                    for(int i= leftover_num; i<(max_words_per_dpu) - (2048/sizeof(T))*iteration; i++){
                        word_cache[i] = 0;
                    }
                    leftover_num = (max_words_per_dpu) - (2048/sizeof(T))*iteration;
                }

                else mram_read((__mram_ptr void const *) (original_addr), word_cache, 2048);

                barrier_wait(&tasklet_8_barrier);

                // write the read words, reordering them as needed
                target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + (target_offset + offset_per_dpu[target_dpu_num] ) + max_words_per_dpu * row *sizeof(T) + 2048*iteration;

                if(iteration == iter - 1) mram_write(word_cache, (__mram_ptr void*) target_addr, leftover_num * sizeof(T));
                else mram_write(word_cache, (__mram_ptr void*) target_addr, 2048);

                barrier_wait(&tasklet_8_barrier);
            }
        }
    }

PASS:
    
    printf("\n");
    return 0;
}