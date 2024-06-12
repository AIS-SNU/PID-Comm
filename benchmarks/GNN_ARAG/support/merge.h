#ifndef _MERGE_H_
#define _MERGE_H_

#include <stdio.h>
#include <immintrin.h>
#include <omp.h>

#include "common.h"
#include "partition.h"


//reduce data in DPUs with communication groups parallel to the x axis 
void allreduce_x(T** new_feat_cycle, T** partial_feat, uint32_t nr_of_partitions, uint32_t nr_of_dpus, uint32_t max_rows_per_dpu, uint32_t ncols){

    #pragma omp parallel
    #pragma omp for nowait

    //merge the partial matrices together into one
    for(int i=0; i<nr_of_partitions; i++){
        for(unsigned int row = 0; row < max_rows_per_dpu; row++){
            for(unsigned int col = 0; col < ncols ; col++){
                new_feat_cycle[i][row * ncols + col]=0;
                for(int j=0; j<nr_of_partitions; j++){ 
                    new_feat_cycle[i][row * ncols + col] += partial_feat[j+i*nr_of_partitions][row * ncols + col];            
                }
            }
        }
    }

    return;
}

//reduce data in DPUs with communication groups parallel to the y axis 
void allreduce_y(T** new_feat_cycle, T** partial_feat, uint32_t nr_of_partitions, uint32_t nr_of_dpus, uint32_t max_rows_per_dpu, uint32_t ncols){

    #pragma omp parallel
    #pragma omp for nowait

    //merge the partial matrices together into one
    for(int i=0; i<nr_of_partitions; i++){
        for(unsigned int row = 0; row < max_rows_per_dpu; row++){
            for(unsigned int col = 0; col < ncols ; col++){
                new_feat_cycle[i][row * ncols + col]=0;
                for(int j=0; j<nr_of_partitions; j++){ 
                    new_feat_cycle[i][row * ncols + col] += partial_feat[i+j*nr_of_partitions][row * ncols + col];            
                }
            }
        }
    }

    return;
}


void gather_x(T** new_mid_cycle, T** partial_mid, uint32_t nr_of_partitions, struct dpu_info_t *dpu_info_w,
                    uint32_t max_rows_per_dpu_A, uint32_t max_rows_per_dpu_mid, uint32_t max_cols_per_dpu_w, uint32_t ncols){

    int curr_row;

    #pragma omp parallel
    #pragma omp for nowait
    for(int k=0; k<nr_of_partitions; k++){
        for(int l=0;l<nr_of_partitions;l++){
            int i = k*nr_of_partitions + l;
            for(unsigned int row = 0; row < max_rows_per_dpu_A; row++){
                for(unsigned int col = 0; col < dpu_info_w[i].cols_per_dpu; col++){
                    new_mid_cycle[k][row * ncols + (dpu_info_w[i].prev_cols_dpu + col)] = partial_mid[i][row * max_cols_per_dpu_w + col];            
                }
            }
        }
    }
    return;
}




#endif