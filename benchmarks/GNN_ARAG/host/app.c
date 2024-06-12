#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_log.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>
#include <omp.h>

#include "../support/common.h"
#include "../support/matrix.h"
#include "../support/partition.h"
#include "../support/merge.h"

//For supported communication primitives.
#include <pidcomm.h>

// Define the path of kernels to use here.
#ifndef GNN_KERNEL_1
#define GNN_KERNEL_1 "./bin/GNN_kernel_1"
#endif

#ifndef GNN_KERNEL_2
#define GNN_KERNEL_2 "./bin/GNN_kernel_2"
#endif

#ifndef DATA_RELOCATE_AG
#define DATA_RELOCATE_AG "./bin/data_relocate_AG"
#endif

//total capacity of each DPU
#define DPU_CAPACITY (63 << 20)

/*
 * 1. Coo Matrix
 * 2. Feature Matrix -> later reused to contain new feature
 * 3. Mid_result
 * 4. Weight Matrix
 * 5. Partitioning data
 * 6. Partial matrices for mid
 * 7. Partial matrices for new F
 */

static struct COOMatrix *B;
static struct COOMatrix *A;
static struct Matrix *feature;
static struct Matrix *mid;
static struct Matrix *weight;
static struct Matrix *weight_2;

static T **new_feat_cycle;
static T **new_mid_cycle;

static struct partition_info_t *partition_info;
static T **partial_mid;
static T **partial_feat;


uint32_t cycle_num=3;

int transpose_num(int num, uint32_t nr_of_partitions){
    int a = num/nr_of_partitions;
    int b = num%nr_of_partitions;
    return (b*nr_of_partitions + a);
}


static void GNN_host_mid(struct COOMatrix *A, struct Matrix *feature, T *Mid){

    //reset Mid for storing new values
    for(unsigned int row=0; row<A->nrows; row++){
        for(unsigned int col=0; col<feature->ncols; col++){
            Mid[row*feature->ncols + col] = 0;
        }
    }

#pragma omp parallel for num_threads(12)
    for(unsigned int n = 0; n < A->nnz; n++){
        for(unsigned int col = 0; col < feature->ncols; col++){
            Mid[(A->nnzs[n].rowind * feature->ncols + col)] += feature->val[A->nnzs[n].colind * feature->ncols + col] * A->nnzs[n].val;
        }
        //if(A->nnzs[n].rowind == 0 + 342*1 + 342*0) printf("rowind : %d, colind : %d, val : %d, feature_val : %d\n", A->nnzs[n].rowind, A->nnzs[n].colind, A->nnzs[n].val, feature->val[A->nnzs[n].colind * feature->ncols]);
    }
}

static void GNN_host_mid_2(struct COOMatrix *A, struct Matrix *feature, T *Mid){


    //reset Mid for storing new values
    for(unsigned int row=0; row<A->nrows; row++){
        for(unsigned int col=0; col<feature->ncols; col++){
            Mid[row*feature->ncols + col] = 0;
        }
    }
#pragma omp parallel for num_threads(12)
    for(unsigned int n = 0; n < A->nnz; n++){
        for(unsigned int col = 0; col < feature->ncols; col++){
            Mid[(A->nnzs[n].rowind * feature->ncols + col)] += feature->val[A->nnzs[n].colind * feature->ncols + col] * A->nnzs[n].val;
            //if(A->nnzs[n].rowind == 12 && feature->val[A->nnzs[n].colind * feature->ncols + col] != 0) printf("feat_val : %d\n",feature->val[A->nnzs[n].colind * feature->ncols + col]);
        }
        //if(A->nnzs[n].rowind == 12) printf("rowind : %d, colind : %d, val : %d\n", A->nnzs[n].rowind, A->nnzs[n].colind, A->nnzs[n].val);
    }
}

static void GNN_host_rest(struct Matrix *y, T *Mid, struct Matrix *weight){


    //reset y values for storing new values
    for(unsigned int row = 0; row < y->nrows; row++){
        for(unsigned int col = 0; col < y->ncols; col++){
            y->val[row * (y->ncols) + col] = 0;
        }
    }
#pragma omp parallel for num_threads(12)
    for(unsigned int row = 0; row < y->nrows; row++){
        for(unsigned int col = 0; col < y->ncols; col++){
            for(int i = 0; i < y->ncols; i++){
                y->val[row * (y->ncols) + col] += Mid[row * (y->ncols) + i] * weight->val[col * (y->ncols) + i];
            }
        }
    }
}

//must receive num of rank as input
int main(int argc, char **argv) {

    //for time measurments
    Timer timer;
    
    float total_time;

    uint32_t nr_dpus = atoi(argv[1]);
    uint32_t nr_partition = sqrt(nr_dpus);
    uint32_t PIDComm_lib = atoi(argv[4]);

    char *abs_dir = (char *) malloc(1024);
    abs_dir = getcwd(abs_dir, 1024);
    char* fileName =  strcat(strcat(strcat(abs_dir, (char *)"/inputs/"), (char *)argv[2]), ".mtx");
    printf("%s, done!\n", fileName);

    struct dpu_set_t dpu_set, dpu;

    //num of dpu ranks
    uint32_t nr_of_dpus = nr_dpus;

    //number of partitions => square root of number of dpus
    uint32_t nr_of_partitions = nr_partition;


    /**************************SETTINGS FOR AR_AG***********************************/

    dpu_arguments_comm_t* dpu_argument = (dpu_arguments_comm_t*) malloc(sizeof(dpu_arguments_comm_t) * nr_dpus);

    uint32_t total_data_size;

    uint32_t start_offset;
    uint32_t target_offset;
    uint32_t buffer_offset;
    uint32_t data_size_per_iter;
    uint32_t num_comm_dpu = nr_partition;
    uint32_t each_dpu;

     //set hypercube configuration
    uint32_t dimension=3;
    uint32_t axis_len[dimension]; //The number of DPUs for each axis of the hypercube
    axis_len[0]=nr_partition; //x-axis
    axis_len[1]=nr_partition; //y-axis
    axis_len[2]=1;  //z-axis

    //Allocate DPUs and load binary
    DPU_ASSERT(dpu_alloc_comm(nr_dpus, NULL, &dpu_set, 1));
    DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_1, NULL));
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("[INFO] Allocated %d DPU(s)\n", nr_of_dpus);
    printf("[INFO] Allocated %d TASKLET(s) per DPU\n", NR_TASKLETS);

    //Set hypercube configuration
    hypercube_manager* hypercube_manager = init_hypercube_manager(dpu_set, dimension, axis_len);

    /*******************************************************************************/

    /*
     * 1. Read given COOMatrix
     * 2. Partition the matrix
     * 3. initialize containers for matrices
     */

    //partition_info
    partition_info = partition_init(nr_of_partitions, NR_TASKLETS);

    //create matrices => fix in case of size change
    A = readCOOMatrix(fileName, nr_of_dpus, nr_of_partitions, partition_info);
    B = copy_COOMatrix(A, nr_of_dpus); // make function to copy A for later use
    feature = create_matrix(A->ncols, atoi(argv[3]), 1);


    //read or create matrix W ==> TO DO!!! 사이즈는 어떻게 할지 추후 결정,,,
    weight = create_matrix(feature->ncols, feature->ncols, 0);

    weight_2 = create_matrix(feature->ncols, feature->ncols, 0); //for use in later cycles

    /* uint32_t padding = ((8/byte_dt) - feature->ncols % (8/byte_dt));

    if(padding % (8/byte_dt) != 0){
        zeropad_feat(feature, padding, 1);
        zeropad_weight(weight, padding, 1);
        zeropad_weight(weight_2, padding, 1);
    } */


    mid = malloc(2*sizeof(uint32_t) + sizeof(T*));
    mid->nrows = A->nrows;
    mid->ncols = feature->ncols;

    //compute mid result for correctness
    T *y_host = (T *) calloc((A->nrows * feature->ncols), sizeof(T)); 
    GNN_host_mid(B, feature, y_host); 


    struct dpu_info_t *dpu_info_A = (struct dpu_info_t *) malloc(nr_of_dpus * sizeof(struct dpu_info_t));
    struct dpu_info_t *dpu_info_feat = (struct dpu_info_t *) malloc(nr_of_dpus * sizeof(struct dpu_info_t));
    struct dpu_info_t *dpu_info_w =(struct dpu_info_t *) malloc(nr_of_dpus * sizeof(struct dpu_info_t));
    struct dpu_info_t *dpu_info_mid = (struct dpu_info_t *) malloc(nr_of_dpus * sizeof(struct dpu_info_t));
    dpu_arguments_t *input_args_A = (dpu_arguments_t *) malloc(nr_of_dpus * sizeof(dpu_arguments_t));
    dpu_arguments_t *input_args_feat = (dpu_arguments_t *) malloc(nr_of_dpus * sizeof(dpu_arguments_t));
    dpu_arguments_t *input_args_w = (dpu_arguments_t *) malloc(nr_of_dpus * sizeof(dpu_arguments_t));
    dpu_arguments_t *input_args_mid = (dpu_arguments_t *) malloc(nr_of_dpus * sizeof(dpu_arguments_t));

    uint64_t max_cols_per_dpu_A = 0;
    uint64_t max_rows_per_dpu_A = 0;
    uint64_t max_rows_per_dpu_feat = 0;
    uint64_t max_rows_per_dpu_mid = 0;
    uint64_t max_cols_per_dpu_w = 0;


    uint64_t max_rows_per_tasklet_mid = 0; 

    uint64_t max_nnz_per_dpu = 0; //nnz counting only for A

    unsigned int i=0;
    unsigned int j=0;
    unsigned int k=0;

    //initialize partition_data

    //partition the matrices
    feat_partition_by_row(feature, partition_info, nr_of_partitions);
    w_partition_by_col(weight, partition_info, nr_of_partitions);
    mid_partition_by_row(mid, partition_info, nr_of_partitions);


    //find padding for rows and non-zero elements needed for each CPU-DPU transfer - for input sparse matrix A and feature matrix 
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = (i%nr_of_partitions);
        j = (i/nr_of_partitions);

        uint32_t cols_per_dpu = partition_info->COO_col_split[k+1] - partition_info->COO_col_split[k];
        uint32_t prev_cols_dpu = partition_info->COO_col_split[k];

        uint32_t rows_per_dpu = partition_info->COO_row_split[j+1] - partition_info->COO_row_split[j];
        uint32_t prev_rows_dpu = partition_info->COO_row_split[j];

        if(cols_per_dpu > max_cols_per_dpu_A){
            max_cols_per_dpu_A = cols_per_dpu;
        }

        if(rows_per_dpu > max_rows_per_dpu_A){
            max_rows_per_dpu_A = rows_per_dpu;
        }

        //check padding needed for transfering nnzs
        unsigned int nnz = 0, nnz_pad;
        nnz = A->partitions[i];
        if(nnz % (8 / byte_dt) != 0){
            nnz_pad = nnz + ((8 / byte_dt) - (nnz % (8 / byte_dt)));
        }
        else {
            nnz_pad = nnz;
        }

        if (nnz_pad > max_nnz_per_dpu){
            max_nnz_per_dpu = nnz_pad;
        }

        uint32_t prev_nnz_dpu = 0;
        for(unsigned int r = 0; r < i; r++){
            prev_nnz_dpu += A->partitions[r];
        }

        // Keep information per DPU
        dpu_info_A[i].cols_per_dpu = cols_per_dpu;
        dpu_info_A[i].prev_cols_dpu = prev_cols_dpu;
        dpu_info_A[i].rows_per_dpu = rows_per_dpu;
        dpu_info_A[i].prev_rows_dpu = prev_rows_dpu;
        dpu_info_A[i].prev_nnz_dpu = prev_nnz_dpu;
        dpu_info_A[i].nnz = nnz;
        dpu_info_A[i].nnz_pad = nnz_pad;


        // Find input arguments per DPU
        input_args_A[i].ncols = cols_per_dpu;
        input_args_A[i].trows = A->nrows; 
        input_args_A[i].tstart_col = prev_cols_dpu;

        input_args_A[i].nrows = rows_per_dpu;
        input_args_A[i].tcols = A->ncols; 
        input_args_A[i].tstart_row = prev_rows_dpu;
    }


    //for feature matrix
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = i%nr_of_partitions;
        uint32_t rows_per_dpu = partition_info->feat_row_split[k+1] - partition_info->feat_row_split[k];
        uint32_t prev_rows_dpu = partition_info->feat_row_split[k];

        if(rows_per_dpu > max_rows_per_dpu_feat){
            max_rows_per_dpu_feat = rows_per_dpu;
        }

        // Keep information per DPU
        dpu_info_feat[i].rows_per_dpu = rows_per_dpu;
        dpu_info_feat[i].prev_rows_dpu = prev_rows_dpu;

        // Find input arguments per DPU
        input_args_feat[i].nrows = rows_per_dpu;
        input_args_feat[i].tcols = feature->ncols; 
        input_args_feat[i].trows = feature->nrows; 
        input_args_feat[i].tstart_row = dpu_info_feat[i].prev_rows_dpu;
    }


    // Initializations for parallel transfers with padding needed
    if (max_cols_per_dpu_A % 2 == 1) 
        max_cols_per_dpu_A++;
    if (max_rows_per_dpu_A % 2 == 1) 
        max_rows_per_dpu_A++;
    if (max_nnz_per_dpu % (16 / byte_dt) != 0)
        max_nnz_per_dpu += ((16 / byte_dt) - (max_nnz_per_dpu % (16 / byte_dt)));


    // Re-allocation & restructure A
    A->nnzs = (struct elem_t *) realloc(A->nnzs, (max_nnz_per_dpu) * nr_of_dpus * sizeof(struct elem_t));


    /* Make dedicated cache insize WRAM
     * let's aim for using ~60% of the WRAM for caching the feature matrix
     * WRAM is 64 KB so 60% is about 40 KB
     * total feature size is feature->nrows * feature->ncols * sizeof(T)
     * so num of partitions are (feature->nrows * feature->ncols * sizeof(T) / 40KB) + 1
     */

    
    //18000 is a number found experimentally
    if (max_rows_per_dpu_feat % (8 / byte_dt) != 0) 
        max_rows_per_dpu_feat += ((8 / byte_dt) - (max_rows_per_dpu_feat % (8 / byte_dt)));
     
    uint32_t num_of_partitions = ( max_rows_per_dpu_feat / (18000/(feature->ncols * sizeof(T))-1) ) + 1;


    partition_info->num_of_partitions = num_of_partitions;

    //reconstruct input COO graph matrix
    reconstruct_COO_matrix(A, dpu_info_A, partition_info, input_args_A, max_rows_per_dpu_A, num_of_partitions, max_cols_per_dpu_A, nr_of_partitions, nr_of_dpus, max_nnz_per_dpu);

    // restructure feature matrix
    reconstruct_matrix(feature, dpu_info_feat, max_rows_per_dpu_feat, nr_of_partitions);
    


    //for input matrix w
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = (i%nr_of_partitions);
        uint32_t cols_per_dpu = partition_info->w_col_split[k+1] - partition_info->w_col_split[k];
        uint32_t prev_cols_dpu = partition_info->w_col_split[k];

        if(cols_per_dpu > max_cols_per_dpu_w){
            max_cols_per_dpu_w = cols_per_dpu;
        }

        // Keep information per DPU
        dpu_info_w[i].cols_per_dpu = cols_per_dpu;
        dpu_info_w[i].prev_cols_dpu = prev_cols_dpu;

        // Find input arguments per DPU
        input_args_w[i].ncols = cols_per_dpu;
        input_args_w[i].trows = weight->nrows; 
        input_args_w[i].tstart_col = dpu_info_w[i].prev_cols_dpu;
    }


    // Initializations for parallel transfers with padding needed => for weight and mid
    if (max_cols_per_dpu_w % (8 / byte_dt) != 0) 
        max_cols_per_dpu_w += ((8 / byte_dt) - (max_cols_per_dpu_w % (8 / byte_dt)));
    
    // reconstruct weight to match transmission restrictions
    reconstruct_weight(weight, dpu_info_w, max_cols_per_dpu_w, nr_of_partitions);


    //for mid result matrix => we are going to transpose(?) this, so adjust rows_per_dpu and cols per_dpu & other elements
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = (i/nr_of_partitions);
        uint32_t rows_per_dpu = partition_info->mid_row_split[k+1] - partition_info->mid_row_split[k];
        uint32_t prev_rows_dpu = partition_info->mid_row_split[k];

        if(rows_per_dpu > max_rows_per_dpu_mid){
            max_rows_per_dpu_mid = rows_per_dpu;
        }

        // Keep information per DPU
        dpu_info_mid[i].rows_per_dpu = rows_per_dpu;
        dpu_info_mid[i].prev_rows_dpu = prev_rows_dpu;

        // Find input arguments per DPU
        input_args_mid[i].nrows = rows_per_dpu;
        input_args_mid[i].tcols = mid->ncols; 
        input_args_mid[i].tstart_row = dpu_info_mid[i].prev_rows_dpu;
    }

    partition_by_row(partition_info, max_rows_per_dpu_mid, NR_TASKLETS);

    //save row_split info for use in kernel
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        uint32_t t;
        for (t = 0; t < NR_TASKLETS; t++) {
            input_args_mid[i].start_row[t] = partition_info->row_split_tasklet[t]; 
            input_args_mid[i].rows_per_tasklet[t] = partition_info->row_split_tasklet[t+1] - partition_info->row_split_tasklet[t];
            if (input_args_mid[i].rows_per_tasklet[t] > max_rows_per_tasklet_mid)
                max_rows_per_tasklet_mid = input_args_mid[i].rows_per_tasklet[t];
        }
    }
    
    //for padding
    if (max_rows_per_dpu_mid % (8 / byte_dt) != 0) 
        max_rows_per_dpu_mid += ((8 / byte_dt) - (max_rows_per_dpu_mid % (8 / byte_dt)));

    //Allocate mid matrix & sub result matrices for each DPU
    //the sub_results will be merged on the CPU
    new_mid_cycle = (T**)malloc(nr_of_partitions * sizeof(T*));
    for(i=0;i<nr_of_partitions;i++){
        new_mid_cycle[i] = (T*) calloc(feature->ncols * max_rows_per_dpu_mid, sizeof(T));
    }

    partial_mid = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        partial_mid[i] = (T*) calloc(feature->ncols * max_rows_per_dpu_A, sizeof(T));
    }

    //calculate total bytes sent to DPU in order to check if bytes exceeded MRAM size
    if(DPU_CAPACITY <= 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T) + max_rows_per_dpu_feat * feature->ncols * sizeof(T)){
        printf("size : %d\n", 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T) + max_rows_per_dpu_feat * feature->ncols * sizeof(T));
        printf("data size exceeded MRAM size\n");
        goto EXIT;
    }

    //send info on A and feature to DPU
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_A[i].cycle = 1;
        input_args_A[i].max_rows_per_dpu = max_rows_per_dpu_A;
        input_args_A[i].max_cols_per_dpu = max_cols_per_dpu_w; // just to pass on max_cols_per_dpu_w
        input_args_A[i].max_nnz_per_dpu = max_nnz_per_dpu;
        input_args_A[i].num_of_partitions = num_of_partitions;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_feat[i].max_rows_per_dpu = max_rows_per_dpu_feat;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_feat+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_feat", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    startTimer(&timer, 1);
    
    // Copy adjacency matrix to DPUs
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
        //set address to start data sending
        DPU_ASSERT(dpu_prepare_xfer(dpu, A->nnzs + max_nnz_per_dpu * i));
    }
    //offset : total number of rows * T + size of feature_matrix values
    //total size : max num of non-zero elements * sizeof(elem_t)
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, max_nnz_per_dpu * sizeof(struct elem_t), DPU_XFER_DEFAULT));

    //we send in another pair because the transposed data will be used on other cycles.
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, A->nnzs + max_nnz_per_dpu * transpose_num(i, nr_of_partitions)));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, max_nnz_per_dpu * sizeof(struct elem_t), max_nnz_per_dpu * sizeof(struct elem_t), DPU_XFER_DEFAULT));

    // Copy input feature to DPUs
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
        //same data may be sent multiple times => dealt with on the dpu side 
        DPU_ASSERT(dpu_prepare_xfer(dpu, feature->val + max_rows_per_dpu_feat * feature->ncols  * (i%nr_of_partitions)));
    } 
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T), max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));
    stopTimer(&timer, 1);

    // Run kernel on DPUs
    startTimer(&timer, 2);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 2);



/*****************************Conventional AllReduce*****************************/

    if(PIDComm_lib == 1){
        //retrieve results
        startTimer(&timer, 3);
        
        i=0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

        allreduce_x(new_mid_cycle, partial_mid, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_mid, mid->ncols);

        stopTimer(&timer, 3);
    }

/*****************************************************************************/

/****************************************PIDComm allreduce*************************************************/

    if(PIDComm_lib == 0){
        start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T);
        target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T);
        buffer_offset = 32*1024*1024;
        total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);

        startTimer(&timer, 3);
        pidcomm_all_reduce(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T), 0);
        stopTimer(&timer, 3);
    }

/**************************************************************************************************************/
    

    //compare y_host and mid for correctness
    bool status = true;
    unsigned int n, t;

    total_time += (timer.time[1] + timer.time[2] + timer.time[3]) / (1000);


//for the second part of the GNN


    
    if(mid->ncols > 512){ 
        printf("feature row length : %d\n", mid->ncols);
        printf("feature row length exceeded limit\n");
        goto EXIT;
    }

    //load other binary file to dpu
    DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));

    //compute final result for correctness
    struct Matrix *y_final;
    y_final = malloc(2*sizeof(uint32_t) + sizeof(T*));
    y_final->val = (T *) calloc((A->nrows * feature->ncols), sizeof(T));
    y_final->nrows = A->nrows;
    y_final->ncols = feature->ncols;
    GNN_host_rest(y_final, y_host, weight_2); 


    //Allocate new_feat and sub_result matrices for each DPU
    new_feat_cycle = (T**)malloc((nr_of_partitions) * sizeof(T*));
    for(i=0;i<nr_of_partitions;i++){
        new_feat_cycle[i] = (T*) calloc(max_rows_per_dpu_feat * feature->ncols, sizeof(T));
    }

    partial_feat = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        partial_feat[i] = (T*) calloc(feature->ncols * max_rows_per_dpu_mid, sizeof(T));
    }



    //send arguments to dpu
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_mid[i].max_rows_per_dpu = max_rows_per_dpu_mid;
        input_args_mid[i].max_nnz_per_dpu = max_nnz_per_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_mid+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_mid", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_w[i].max_cols_per_dpu = max_cols_per_dpu_w;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_w+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_weight", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    //send input matrices to DPUs
    startTimer(&timer, 6);

    // Copy input weight to DPUs
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_cols_per_dpu_w * weight->nrows  * (i%nr_of_partitions)));
    } 

    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_cols_per_dpu_w * weight->nrows * sizeof(T), DPU_XFER_DEFAULT));
 
    if(PIDComm_lib == 1){
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle[i/nr_of_partitions]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * mid->ncols * sizeof(T), DPU_XFER_DEFAULT));
    }

    stopTimer(&timer, 6);

    // Run kernel on DPUs
    startTimer(&timer, 8);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 8);


/****************************************conventional AllGather************************************************/

    if(PIDComm_lib == 0){
        // Copy mid result from DPUs
        startTimer(&timer, 9);
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, (new_feat_cycle[i /nr_of_partitions] + (max_cols_per_dpu_w * max_rows_per_dpu_mid * (i%nr_of_partitions)))));
        } 
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
        
        // Copy gathered data to DPUs
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, new_feat_cycle[i /nr_of_partitions]));
        } 
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 16*1024*1024, max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));

        stopTimer(&timer, 9);
    }

    /*************************************PIDComm AllGather********************************************************/

    if(PIDComm_lib == 1){
        start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T);
        target_offset = 16*1024*1024;
        buffer_offset = 30*1024*1024;
        total_data_size = max_rows_per_dpu_feat * feature->ncols * sizeof(T);

        startTimer(&timer, 9);
        pidcomm_allgather(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset);
        stopTimer(&timer, 9);
    }

    /********************************************************************************************************/

    /************************************Relocate data for faster communication*******************************************************/

    DPU_ASSERT(dpu_load(dpu_set, DATA_RELOCATE_AG, NULL));

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = 16*1024*1024;
        dpu_argument[i].target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T);
        dpu_argument[i].total_data_size = max_rows_per_dpu_feat * feature->ncols * sizeof(T);
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 1;
        dpu_argument[i].num_row = max_rows_per_dpu_feat;
    }

    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    startTimer(&timer, 3);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 3);

    /********************************************************************************************************/

    total_time += (timer.time[6] + timer.time[8] + timer.time[9] + timer.time[3]) / (1000);

    

    //for the other cycles
    for(int cycle = 2; cycle <= cycle_num; cycle++){

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_1, NULL));

        //send A & feature info to DPU
        startTimer(&timer, 1);
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            input_args_A[transpose_num(i, nr_of_partitions)].cycle = cycle;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A+transpose_num(i, nr_of_partitions)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_feat+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_feat", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        stopTimer(&timer, 1);

        // Run kernel on DPUs
        startTimer(&timer, 2);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 2);


/*************************************conventional AllReduce********************************************/

        //retrieve results
        if(PIDComm_lib == 1){
            startTimer(&timer, 3);
            i=0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

            //merge new mid_cycle
            allreduce_y(new_mid_cycle, partial_mid, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_feat, feature->ncols);
            stopTimer(&timer, 3);
        }

/******************************************************************************************/

/****************************************PIDComm AllReduce************************************************/

        if(PIDComm_lib == 0){
            start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T);
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T);
            buffer_offset = 32*1024*1024;
            total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);

            startTimer(&timer, 3);
            pidcomm_all_reduce(hypercube_manager, "010", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T), 0);
            stopTimer(&timer, 3);
        }

/**************************************************************************************************************/

        //compute cpu version result
        GNN_host_mid_2(B, y_final, y_host);

         //compare y_host and mid for correctness
        total_time += (timer.time[1] + timer.time[2] + timer.time[3]) / (1000);


        //the second part
        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));

        //send arguments to DPU
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_mid+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_mid", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_w+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_weight", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        //send input matrices to DPUs
        startTimer(&timer, 6);

        // Copy input weight to DPUs
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            //same data may be sent multiple times => dealt with on the dpu side
            DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_cols_per_dpu_w * weight->nrows  * (i/nr_of_partitions)));
        } 

        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_cols_per_dpu_w * weight->nrows * sizeof(T), DPU_XFER_DEFAULT));

        //load mid-result is conventional communication methods are used
        if(PIDComm_lib == 1){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle[i%nr_of_partitions]));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * mid->ncols * sizeof(T), DPU_XFER_DEFAULT));
        }

        stopTimer(&timer, 6);

        // Run kernel on DPUs
        startTimer(&timer, 8);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 8);


    /****************************************conventional AllGather************************************************/

        if(PIDComm_lib == 0){
            // Copy input feature to DPUs
            startTimer(&timer, 9);
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, (new_feat_cycle[i %nr_of_partitions] + (max_cols_per_dpu_w * max_rows_per_dpu_mid * (i/nr_of_partitions)))));
            } 
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));

            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_feat_cycle[i %nr_of_partitions]));
            } 
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 16*1024*1024, max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));
            stopTimer(&timer, 9);
        }

    /*************************************PIDComm AllGather********************************************************/

        if(PIDComm_lib == 1){
            start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T);
            target_offset = 16*1024*1024;
            buffer_offset = 31*1024*1024;
            total_data_size = max_rows_per_dpu_feat * feature->ncols * sizeof(T);

            startTimer(&timer, 9);
            pidcomm_allgather(hypercube_manager, "010", total_data_size, start_offset, target_offset, buffer_offset);
            stopTimer(&timer, 9);
        }

    /********************************************************************************************************/

    /*************************************Relocate data for faster communication*******************************************************/

        DPU_ASSERT(dpu_load(dpu_set, DATA_RELOCATE_AG, NULL));

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = 16*1024*1024;
            dpu_argument[i].target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T);
            dpu_argument[i].total_data_size = max_rows_per_dpu_feat * feature->ncols * sizeof(T);
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].num_row = max_rows_per_dpu_feat;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        startTimer(&timer, 3);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 3);

    /********************************************************************************************************/


        //calculate on host
        GNN_host_rest(y_final, y_host, weight_2); 

        total_time += (timer.time[6] + timer.time[8] + timer.time[9] + timer.time[3]) / (1000);

        /*********************************NEW CYCLE!!************************************************/
        cycle++;
        if(cycle > cycle_num) goto EXIT;

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_1, NULL));


        startTimer(&timer, 1);
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            input_args_A[transpose_num(i, nr_of_partitions)].cycle = cycle;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_feat+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_feat", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
        stopTimer(&timer, 1);

        // Run kernel on DPUs
        startTimer(&timer, 2);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 2);


/*************************************all reduce x host codes********************************************/

        if(PIDComm_lib == 1){
            //retrieve results
            startTimer(&timer, 3);
            i=0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

            //merge new mid_cycle
            allreduce_x(new_mid_cycle, partial_mid, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_feat, feature->ncols);
            stopTimer(&timer, 3);
        }

/******************************************************************************************/

/****************************************ALL-REDUCE x library CODES************************************************/

        if(PIDComm_lib == 0){
            start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T);
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T);
            buffer_offset = 32*1024*1024;
            total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);

            startTimer(&timer, 3);
            pidcomm_all_reduce(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T), 0);
            stopTimer(&timer, 3);
        }

/**************************************************************************************************************/

        //compute cpu version result
        GNN_host_mid_2(B, y_final, y_host);

        total_time += (timer.time[1] + timer.time[2] + timer.time[3]) / (1000);


        /*****************************the second part**************************************/

        //the second part
        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));

        //send arguments to DPU
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_mid+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_mid", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_w+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_weight", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        //send input matrices to DPUs
        startTimer(&timer, 6);

        // Copy input weight to DPUs
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            //same data may be sent multiple times => dealt with on the dpu side
            DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_cols_per_dpu_w * weight->nrows  * (i%nr_of_partitions)));
        } 

        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_cols_per_dpu_w * weight->nrows * sizeof(T), DPU_XFER_DEFAULT));

        if(PIDComm_lib == 1){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                //set address to start data sending
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle[i/nr_of_partitions]));
            }
            //offset : total number of rows * T + size of feature_matrix values
            //total size : max num of non-zero elements * sizeof(elem_t)
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T) + max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * mid->ncols * sizeof(T), DPU_XFER_DEFAULT));
        }
        
        stopTimer(&timer, 6);

        // Run kernel on DPUs
        startTimer(&timer, 8);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 8);

        /**************************************** Gather finished results ************************************************/
        
        //retrieve results
        startTimer(&timer, 9);
        i=0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_feat + i)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_cols_per_dpu_w * weight->nrows * sizeof(T), max_cols_per_dpu_w * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
        
        gather_x(new_feat_cycle, partial_feat, nr_of_partitions, dpu_info_w, max_rows_per_dpu_A, max_rows_per_dpu_mid, max_cols_per_dpu_w, mid->ncols);
        stopTimer(&timer, 9);

        /**************************************************************************************************************/

        //calculate on host
        GNN_host_rest(y_final, y_host, weight_2); 

        //compare y_host and mid for correctness
        status = true;
        i = 0;
        j=0;
        t=0;
        for(i=0;i<nr_of_partitions;i++){
            for(unsigned int row = 0; row < dpu_info_feat[i].rows_per_dpu; row++){
                for(unsigned int col = 0; col < feature->ncols; col++){
                    if(y_final->val[(row + dpu_info_feat[i].prev_rows_dpu) * feature->ncols + col] != new_feat_cycle[i][row * feature->ncols + col]){
                        status = false; j++; t++;
                        
                    }
                }
            }
        } 
        if (status) {
            printf("[ OK ] Outputs are equal\n");
        } else {
            printf("[ ERROR ] Outputs differ!. %d\n", j);
        }

        total_time += (timer.time[6] + timer.time[8] + timer.time[9]) / (1000);
    }


EXIT : 

    //add deallocation later

    DPU_ASSERT(dpu_free(dpu_set));

    printf("\ntotal exec. time = %f\n", total_time);

    return 0;


}