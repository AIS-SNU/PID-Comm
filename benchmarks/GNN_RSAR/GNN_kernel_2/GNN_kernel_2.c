#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <seqread.h>

#include "../support/common.h"

__host dpu_arguments_t DPU_INPUT_ARGUMENTS_mid;
__host dpu_arguments_t DPU_INPUT_ARGUMENTS_weight;

T* cache_weight;


BARRIER_INIT(tasklet_16_barrier, NR_TASKLETS);


/*
 * In this function we are basically doing a GEMM
 * We made a designated global cache for the weight matrix and tasklet-level mid-result caches to make efficient use 
 * of MRAM read & write functions. After initial setting
 */

int main() {
    uint32_t tasklet_id = me();

    if(tasklet_id == 0) mem_reset();

    barrier_wait(&tasklet_16_barrier);

    //set arguments to use for A
    uint32_t nrows_mid = DPU_INPUT_ARGUMENTS_mid.nrows;

    uint32_t max_cols_per_dpu_mid = DPU_INPUT_ARGUMENTS_mid.max_cols_per_dpu;

    uint32_t tstart_row_mid = DPU_INPUT_ARGUMENTS_mid.tstart_row;
    uint32_t max_rows_per_dpu_mid = DPU_INPUT_ARGUMENTS_mid.max_rows_per_dpu;
    //for each tasklet 
    uint32_t start_row = DPU_INPUT_ARGUMENTS_mid.start_row[tasklet_id];
    uint32_t rows_per_tasklet = DPU_INPUT_ARGUMENTS_mid.rows_per_tasklet[tasklet_id];

    uint32_t max_nnz_per_dpu = DPU_INPUT_ARGUMENTS_mid.max_nnz_per_dpu;

    //set arguments to use for feature matrix
    uint32_t ncols_weight = DPU_INPUT_ARGUMENTS_weight.ncols; 

    uint32_t max_rows_per_dpu_weight = DPU_INPUT_ARGUMENTS_weight.max_rows_per_dpu;

    uint32_t tstart_col_weight = DPU_INPUT_ARGUMENTS_weight.tstart_col;

    uint32_t tcols_weight = DPU_INPUT_ARGUMENTS_weight.tcols;

    //start addresses in MRAM

    uint32_t mram_base_addr_weight = (uint32_t) (DPU_MRAM_HEAP_POINTER + 2 * max_nnz_per_dpu * sizeof(struct elem_t));
    uint32_t mram_temp_addr_weight;
    uint32_t mram_base_addr_new_feat = (uint32_t) (mram_base_addr_weight + tcols_weight * max_rows_per_dpu_weight * sizeof(T));
    uint32_t mram_temp_addr_new_feat;
    uint32_t mram_base_addr_mid = (uint32_t) (mram_base_addr_new_feat + (tcols_weight * max_rows_per_dpu_mid * sizeof(T)));
    uint32_t mram_temp_addr_mid;

    uint32_t i, j;

    //zero out all the values pre-written to MRAM
    if(tasklet_id == 0) {
        T *cache_y = mem_alloc(tcols_weight * sizeof(T));

        for(int i=0; i<tcols_weight; i++){
            cache_y[i]=0;
        }

        mram_temp_addr_new_feat = mram_base_addr_new_feat;
        uint32_t iter = 0;
        iter = (max_rows_per_dpu_mid);
        for(i=0;i<iter;i++){
            mram_write(cache_y, (__mram_ptr void *) (mram_temp_addr_new_feat), tcols_weight * sizeof(T));
            mram_temp_addr_new_feat += tcols_weight * sizeof(T);
        }
    }
    barrier_wait(&tasklet_16_barrier);

    //allocate global weight cache
    if(tasklet_id == 0) cache_weight = mem_alloc(9800);
    uint32_t num_cols_per_iteration = 9500/(max_rows_per_dpu_weight * sizeof(T));

    if(num_cols_per_iteration * max_cols_per_dpu_mid * sizeof(T) % 8 != 0){
        if(num_cols_per_iteration * max_cols_per_dpu_mid * sizeof(T) + 8-(num_cols_per_iteration * max_cols_per_dpu_mid * sizeof(T) % 8) > 9500){
            num_cols_per_iteration--;
        }
    }

    if(num_cols_per_iteration > tcols_weight)
        num_cols_per_iteration = tcols_weight;


    //define cache for result to store
    T* cache_new_feat = mem_alloc(2048 + 16);
    uint32_t num_row_per_iteration = 2048/(tcols_weight * sizeof(T));

    if((num_row_per_iteration * tcols_weight * sizeof(T) % 8) != 0){
        if(num_row_per_iteration * tcols_weight * sizeof(T) + 8-(num_row_per_iteration * tcols_weight * sizeof(T) % 8) > 2048){
            num_row_per_iteration--;
        }
    }

    if(num_row_per_iteration > rows_per_tasklet)
        num_row_per_iteration = rows_per_tasklet;
    
    //define cache for matrix to multiply
    T* cache_mid = mem_alloc(sizeof(T) * max_cols_per_dpu_mid * num_row_per_iteration);


    uint32_t index_r;
    uint32_t index_c;
    uint32_t index;
    uint32_t curr_row;
    uint32_t curr_col;
    uint32_t real_num_cols_per_iteration;
    uint32_t real_num_row_per_iteration;

    T mid_sum;

    //to adjust to adressing changes for aligning the data 
    uint32_t mid_aligned=0;
    uint32_t mid_align=0;
    uint32_t weight_aligned=0;
    uint32_t weight_align=0;

    barrier_wait(&tasklet_16_barrier);

    //iterate so that all the rows each tasklet is assigned is all covered
    // all tasklets should go through same number of iterations
    
    for(curr_row = 0; curr_row < DPU_INPUT_ARGUMENTS_mid.rows_per_tasklet[0]; curr_row = curr_row + num_row_per_iteration){

        //cache in mid matrix rows into WRAM, change num_row_per_iteration for last iteration
        mram_temp_addr_mid = (uint32_t) (mram_base_addr_mid + (start_row + curr_row) * max_cols_per_dpu_mid * sizeof(T));

        mram_temp_addr_new_feat = (uint32_t) (mram_base_addr_new_feat + (start_row + curr_row) * tcols_weight * sizeof(T));

        if(mram_temp_addr_new_feat % 8 != 0){
            mid_aligned = (mram_temp_addr_new_feat % 8)/sizeof(T);
            mram_temp_addr_new_feat -= mid_aligned*sizeof(T);
            mid_align=1;
        }
        else {
            mid_aligned=0; mid_align=0;
        }

        if(rows_per_tasklet - curr_row < num_row_per_iteration)
            real_num_row_per_iteration = rows_per_tasklet - curr_row;
        else
            real_num_row_per_iteration = num_row_per_iteration;

        if(real_num_row_per_iteration > 0){
            // to take care of reads over 2048 bytes
            if(sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8 <= 2048)
                mram_read((__mram_ptr void const *) (mram_temp_addr_new_feat), cache_new_feat, sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8);
            else {
                for(i=0; i< ((sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8)/2048 ); i++){
                    mram_read((__mram_ptr void const *) (mram_temp_addr_new_feat + i*2048), cache_new_feat + i*2048/sizeof(T), 2048);
                }
                mram_read((__mram_ptr void const *) (mram_temp_addr_new_feat + i*2048), cache_new_feat + i*2048/sizeof(T), sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * max_cols_per_dpu_mid * real_num_row_per_iteration % 8)) % 8) + mid_align*8 - 2048*i);
            }

            //read in partial line from result
            mram_read((__mram_ptr void const *) (mram_temp_addr_mid), cache_mid, sizeof(T) * max_cols_per_dpu_mid * real_num_row_per_iteration);
        }


        //iterate through the weight matrices
        for(curr_col = 0; curr_col < tcols_weight; curr_col = curr_col + num_cols_per_iteration){

            //reset real number of columns the iteration uses
            if(tcols_weight - curr_col < num_cols_per_iteration)
                real_num_cols_per_iteration = tcols_weight - curr_col;
            else real_num_cols_per_iteration = num_cols_per_iteration;

            //set temp address for feature & cache-in one partition && watch out for data movement over size 2048
            mram_temp_addr_weight = (uint32_t) (mram_base_addr_weight + (curr_col * max_rows_per_dpu_weight) * sizeof(T));

            if(mram_temp_addr_weight % 8 != 0){
                weight_aligned = (mram_temp_addr_weight % 8)/sizeof(T);
                mram_temp_addr_weight -= weight_aligned * sizeof(T);
                weight_align=1;
            }
            else {
                weight_aligned=0; weight_align=0;
            }

            //read the weight matrix into the weight cache
            if(tasklet_id == 0) {
                if(sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight + ((8-(sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight % 8)) % 8)+weight_align*8 <= 2048)
                    mram_read((__mram_ptr void const *) (mram_temp_addr_weight), cache_weight, sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight + ((8-(sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight % 8)) % 8)+weight_align*8);
                else{
                    for(i=0; i< ((sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight)/2048 ); i++){
                        mram_read((__mram_ptr void const *) (mram_temp_addr_weight + i*2048), cache_weight + i*2048/sizeof(T), 2048);
                    }
                    mram_read((__mram_ptr void const *) (mram_temp_addr_weight + i*2048), cache_weight + i*2048/sizeof(T), sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight + ((8-(sizeof(T) * real_num_cols_per_iteration * max_rows_per_dpu_weight % 8)) % 8)+weight_align*8-2048*i);
                }
            }

            barrier_wait(&tasklet_16_barrier);

            //calculate result
            for(index_r = 0; index_r < real_num_row_per_iteration; index_r++){
                for(index_c = 0; index_c < real_num_cols_per_iteration; index_c++){
                    mid_sum = 0;
                    for(index = 0; index < max_rows_per_dpu_weight; index++){
                        mid_sum += cache_mid[index_r * max_rows_per_dpu_weight + index + mid_aligned] * cache_weight[index_c * max_rows_per_dpu_weight + index+ weight_aligned];
                    }
                    cache_new_feat[index_r * tcols_weight + (curr_col+index_c)] = mid_sum;
                }
            }

            barrier_wait(&tasklet_16_barrier);
        }
        // write back result to mram
        if(real_num_row_per_iteration > 0)
            if(sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8 <= 2048)
                mram_write(cache_new_feat, (__mram_ptr void*) (mram_temp_addr_new_feat), sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8);
            else {
                for(i=0; i< ((sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * tcols_weight * real_num_row_per_iteration % 8)) % 8) + mid_align*8)/2048 ); i++){
                    mram_write(cache_new_feat + i*2048/sizeof(T), (__mram_ptr void const *) (mram_temp_addr_new_feat + i*2048), 2048);
                }
                mram_write(cache_new_feat + i*2048/sizeof(T), (__mram_ptr void const *) (mram_temp_addr_new_feat + i*2048), sizeof(T) * tcols_weight * real_num_row_per_iteration + ((8-(sizeof(T) * max_cols_per_dpu_mid * real_num_row_per_iteration % 8)) % 8) + mid_align*8 - 2048*i);
            }
        barrier_wait(&tasklet_16_barrier);
    }

    EXIT: 

    printf("\n");

    return 0;

}