#ifndef _COMMON_H_
#define _COMMON_H_

// Data type
#ifdef UINT32
#define T uint32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT32
#define T int32_t
#define byte_dt 4
#define DIV 2 // Shift right to divide by sizeof(T)
#elif INT16
#define T int16_t
#define byte_dt 2
#define DIV 1 // Shift right to divide by sizeof(T)
#elif INT8
#define T int8_t
#define byte_dt 1
#define DIV 0 // Shift right to divide by sizeof(T)
#elif INT64
#define T int64_t
#define byte_dt 8
#define DIV 3 // Shift right to divide by sizeof(T)
#endif

/* Structures used by both the host and the dpu to communicate information */
typedef struct {
    uint32_t cycle;

    uint32_t nrows;
    uint32_t tcols;
    uint32_t tstart_row;
    uint32_t max_rows_per_dpu;

    uint32_t max_nnz_per_dpu;
    uint32_t num_of_partitions;

    //for caching fractions of weight matrix
    uint32_t tasklet_group_nnz_offset;

    uint32_t start_row[NR_TASKLETS];
    uint32_t rows_per_tasklet[NR_TASKLETS];

    uint32_t ncols; //number of columns
    uint32_t trows; //number of rows
    uint32_t tstart_col;
    uint32_t max_cols_per_dpu;
} dpu_arguments_t;

//element of COO matrix
struct elem_t {
    uint32_t rowind;
    uint32_t colind;
    T val;
};

//info for each DPU
struct dpu_info_t {
    uint32_t rows_per_dpu;
    uint32_t rows_per_dpu_pad;
    uint32_t prev_rows_dpu;
    uint32_t cols_per_dpu;
    uint32_t cols_per_dpu_pad;
    uint32_t prev_cols_dpu;
    uint32_t prev_nnz_dpu;
    uint32_t nnz;
    uint32_t nnz_pad;
};

//matrix partition info - on which row/column the split is
struct partition_info_t {
    uint32_t *COO_row_split;
    uint32_t *COO_col_split;
    uint32_t *feat_row_split;
    uint32_t *mid_row_split;
    uint32_t *w_col_split;

    //for caching fractions of feature matrix
    uint32_t num_of_partitions;

    uint32_t *row_split_tasklet;
};

struct COOMatrix {
    uint32_t nrows;
    uint32_t ncols;
    uint32_t nnz;
    
    //partition for storing number of nnzs for each partition
    uint32_t *partitions;
    uint32_t **rows;
    struct elem_t *nnzs;
};

//matrices other than COOMatrix ( for feature, mid, weight, result(new feature) )
// for mid, the values are in row sequence (val[row_ind * ncols + col_ind]) 
// for feature and weight, the values are in column sequence ( val[col_ind * nrows + row_ind])
struct Matrix {
    uint32_t nrows;
    uint32_t ncols;
    T *val; 
};




#endif