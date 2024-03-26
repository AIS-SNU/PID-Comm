#ifndef _MERGE_H_
#define _MERGE_H_

#include <stdio.h>
#include <immintrin.h>
#include <omp.h>

#include "common.h"


void reducescatter_x(T** new_mid_cycle, T** partial_mid, uint32_t num_comm_dpu, uint32_t total_data_size, int a, int b, int c){

    uint32_t num_elem = (total_data_size/num_comm_dpu)/sizeof(T);

    
    for(int l=0; l<a*b*c; l = l+num_comm_dpu){

        #pragma omp parallel
        #pragma omp for nowait
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

void reducescatter_y(T** new_mid_cycle, T** partial_mid, uint32_t num_comm_dpu, uint32_t total_data_size, int a, int b, int c){

    uint32_t num_elem = (total_data_size/num_comm_dpu)/sizeof(T);

    
    for(int l=0; l<a; l++){
        for(int m=0; m<c; m++){
            #pragma omp parallel
            #pragma omp for nowait
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


void allreduce_x(T** new_feat_cycle, T** partial_feat, uint32_t num_comm_dpu, uint32_t total_data_size, int a, int b, int c){
    
    uint32_t num_elem = (total_data_size)/sizeof(T);
    
    for(int l=0; l<a*b*c; l = l+num_comm_dpu){
        #pragma omp parallel
        #pragma omp for nowait
        for(int i=0; i<num_elem; i++){
            new_feat_cycle[l][i] = 0;
            for(int j=0; j<num_comm_dpu; j++){
                new_feat_cycle[l/num_comm_dpu][i] += partial_feat[l+j][i];
            }
        }
    }
    return;
}



#endif