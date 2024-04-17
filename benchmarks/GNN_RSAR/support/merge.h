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

//reduce & scatter data in DPUs with communication groups parallel to the x axis 
void reducescatter_x(T** new_mid_cycle, T** partial_mid, uint32_t num_comm_dpu, uint32_t total_data_size, int a, int b, int c){

    //number of elements in the communicating data
    uint32_t num_elem = (total_data_size/num_comm_dpu)/sizeof(T);

    for(int l=0; l<a*b*c; l = l+num_comm_dpu){

        #pragma omp parallel
        #pragma omp for nowait
        //merge the partial matrices together into one
        for(int k=0; k<num_comm_dpu; k++){
            for(int i=0; i<num_elem; i++){
                new_mid_cycle[l+k][i] = 0;
                for(int j=0; j<num_comm_dpu; j++){
                    new_mid_cycle[l+k][i] += partial_mid[l+j][i + k*num_elem];
                }
            }
        }
    }

    return;
}

//reduce & scatter data in DPUs with communication groups parallel to the y axis 
void reducescatter_y(T** new_mid_cycle, T** partial_mid, uint32_t num_comm_dpu, uint32_t total_data_size, int a, int b, int c){

    //number of elements in the communicating data
    uint32_t num_elem = (total_data_size/num_comm_dpu)/sizeof(T);

    for(int l=0; l<a; l++){
        for(int m=0; m<c; m++){
            #pragma omp parallel
            #pragma omp for nowait
            //merge the partial matrices together into one
            for(int k=0; k<num_comm_dpu; k++){
                for(int i=0; i<num_elem; i++){
                    new_mid_cycle[a*b*m + k*a + l][i] = 0;
                    for(int j=0; j<num_comm_dpu; j++){
                        new_mid_cycle[a*b*m + k*a + l][i] += partial_mid[a*b*m + j*a + l][i + k*num_elem];
                    }
                }
            }
        }
    }

    return;
}

#endif