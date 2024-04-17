#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <seqread.h>

#include "../support/common.h"

__host dpu_arguments_t DPU_INPUT_ARGUMENTS_A;
__host dpu_arguments_t DPU_INPUT_ARGUMENTS_feat;

T* start_col_partition;

T* cache_feat_1;
T* cache_feat_2;

T* cache_mid_1;
struct elem_t *cur_elem_1;
seqreader_buffer_t cache_elems_1;
seqreader_t sr_elem_1;


T* cache_mid_2;
struct elem_t *cur_elem_2;
seqreader_buffer_t cache_elems_2;
seqreader_t sr_elem_2;

BARRIER_INIT(tasklet_16_barrier, 16);
//two barriers for each read group
BARRIER_INIT(tasklet_8_barrier_1, NR_TASKLETS/2);
BARRIER_INIT(tasklet_8_barrier_2, NR_TASKLETS/2);


/*
 * In this function we are generally doing a SPMM
 * We made a designated global cache for the feature and mid matrices, to make efficient use of MRAM read & write functions
 * After initial setting, the tasklets are divided into two groups, each of which read in feature and Sparse matrix
 * elements, multiply them, and write the result to the designated WRAM space before writing the results to MRAM
 */

int main() {
    uint32_t tasklet_id = me();

    if(tasklet_id == 0) mem_reset();

    barrier_wait(&tasklet_16_barrier);

    //set arguments to use for A
    uint32_t cycle = DPU_INPUT_ARGUMENTS_A.cycle;
    uint32_t tstart_row_A = DPU_INPUT_ARGUMENTS_A.tstart_row;
    uint32_t tstart_col_A = DPU_INPUT_ARGUMENTS_A.tstart_col;
    uint32_t max_rows_per_dpu_A = DPU_INPUT_ARGUMENTS_A.max_rows_per_dpu;
    uint32_t max_nnz_per_dpu_A = DPU_INPUT_ARGUMENTS_A.max_nnz_per_dpu;
    uint32_t num_of_partitions = DPU_INPUT_ARGUMENTS_A.num_of_partitions;
    uint32_t max_rows_per_dpu_w = DPU_INPUT_ARGUMENTS_A.max_cols_per_dpu;
    uint32_t nnz_offset = DPU_INPUT_ARGUMENTS_A.tasklet_group_nnz_offset;


    //set arguments to use for feature matrix
    uint32_t nrows_feat = DPU_INPUT_ARGUMENTS_feat.nrows; 
    uint32_t trows_feat = DPU_INPUT_ARGUMENTS_feat.trows;
    uint32_t tcols_feat = DPU_INPUT_ARGUMENTS_feat.tcols;
    uint32_t tstart_col_feat = DPU_INPUT_ARGUMENTS_feat.tstart_col;
    uint32_t max_rows_per_dpu_feat = DPU_INPUT_ARGUMENTS_feat.max_rows_per_dpu;


    //start addresses in MRAM
    uint32_t mram_base_addr_mid = (uint32_t) (DPU_MRAM_HEAP_POINTER + 2 * max_nnz_per_dpu_A * sizeof(struct elem_t) + max_rows_per_dpu_w * tcols_feat * sizeof(T));
    uint32_t mram_temp_addr_mid;
    uint32_t mram_base_addr_feature = (uint32_t) (mram_base_addr_mid + tcols_feat * max_rows_per_dpu_A * sizeof(T));
    uint32_t mram_temp_addr_feature;
    uint32_t mram_base_addr_elems;
    if (cycle%2 == 1) mram_base_addr_elems = (uint32_t) (DPU_MRAM_HEAP_POINTER);
    else  mram_base_addr_elems = (uint32_t) (DPU_MRAM_HEAP_POINTER + max_nnz_per_dpu_A * sizeof(struct elem_t));

    uint32_t i, j;

    //clean up mram before use
    if(tasklet_id == 0) {
        T *cache_y = mem_alloc(tcols_feat*sizeof(T));

        for(int i=0; i<tcols_feat; i++){
            cache_y[i]=0;
        }

        mram_temp_addr_mid = mram_base_addr_mid;
        uint32_t iter = 0;
        
        iter = ((max_rows_per_dpu_A));

        for(i=0;i<iter;i++){
            mram_write(cache_y, (__mram_ptr void *) (mram_temp_addr_mid), tcols_feat*sizeof(T));
            mram_temp_addr_mid += tcols_feat*sizeof(T);
        }
    }
    barrier_wait(&tasklet_16_barrier);

    uint32_t index;
    //for checking partition data
    uint32_t partition_row_size = nrows_feat/num_of_partitions;

    barrier_wait(&tasklet_16_barrier);


    //define cache for feature
    if(tasklet_id == 0) cache_feat_2 = mem_alloc(20000);
    if(tasklet_id == 0) cache_feat_1 = mem_alloc(20000);

    barrier_wait(&tasklet_16_barrier);
            

    //define cache for mid_result
    if(tasklet_id == 0) cache_mid_2 = mem_alloc(sizeof(T) * tcols_feat);
    //define cache for mid_result
    if(tasklet_id == 0) cache_mid_1 = mem_alloc(sizeof(T) * tcols_feat);

    barrier_wait(&tasklet_16_barrier);


    //initialize sequential_reader for nnzs, for the 2 readers
    if(tasklet_id == 0) cache_elems_2 = seqread_alloc();
    if(tasklet_id == 0) cur_elem_2 = seqread_init(cache_elems_2, (__mram_ptr void *) (mram_base_addr_elems + nnz_offset * sizeof(struct elem_t)), &sr_elem_2);

    if(tasklet_id == 0) cache_elems_1 = seqread_alloc();
    if(tasklet_id == 0) cur_elem_1 = seqread_init(cache_elems_1, (__mram_ptr void *) mram_base_addr_elems, &sr_elem_1);

    

    barrier_wait(&tasklet_16_barrier);

    if(tasklet_id < 8){

        
        uint32_t prev_row = cur_elem_1->rowind; 
        uint32_t temp_feat_diff_neg; // start_col_partition[index] - start_col_partition[index-1]
        uint32_t temp_feat_diff; // start_col_partition[index+1] - start_col_partition[index]
        uint32_t temp_start_col; // start_col_partition[index]

        //set mid result address & read in line from result
        mram_temp_addr_mid = (uint32_t) (mram_base_addr_mid + ((cur_elem_1->rowind - tstart_row_A) * tcols_feat) * sizeof(T));
        if(tasklet_id == 0) mram_read((__mram_ptr void const *) (mram_temp_addr_mid), cache_mid_1, sizeof(T) * tcols_feat);


        //set temp address for feature & cache-in one partition && watch out for data movement over size 2048
        mram_temp_addr_feature = (uint32_t) (mram_base_addr_feature);
        if(tasklet_id == 0) {
            temp_feat_diff = (0 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

            if(sizeof(T) * (temp_feat_diff) * tcols_feat <= 2048)
                mram_read((__mram_ptr void const *) (mram_temp_addr_feature), cache_feat_1, sizeof(T) * (temp_feat_diff) * tcols_feat);
            else{
                for(i=0; i< ((sizeof(T) * (temp_feat_diff)* tcols_feat)/2048 ); i++){
                    mram_read((__mram_ptr void const *) (mram_temp_addr_feature + i*2048), cache_feat_1 + i*2048/sizeof(T), 2048);
                }
                mram_read((__mram_ptr void const *) (mram_temp_addr_feature + i*2048), cache_feat_1 + i*2048/sizeof(T), sizeof(T) * (temp_feat_diff) * tcols_feat - 2048*i);
            }
        }


        //initialize variables for use
        uint32_t diff, m_diff; // offset for getting appropriate address of result
        uint32_t num_of_vals;
        


        //SpMM
        index = 1;
        temp_start_col = (0 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

        temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
        temp_feat_diff_neg = (index-1 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

        barrier_wait(&tasklet_8_barrier_1);

        for(i=0;i<nnz_offset; i++){
            if((cur_elem_1->colind - tstart_col_A) >= temp_start_col){
                //find appropriate index for start_col_partition
                temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                while((cur_elem_1->colind - tstart_col_A) >= temp_start_col + temp_feat_diff){
                    temp_start_col = temp_start_col + temp_feat_diff;
                    index++;
                    temp_feat_diff_neg = temp_feat_diff;
                    temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                } 

                //cache next partition of feature
                mram_temp_addr_feature = (uint32_t) (mram_base_addr_feature + (temp_start_col * tcols_feat) * sizeof(T));

                //read as much data as possible (max 2048 bytes)
                if(tasklet_id == 0) {
                    if((sizeof(T) * (temp_feat_diff) * tcols_feat) <= 2048){
                        mram_read((__mram_ptr void const *) (mram_temp_addr_feature), cache_feat_1, sizeof(T) * (temp_feat_diff) * tcols_feat);
                    }
                    else{
                        for(j=0; j< ((sizeof(T) * (temp_feat_diff)* tcols_feat)/2048); j++){
                            mram_read((__mram_ptr void const *) (mram_temp_addr_feature + j*2048), cache_feat_1 + j*2048/sizeof(T), 2048);
                        }
                        mram_read((__mram_ptr void const *) (mram_temp_addr_feature + j*2048), cache_feat_1 + j*2048/sizeof(T), sizeof(T) * (temp_feat_diff) * tcols_feat - 2048*j);
                    }
                }
                temp_start_col = temp_start_col + temp_feat_diff;
                index++;
                temp_feat_diff_neg = temp_feat_diff;
                temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                barrier_wait(&tasklet_8_barrier_1);
            }
            
            if(tasklet_id == 0){
                if((cur_elem_1->rowind) != prev_row) {
                    prev_row = cur_elem_1->rowind;
                    //write back result row and modify mram_temp_addr for mid matrix.
                    //then read new line from mram
                    mram_write(cache_mid_1, (__mram_ptr void*) (mram_temp_addr_mid), tcols_feat * sizeof(T));
                    mram_temp_addr_mid = (uint32_t) (mram_base_addr_mid + ((cur_elem_1->rowind - tstart_row_A) * tcols_feat) * sizeof(T));
                    mram_read((__mram_ptr void const *) (mram_temp_addr_mid), cache_mid_1, tcols_feat * sizeof(T));
                }
            }
            barrier_wait(&tasklet_8_barrier_1);

            /********************************  calculate each sum  *****************************************/

            if(tcols_feat % (NR_TASKLETS/2) > tasklet_id) {
                diff = (uint32_t) ((((cur_elem_1->colind - tstart_col_A) - (temp_start_col - temp_feat_diff_neg)) * tcols_feat + tasklet_id * (tcols_feat/(NR_TASKLETS/2)) + tasklet_id));
                m_diff = (uint32_t) ((tasklet_id * (tcols_feat/(NR_TASKLETS/2)) + tasklet_id));
                num_of_vals = tcols_feat/(NR_TASKLETS/2) + 1;
            }
            else {
                diff = (uint32_t) ((((cur_elem_1->colind - tstart_col_A) - (temp_start_col - temp_feat_diff_neg)) * tcols_feat + tasklet_id * (tcols_feat/(NR_TASKLETS/2)) + tcols_feat % (NR_TASKLETS/2)));
                m_diff = (uint32_t) ((tasklet_id * (tcols_feat/(NR_TASKLETS/2)) + tcols_feat % (NR_TASKLETS/2)));
                num_of_vals = tcols_feat/(NR_TASKLETS/2);
            }

            for(j=0;j<num_of_vals; j++){
                if(num_of_vals > 0) cache_mid_1[m_diff + j] += cache_feat_1[diff + j] * cur_elem_1->val;
            }
            
            barrier_wait(&tasklet_8_barrier_1);

            if(tasklet_id == 0) cur_elem_1 = seqread_get(cur_elem_1, sizeof(struct elem_t), &sr_elem_1);
            barrier_wait(&tasklet_8_barrier_1);

        }
        if(tasklet_id == 0) mram_write(cache_mid_1, (__mram_ptr void*) (mram_temp_addr_mid), sizeof(T) * tcols_feat);
        barrier_wait(&tasklet_8_barrier_1); 
    }



//for the second group
    else {

        uint32_t prev_row = cur_elem_2->rowind; 
        uint32_t temp_feat_diff_neg;
        uint32_t temp_feat_diff;
        uint32_t temp_start_col;

        //set mid result address & read in line from result
        mram_temp_addr_mid = (uint32_t) (mram_base_addr_mid + ((cur_elem_2->rowind - tstart_row_A) * tcols_feat) * sizeof(T));
        if(tasklet_id == 8) mram_read((__mram_ptr void const *) (mram_temp_addr_mid), cache_mid_2, sizeof(T) * tcols_feat);


        //set temp address for feature & cache-in one partition && watch out for data movement over size 2048
        mram_temp_addr_feature = (uint32_t) (mram_base_addr_feature);
        if(tasklet_id == 8) {
            temp_feat_diff = (0 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

            if(sizeof(T) * (temp_feat_diff) * tcols_feat <= 2048)
                mram_read((__mram_ptr void const *) (mram_temp_addr_feature), cache_feat_2, sizeof(T) * (temp_feat_diff) * tcols_feat);
            else{
                for(i=0; i< ((sizeof(T) * (temp_feat_diff)* tcols_feat)/2048 ); i++){
                    mram_read((__mram_ptr void const *) (mram_temp_addr_feature + i*2048), cache_feat_2 + i*2048/sizeof(T), 2048);
                }
                mram_read((__mram_ptr void const *) (mram_temp_addr_feature + i*2048), cache_feat_2 + i*2048/sizeof(T), sizeof(T) * (temp_feat_diff) * tcols_feat - 2048*i);
            }
        }


        //initialize variables for use
        uint32_t diff, m_diff; // offset for getting appropriate address of result
        uint32_t num_of_vals;
        


        //SpMM
        index = 1;
        temp_start_col = (0 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

        temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
        temp_feat_diff_neg = (index-1 < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);

        barrier_wait(&tasklet_8_barrier_2);

        for(i=nnz_offset; i<max_nnz_per_dpu_A; i++){
            if((cur_elem_2->colind - tstart_col_A) >= temp_start_col){
                //find appropriate index for start_col_partition
                temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                while((cur_elem_2->colind - tstart_col_A) >= temp_start_col + temp_feat_diff){
                    temp_start_col = temp_start_col + temp_feat_diff;
                    index++;
                    temp_feat_diff_neg = temp_feat_diff;
                    temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                } 

                //cache next partition of feature
                mram_temp_addr_feature = (uint32_t) (mram_base_addr_feature + (temp_start_col * tcols_feat) * sizeof(T));

                if(tasklet_id == 8) {
                    
                    if((sizeof(T) * (temp_feat_diff) * tcols_feat) <= 2048){
                        mram_read((__mram_ptr void const *) (mram_temp_addr_feature), cache_feat_2, sizeof(T) * (temp_feat_diff) * tcols_feat);
                    }
                    else{
                        for(j=0; j< ((sizeof(T) * (temp_feat_diff)* tcols_feat)/2048); j++){
                            mram_read((__mram_ptr void const *) (mram_temp_addr_feature + j*2048), cache_feat_2 + j*2048/sizeof(T), 2048);
                        }
                        mram_read((__mram_ptr void const *) (mram_temp_addr_feature + j*2048), cache_feat_2 + j*2048/sizeof(T), sizeof(T) * (temp_feat_diff) * tcols_feat - 2048*j);
                    }
                }
                temp_start_col = temp_start_col + temp_feat_diff;
                index++;
                temp_feat_diff_neg = temp_feat_diff;
                temp_feat_diff = (index < (nrows_feat % num_of_partitions))? (partition_row_size) +1 : (partition_row_size);
                barrier_wait(&tasklet_8_barrier_2);
            }
            
            if(tasklet_id == 8){
                if((cur_elem_2->rowind) != prev_row) {
                    prev_row = cur_elem_2->rowind;
                    //write back result row and modify mram_temp_addr for mid matrix.
                    //then read new line from mram
                    mram_write(cache_mid_2, (__mram_ptr void*) (mram_temp_addr_mid), tcols_feat * sizeof(T));
                    mram_temp_addr_mid = (uint32_t) (mram_base_addr_mid + ((cur_elem_2->rowind - tstart_row_A) * tcols_feat) * sizeof(T));
                    mram_read((__mram_ptr void const *) (mram_temp_addr_mid), cache_mid_2, tcols_feat * sizeof(T));
                }
            }
            barrier_wait(&tasklet_8_barrier_2);

                /********************************  calculate each sum  *****************************************/

            if(tcols_feat % (NR_TASKLETS/2) > (tasklet_id-8)) {
                diff = (uint32_t) ((((cur_elem_2->colind - tstart_col_A) - (temp_start_col - temp_feat_diff_neg)) * tcols_feat + (tasklet_id-8) * (tcols_feat/(NR_TASKLETS/2)) + (tasklet_id-8)));
                m_diff = (uint32_t) (((tasklet_id-8) * (tcols_feat/(NR_TASKLETS/2)) + (tasklet_id-8)));
                num_of_vals = tcols_feat/(NR_TASKLETS/2) + 1;
            }
            else {
                diff = (uint32_t) ((((cur_elem_2->colind - tstart_col_A) - (temp_start_col - temp_feat_diff_neg)) * tcols_feat + (tasklet_id-8) * (tcols_feat/(NR_TASKLETS/2)) + tcols_feat % (NR_TASKLETS/2)));
                m_diff = (uint32_t) (((tasklet_id-8) * (tcols_feat/(NR_TASKLETS/2)) + tcols_feat % (NR_TASKLETS/2)));
                num_of_vals = tcols_feat/(NR_TASKLETS/2);
            }

            for(j=0;j<num_of_vals; j++){
                if(num_of_vals > 0) cache_mid_2[m_diff + j] += cache_feat_2[diff + j] * cur_elem_2->val;
            }
            barrier_wait(&tasklet_8_barrier_2);


            if(tasklet_id == 8) cur_elem_2 = seqread_get(cur_elem_2, sizeof(struct elem_t), &sr_elem_2);
            barrier_wait(&tasklet_8_barrier_2);
        }
        if(tasklet_id == 8) mram_write(cache_mid_2, (__mram_ptr void*) (mram_temp_addr_mid), sizeof(T) * tcols_feat);
        barrier_wait(&tasklet_8_barrier_2); 
    }


    EXIT: 

    printf("\n");

    return 0;

}