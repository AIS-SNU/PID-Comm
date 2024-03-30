/* Copyright 2024 AISys. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
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

//initialize tasklets. each are barriers for 8, 4, and 2 tasklets. 
BARRIER_INIT(tasklet_8_barrier, NR_TASKLETS/2);
BARRIER_INIT(tasklet_4_barrier, NR_TASKLETS/4);
BARRIER_INIT(tasklet_2_barrier, NR_TASKLETS/8);

uint32_t* words_per_dpu;
uint32_t* words;
uint32_t max_words_per_dpu;

/*
 * In this function we aim to reorder the target data we are using for
 * communication. The data will be ordered to correctly order for
 * AlltoAll communication when x-axis length is 2 & y-axis length is 4.
 * Rotation scheme is different as x-axis length is 2.
 * Rotating scheme is decided on if x-axis is included in communication,
 * and if number of PEs participating in communication in each entangled group 
 * is smaller than 8 or not
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
    uint32_t comm_type = DPU_INPUT_ARGUMENTS_RS1.comm_type; //whether the communication contains the x axis
    uint32_t a_length = DPU_INPUT_ARGUMENTS_RS1.a_length; //length of the x-axis
    uint32_t num_comm_rg = DPU_INPUT_ARGUMENTS_RS1.num_comm_rg; // number of PEs participating in communication in each entangled group 

    uint32_t original_addr; // address for word to move
    uint32_t target_addr; //address for word to 


    if(!comm_type){

        if(tasklet_id >= num_comm_rg) goto PASS;

        max_words_per_dpu = (total_data_size / (num_comm_dpu * sizeof(T)));
    
        //cache is used to move one word at a time to the right place
        T* word_cache = (T*) mem_alloc(2048);

        //num of words each tasklet should move
        uint32_t iter;
        if((max_words_per_dpu * sizeof(T)) % 2048 == 0) iter = (max_words_per_dpu * sizeof(T)) / 2048;
        else iter = (max_words_per_dpu * sizeof(T)) / 2048 + 1;

        uint32_t row_index, col_index, leftover_num;
        int offset;

        //for each target dpu data block, relocate them all somewhere else
        for(int target_dpu_num = tasklet_id; target_dpu_num < num_comm_dpu; target_dpu_num = target_dpu_num + num_comm_rg){
            for(int iteration =0; iteration < iter; iteration++){
                //address to read the word from
                original_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + start_offset + max_words_per_dpu * target_dpu_num *sizeof(T) + 2048*iteration;

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

                if(num_comm_rg == 4) barrier_wait(&tasklet_4_barrier);
                else if(num_comm_rg == 2) barrier_wait(&tasklet_2_barrier);

                if(no_rotate){
                    target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + target_offset + max_words_per_dpu * target_dpu_num*sizeof(T) + 2048*iteration;
                }
                else{
                    offset = (dpu_num - target_dpu_num) % num_comm_rg;
                    target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + target_offset + (target_dpu_num/num_comm_rg) * num_comm_rg * max_words_per_dpu * sizeof(T) + offset * max_words_per_dpu * sizeof(T) + 2048*iteration;
                }
                if(iteration == iter - 1) mram_write(word_cache, (__mram_ptr void*) target_addr, leftover_num * sizeof(T));
                else mram_write(word_cache, (__mram_ptr void*) target_addr, 2048);

                if(num_comm_rg == 4) barrier_wait(&tasklet_4_barrier);
                else if(num_comm_rg == 2) barrier_wait(&tasklet_2_barrier);
            }
        }
    }

    else if(comm_type){

        if(tasklet_id >= num_comm_rg) goto PASS;

        max_words_per_dpu = (total_data_size / (num_comm_dpu * sizeof(T)));
    
        //cache is used to move one word at a time to the right place
        T* word_cache = (T*) mem_alloc(2048);

        //num of words each tasklet should move
        uint32_t iter;
        if((max_words_per_dpu * sizeof(T)) % 2048 == 0) iter = (max_words_per_dpu * sizeof(T)) / 2048;
        else iter = (max_words_per_dpu * sizeof(T)) / 2048 + 1;

        uint32_t row_index, col_index, leftover_num;
        int offset;

        //for each target dpu data block, relocate them all somewhere else
        for(int target_dpu_num = tasklet_id; target_dpu_num < num_comm_dpu; target_dpu_num = target_dpu_num + num_comm_rg){
            for(int iteration =0; iteration < iter; iteration++){
                //address to read the word from
                original_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + start_offset + max_words_per_dpu * target_dpu_num *sizeof(T) + 2048*iteration;

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


                if(no_rotate){
                    target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + target_offset + max_words_per_dpu * target_dpu_num*sizeof(T) + 2048*iteration;
                }
                else{
                    offset = (dpu_num/(8/num_comm_rg) - target_dpu_num) % num_comm_rg;
                    target_addr = (uint32_t) DPU_MRAM_HEAP_POINTER + target_offset + (target_dpu_num/num_comm_rg) * num_comm_rg * max_words_per_dpu * sizeof(T) + offset * max_words_per_dpu * sizeof(T) + 2048*iteration;
                }
                if(iteration == iter - 1) mram_write(word_cache, (__mram_ptr void*) target_addr, leftover_num * sizeof(T));
                else mram_write(word_cache, (__mram_ptr void*) target_addr, 2048);
            }
        }
    }

PASS:
    
    printf("\n");
    return 0;
}