#ifndef _PARTITION_H_
#define _PARTITION_H_

#include "common.h"

struct partition_info_t *partition_init(uint32_t nr_of_partitions, uint32_t nr_of_tasklets){
    struct partition_info_t *partition_info;
    partition_info = (struct partition_info_t *) malloc(sizeof(struct partition_info_t));

    partition_info->COO_row_split = (uint32_t *) malloc((nr_of_partitions + 1) * sizeof(uint32_t));
    partition_info->COO_col_split = (uint32_t *) malloc((nr_of_partitions + 1) * sizeof(uint32_t));
    partition_info->feat_row_split = (uint32_t *) malloc((nr_of_partitions + 1) * sizeof(uint32_t));
    partition_info->mid_row_split = (uint32_t *) malloc((nr_of_partitions + 1) * sizeof(uint32_t));
    partition_info->w_col_split = (uint32_t *) malloc((nr_of_partitions + 1) * sizeof(uint32_t));

    partition_info->row_split_tasklet = (uint32_t *) malloc((nr_of_tasklets + 1) * sizeof(uint32_t));

    return partition_info;
}

void COO_partition(struct COOMatrix *COOMtx, struct partition_info_t *partition_info, uint32_t nr_of_partitions){

    // Compute the matrix splits.
    uint32_t chunks_col = COOMtx->ncols / nr_of_partitions;
    uint32_t rest_cols = COOMtx->ncols % nr_of_partitions;
    uint32_t cols_per_dpu;
    uint32_t curr_col = 0;
    uint32_t i;

    partition_info->COO_col_split[0] = curr_col;
    for(i=0; i<nr_of_partitions; i++){
        //set cols_per_dpu
        cols_per_dpu = chunks_col;
        if(i<rest_cols) cols_per_dpu++;

        curr_col +=cols_per_dpu;

        if(curr_col > COOMtx->ncols) curr_col = COOMtx->ncols;

        partition_info->COO_col_split[i+1] = curr_col;
    }

    uint32_t chunks_row = COOMtx->nrows / nr_of_partitions;
    uint32_t rest_rows = COOMtx->nrows % nr_of_partitions;
    uint32_t rows_per_dpu;
    uint32_t curr_row = 0;

    partition_info->COO_row_split[0] = curr_row;
    for(i=0; i<nr_of_partitions; i++){
        //set cols_per_dpu
        rows_per_dpu = chunks_row;
        if(i<rest_rows) rows_per_dpu++;

        curr_row +=rows_per_dpu;

        if(curr_row > COOMtx->nrows) curr_row = COOMtx->nrows;

        partition_info->COO_row_split[i+1] = curr_row;
    }
}

void mid_partition_by_row(struct Matrix *mat, struct partition_info_t *partition_info, uint32_t nr_of_partitions){
    // Compute the matrix splits.
    uint32_t chunks = mat->nrows / nr_of_partitions;
    uint32_t rest_rows = mat->nrows % nr_of_partitions;
    uint32_t rows_per_dpu;
    uint32_t curr_row = 0;
    uint32_t i;

    partition_info->mid_row_split[0] = curr_row;
    for(i=0; i<nr_of_partitions; i++){
        //set rows_per_dpu
        rows_per_dpu = chunks;
        if(i<rest_rows) rows_per_dpu++;

        curr_row +=rows_per_dpu;

        if(curr_row > mat->nrows) curr_row = mat->nrows;

        partition_info->mid_row_split[i+1] = curr_row;
    }
}

void feat_partition_by_row(struct Matrix *mat, struct partition_info_t *partition_info, uint32_t nr_of_partitions){
    // Compute the matrix splits.
    uint32_t chunks_row = mat->nrows / nr_of_partitions;
    uint32_t rest_rows = mat->nrows % nr_of_partitions;
    uint32_t rows_per_dpu;
    uint32_t curr_row = 0;
    uint32_t i;

    partition_info->feat_row_split[0] = curr_row;
    for(i=0; i<nr_of_partitions; i++){
        //set rows_per_dpu
        rows_per_dpu = chunks_row;
        if(i<rest_rows) rows_per_dpu++;

        curr_row +=rows_per_dpu;

        if(curr_row > mat->nrows) curr_row = mat->nrows;

        partition_info->feat_row_split[i+1] = curr_row;
    }
}

void w_partition_by_col(struct Matrix *mat, struct partition_info_t *partition_info, uint32_t nr_of_partitions){

    // Compute the matrix splits.
    uint32_t chunks = mat->ncols / nr_of_partitions;
    uint32_t rest_cols = mat->ncols % nr_of_partitions;
    uint32_t cols_per_dpu;
    uint32_t curr_col = 0;
    uint32_t i;

    partition_info->w_col_split[0] = curr_col;
    for(i=0; i<nr_of_partitions; i++){
        //set cols_per_dpu
        cols_per_dpu = chunks;
        if(i<rest_cols) cols_per_dpu++;

        curr_col +=cols_per_dpu;

        if(curr_col > mat->ncols) curr_col = mat->ncols;

        partition_info->w_col_split[i+1] = curr_col;
    }
}

//decide how many rows each tasklet should take
void partition_by_row(struct partition_info_t *partition_info, int rows_per_dpu, int nr_of_tasklets) {

    // Compute the matrix splits.
    uint32_t chunks = rows_per_dpu / nr_of_tasklets;  //minimum num of rows each tasklet should take on
    uint32_t rest_rows = rows_per_dpu % nr_of_tasklets;
    uint32_t rows_per_tasklet; // final number of rows each tasklet should take
    uint32_t curr_row = 0;

    partition_info->row_split_tasklet[0] = curr_row;
    for(unsigned int i=0; i < nr_of_tasklets; i++) {
        rows_per_tasklet = chunks;
        if (i < rest_rows)
            rows_per_tasklet += 1;
        curr_row += rows_per_tasklet;
        if (curr_row > rows_per_dpu)
            curr_row = rows_per_dpu;
        partition_info->row_split_tasklet[i+1] = curr_row;
    }
}

/*
//decide how many cols each partition should take
void partition_by_col(struct partition_info_t *partition_info, int cols_per_dpu, int nr_of_tasklets) {

    // Compute the matrix splits.
    uint32_t chunks = cols_per_dpu / nr_of_tasklets;  //minimum num of cols each tasklet should take on
    uint32_t rest_cols = cols_per_dpu % nr_of_tasklets;
    uint32_t cols_per_tasklet; // final number of cols each tasklet should take
    uint32_t curr_col= 0;

    partition_info->col_split_tasklet[0] = curr_col;
    for(unsigned int i=0; i < nr_of_tasklets; i++) {
        cols_per_tasklet = chunks;
        if (i < rest_cols)
            cols_per_tasklet += 1;
        curr_col += cols_per_tasklet;
        if (curr_col > cols_per_dpu)
            curr_col = cols_per_dpu;
        partition_info->col_split_tasklet[i+1] = curr_col;
    }
}*/

#endif