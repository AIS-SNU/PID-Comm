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

#ifndef DATA_RELOCATE_COMM
#define DATA_RELOCATE_COMM "./bin/data_relocate_comm"
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

//must receive num of rank as input
int main(int argc, char **argv) {

    //for time measurments
    Timer timer;

    float total_time = 0;

    uint32_t nr_dpus = atoi(argv[1]);
    uint32_t nr_partition = sqrt(nr_dpus);
    uint32_t PIDComm_lib = atoi(argv[4]);

    char *abs_dir = (char *) malloc(1024);
    abs_dir = getcwd(abs_dir, 1024);
    char* fileName =  strcat(strcat(strcat(abs_dir, (char *)"/inputs/"), (char *)argv[2]), ".mtx");
    printf("%s, done!\n", fileName);

    struct dpu_set_t dpu_set, dpu;


    uint32_t nr_of_dpus = nr_dpus;
    //number of partitions => square root of number of dpus
    uint32_t nr_of_partitions = nr_partition;



    /**************************SETTINGS FOR RS_AR***********************************/

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

    //create matrices
    A = readCOOMatrix(fileName, nr_of_dpus, nr_of_partitions, partition_info);
    B = copy_COOMatrix(A, nr_of_dpus); // make function to copy A for later use
    feature = create_matrix(A->ncols, atoi(argv[3]), 1);


    //create matrix W
    weight = create_matrix(feature->ncols, feature->ncols, 1);

    weight_2 = create_matrix(feature->ncols, feature->ncols, 1); //for use in later cycles

    uint32_t padding = ((8/byte_dt) - feature->ncols % (8/byte_dt));

    if(padding % (8/byte_dt) != 0){
        zeropad_feat(feature, padding, 1);
        zeropad_weight(weight, padding, 1);
        zeropad_weight(weight_2, padding, 1);
    }


    mid = malloc(2*sizeof(uint32_t) + sizeof(T*));
    mid->nrows = A->nrows;
    mid->ncols = feature->ncols;


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
    uint64_t max_cols_per_dpu_mid = 0;
    uint64_t max_rows_per_dpu_w = 0;


    uint64_t max_rows_per_tasklet_mid = 0; 

    uint64_t max_nnz_per_dpu = 0;

    unsigned int i=0;
    unsigned int j=0;
    unsigned int k=0;

    //initialize partition_data

    //partition the matrices
    feat_partition_by_row(feature, partition_info, nr_of_partitions);
    w_partition_by_row(weight, partition_info, nr_of_partitions);
    mid_partition(mid, partition_info, nr_of_partitions);


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

    //same for feature and weight matrices
    if (max_rows_per_dpu_feat % (8 / byte_dt) != 0) 
        max_rows_per_dpu_feat += ((8 / byte_dt) - (max_rows_per_dpu_feat % (8 / byte_dt)));


    // Re-allocation & restructure A
    A->nnzs = (struct elem_t *) realloc(A->nnzs, (max_nnz_per_dpu) * nr_of_dpus * sizeof(struct elem_t));


    /* Make dedicated cache insize WRAM
     * let's aim for using ~60% of the WRAM for caching the feature matrix
     * WRAM is 64 KB so 60% is about 40 KB
     * total feature size is feature->nrows * feature->ncols * sizeof(T)
     * so num of partitions are (feature->nrows * feature->ncols * sizeof(T) / 40KB) + 1
     */

    
    //18000 is a number found experimentally
    uint32_t num_of_partitions = ( max_rows_per_dpu_feat / (18000/(feature->ncols * sizeof(T))-1) ) + 1;


    partition_info->num_of_partitions = num_of_partitions;

    //reconstruct the feature matrix for
    reconstruct_COO_matrix(A, dpu_info_A, partition_info, input_args_A, max_rows_per_dpu_A, num_of_partitions, max_cols_per_dpu_A, nr_of_partitions, nr_of_dpus, max_nnz_per_dpu);

    //restructure feature matrix
    reconstruct_matrix(feature, dpu_info_feat, max_rows_per_dpu_feat, nr_of_partitions);
    


    //for input matrix w
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = (i/nr_of_partitions);
        uint32_t rows_per_dpu = partition_info->w_row_split[k+1] - partition_info->w_row_split[k];
        uint32_t prev_rows_dpu = partition_info->w_row_split[k];

        if(rows_per_dpu > max_rows_per_dpu_w){
            max_rows_per_dpu_w = rows_per_dpu;
        }

        // Keep information per DPU
        dpu_info_w[i].rows_per_dpu = rows_per_dpu;
        dpu_info_w[i].prev_rows_dpu = prev_rows_dpu;

        // Find input arguments per DPU
        input_args_w[i].nrows = rows_per_dpu;
        input_args_w[i].tcols = weight->ncols; 
        input_args_w[i].tstart_row = dpu_info_w[i].prev_rows_dpu;
    }


    // Initializations for parallel transfers with padding needed => for weight and mid
    if (max_rows_per_dpu_w % (8 / byte_dt) != 0) 
        max_rows_per_dpu_w += ((8 / byte_dt) - (max_rows_per_dpu_w % (8 / byte_dt)));
    
    // reconstruct weight to match transmission restrictions
    reconstruct_weight(weight, dpu_info_w, max_rows_per_dpu_w, nr_of_partitions);


    //for mid result matrix => we are going to transpose(?) this, so adjust rows_per_dpu and cols per_dpu & other elements
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        k = (i%nr_of_partitions);
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

        k = (i/nr_of_partitions);
        uint32_t cols_per_dpu = partition_info->mid_col_split[k+1] - partition_info->mid_col_split[k];
        uint32_t prev_cols_dpu = partition_info->mid_col_split[k];

        if(cols_per_dpu > max_cols_per_dpu_mid){
            max_cols_per_dpu_mid = cols_per_dpu;
        }

        // Keep information per DPU
        dpu_info_mid[i].cols_per_dpu = cols_per_dpu;
        dpu_info_mid[i].prev_cols_dpu = prev_cols_dpu;

        // Find input arguments per DPU
        input_args_mid[i].ncols = cols_per_dpu;
        input_args_mid[i].trows = mid->nrows; 
        input_args_mid[i].tstart_col = dpu_info_mid[i].prev_cols_dpu;
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
    if (max_cols_per_dpu_mid % (8 / byte_dt) != 0) 
        max_cols_per_dpu_mid += ((8 / byte_dt) - (max_cols_per_dpu_mid % (8 / byte_dt)));


    //Allocate mid matrix & sub result matrices for each DPU
    //the sub_results will be merged on the CPU
    new_mid_cycle = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        new_mid_cycle[i] = (T*) calloc(max_rows_per_dpu_A * max_cols_per_dpu_mid, sizeof(T));
    }

    partial_mid = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        partial_mid[i] = (T*) calloc(feature->ncols * max_rows_per_dpu_A, sizeof(T));
    }

    //calculate total bytes sent to DPU in order to check if bytes exceeded MRAM size
    if(DPU_CAPACITY <= 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T) + max_rows_per_dpu_feat * feature->ncols * sizeof(T)){
        printf("size : %d\n", 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T) + max_rows_per_dpu_feat * feature->ncols * sizeof(T));
        printf("data size exceeded MRAM size\n");
        goto EXIT;
    }


    //send info on A and feature to DPU
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_A[i].cycle = 1;
        input_args_A[i].max_rows_per_dpu = max_rows_per_dpu_A;
        input_args_A[i].max_cols_per_dpu = max_rows_per_dpu_w;
        input_args_A[i].max_nnz_per_dpu = max_nnz_per_dpu;
        input_args_A[i].num_of_partitions = num_of_partitions;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_feat[i].max_rows_per_dpu = max_rows_per_dpu_feat;
        input_args_feat[i].max_cols_per_dpu = max_cols_per_dpu_mid;
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
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T), max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));
    stopTimer(&timer, 1);

    // Run kernel on DPUs
    startTimer(&timer, 2);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 2);


/*****************************Relocate data for faster communication************************************/

    DPU_ASSERT(dpu_load(dpu_set, DATA_RELOCATE_COMM, NULL));

    target_offset = 63*512*1024;

    for(int i=0; i<nr_dpus; i++){
        dpu_argument[i].start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
        dpu_argument[i].target_offset = target_offset;
        dpu_argument[i].total_data_size = feature->ncols * sizeof(T);
        dpu_argument[i].num_comm_dpu = num_comm_dpu;
        dpu_argument[i].no_rotate = 1;
        dpu_argument[i].num_row = max_rows_per_dpu_A;
    }

    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        dpu_argument[i].each_dpu = i;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

    startTimer(&timer, 3);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 3);


/*****************************************************************************/

    // for reading in mid-results from DPU
    T** new_mid_cycle1 = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        new_mid_cycle1[i] = (T*) calloc(max_rows_per_dpu_A * max_cols_per_dpu_mid, sizeof(T));
    }

/*****************************Conventional reducescatter*****************************/

    if(PIDComm_lib == 0){
        startTimer(&timer, 4);
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, target_offset, feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

        
        reducescatter_x(new_mid_cycle1, partial_mid, num_comm_dpu, feature->ncols * max_rows_per_dpu_A * sizeof(T), nr_of_partitions, nr_of_partitions, 1);
        stopTimer(&timer, 4);
    }

/*****************************************************************************/
    
/******************************** PIDComm reducescatter **********************************************/

    if(PIDComm_lib == 1){
        start_offset = 63*512*1024;
        target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T);
        buffer_offset = 16*1024*1024;
        total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);
        


        startTimer(&timer, 4);
        pidcomm_reduce_scatter(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T));
        stopTimer(&timer, 4);
    }

/*******************************************************************************************************************************/



    
    // //compare y_host and mid for correctness
    bool status = true;
    unsigned int n, t;

    total_time += (timer.time[1] + timer.time[2] + timer.time[3] + timer.time[4]) / (1000);

//for the second part of the GNN

    //if feature row length exceeds limit, exit
    if(max_cols_per_dpu_mid > 512){ 
        printf("feature row length exceeded limit\n");
        goto EXIT;
    }



    //load other binary file to dpu
    DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));


    //Allocate new_feat and sub_result matrices for each DPU
    new_feat_cycle = (T**)malloc((nr_of_partitions) * sizeof(T*));
    for(i=0;i<nr_of_partitions;i++){
        new_feat_cycle[i] = (T*) calloc(max_rows_per_dpu_feat * feature->ncols, sizeof(T));
    }

    partial_feat = (T**)malloc(nr_of_dpus * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        partial_feat[i] = (T*) calloc(weight->ncols * max_rows_per_dpu_mid, sizeof(T));
    }

    if(DPU_CAPACITY <= 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T) + max_rows_per_dpu_mid * max_cols_per_dpu_mid * sizeof(T)){
        printf("data size exceeded MRAM size\n");
        goto EXIT;
    }


    //send arguments to dpu
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_mid[i].max_rows_per_dpu = max_rows_per_dpu_mid;
        input_args_mid[i].max_cols_per_dpu = max_cols_per_dpu_mid;
        input_args_mid[i].max_nnz_per_dpu = max_nnz_per_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_mid+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_mid", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
        input_args_w[i].max_rows_per_dpu = max_rows_per_dpu_w;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_w+i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_weight", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    //send input matrices to DPUs
    startTimer(&timer, 6);

    // Copy input weight to DPUs
    i = 0;
    DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
        //same data may be sent multiple times => dealt with on the dpu side
        DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_rows_per_dpu_w * weight->ncols  * (transpose_num(i, nr_of_partitions)/nr_of_partitions)));
    } 

    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_rows_per_dpu_w * weight->ncols * sizeof(T), DPU_XFER_DEFAULT));

    //load matrix if conventional communication methods are used
    if(PIDComm_lib == 0){
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle1[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * max_cols_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
    }
  
    stopTimer(&timer, 6);
 

    // Run kernel on DPUs
    startTimer(&timer, 8);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 8);


    
/****************************************comventional allreduce************************************************/
    //retrieve results
    if(PIDComm_lib == 0){
        startTimer(&timer, 9);    
        i=0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_feat + i)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T), weight->ncols * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));

        allreduce_x(new_feat_cycle, partial_feat, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_feat, feature->ncols);
        stopTimer(&timer, 9);
    }
/**************************************************************************************************************/

    T** new_feat_cycle1 = (T**)malloc((nr_of_dpus) * sizeof(T*));
    for(i=0;i<nr_of_dpus;i++){
        new_feat_cycle1[i] = (T*) calloc(max_rows_per_dpu_feat * feature->ncols, sizeof(T));
    }

/****************************************PIDComm allreduce************************************************/

    if(PIDComm_lib == 1){
        start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
        target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T);
        buffer_offset = 32*1024*1024;
        total_data_size = weight->ncols * max_rows_per_dpu_mid * sizeof(T);

        startTimer(&timer, 9);
        pidcomm_all_reduce(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T), 0);
        stopTimer(&timer, 9);
    }

/**************************************************************************************************************/

    total_time += (timer.time[6] + timer.time[8] + timer.time[9]) / (1000);

    //continue for the remaining cycles
    for(int cycle = 2; cycle <= cycle_num; cycle++){

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_1, NULL));

        //send A & feature info to DPU
        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            input_args_A[transpose_num(i, nr_of_partitions)].cycle = cycle;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A + transpose_num(i, nr_of_partitions)));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_feat+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_feat", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));


         // Copy input feature to DPUs
        startTimer(&timer, 1);
        if(PIDComm_lib == 0){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                //same data may be sent multiple times => dealt with on the dpu side 
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_feat_cycle[i /nr_of_partitions]));
            } 
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T), max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));
        }
        stopTimer(&timer, 1);

        // Run kernel on DPUs
        startTimer(&timer, 2);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 2);


/*****************************Relocate data for faster communication************************************/

        DPU_ASSERT(dpu_load(dpu_set, DATA_RELOCATE_COMM, NULL));

        target_offset = 63*512*1024;

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = feature->ncols * sizeof(T);
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].num_row = max_rows_per_dpu_A;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        startTimer(&timer, 3);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 3);


/*****************************************************************************/

/*****************************conventional reducescatter*****************************/

        if(PIDComm_lib == 0){
            startTimer(&timer, 5);
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, target_offset, feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

            reducescatter_y(new_mid_cycle1, partial_mid, num_comm_dpu, feature->ncols * max_rows_per_dpu_A * sizeof(T), nr_of_partitions, nr_of_partitions, 1);
            stopTimer(&timer, 5);
        }

/*****************************************************************************/

/******************************PIDComm reducescatter*************************************/

        if(PIDComm_lib == 1){
            start_offset = 63*512*1024;
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T);
            buffer_offset = 16*1024*1024;
            total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);

            startTimer(&timer, 5);
            pidcomm_reduce_scatter(hypercube_manager, "010", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T));
            stopTimer(&timer, 5);
        }

/**********************************************************************************************************************************************************/

        total_time += (timer.time[1] + timer.time[2] + timer.time[3] + timer.time[5]) / (1000);

        //the second part

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));

        //send arguments to dpu
        i = 0;
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

        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            //same data may be sent multiple times => dealt with on the dpu side
            DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_rows_per_dpu_w * weight->ncols  * (i/nr_of_partitions)));
        } 

        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_rows_per_dpu_w * weight->ncols * sizeof(T), DPU_XFER_DEFAULT));

        //load mid-result is conventional communication methods are used
        if(PIDComm_lib == 0){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle1[i]));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2*max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * max_cols_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
        }

        stopTimer(&timer, 6);

        // Run kernel on DPUs
        startTimer(&timer, 8);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 8);

        

/****************************************conventional allreduce*******************************************************************/
        if(PIDComm_lib == 0){
            //retrieve results
            startTimer(&timer, 9);        
            i=0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_feat + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2*max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T), weight->ncols * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
            
            allreduce_y(new_feat_cycle, partial_feat, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_feat, feature->ncols);
            stopTimer(&timer, 9);
        }
/*********************************************************************************************************************************/

/****************************************PIDComm allreduce************************************************/

        if(PIDComm_lib == 1){
            new_feat_cycle1 = (T**)malloc((nr_of_dpus) * sizeof(T*));
            for(i=0;i<nr_of_dpus;i++){
                new_feat_cycle1[i] = (T*) calloc(max_rows_per_dpu_feat * feature->ncols, sizeof(T));
            }

            start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T);
            buffer_offset = 32*1024*1024;
            total_data_size = weight->ncols * max_rows_per_dpu_mid * sizeof(T);

            startTimer(&timer, 9);
            pidcomm_all_reduce(hypercube_manager, "010", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T), 0);
            stopTimer(&timer, 9);
        }

/**************************************************************************************************************/

        total_time += (timer.time[6] + timer.time[8] + timer.time[9]) / (1000);
        
        /****************************** next cycle ************************************************/

        cycle++;
        if(cycle > cycle_num) goto EXIT;

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_1, NULL));

        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            input_args_A[i].cycle = cycle;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_A+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_A", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args_feat+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_feat", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));


        // Copy input feature to DPUs
        startTimer(&timer, 1);

        if(PIDComm_lib == 0){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_feat_cycle[i%nr_of_partitions]));
            } 
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T), max_rows_per_dpu_feat * feature->ncols * sizeof(T), DPU_XFER_DEFAULT));
        }

        stopTimer(&timer, 1);

        // Run kernel on DPUs
        startTimer(&timer, 2);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 2);


/*****************************Relocate data for faster communication************************************/

        DPU_ASSERT(dpu_load(dpu_set, DATA_RELOCATE_COMM, NULL));

        target_offset = 63*512*1024;

        for(int i=0; i<nr_dpus; i++){
            dpu_argument[i].start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
            dpu_argument[i].target_offset = target_offset;
            dpu_argument[i].total_data_size = feature->ncols * sizeof(T);
            dpu_argument[i].num_comm_dpu = num_comm_dpu;
            dpu_argument[i].no_rotate = 1;
            dpu_argument[i].num_row = max_rows_per_dpu_A;
        }

        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
            dpu_argument[i].each_dpu = i;
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_argument+i));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS_RS1", 0, sizeof(dpu_arguments_comm_t), DPU_XFER_DEFAULT));

        startTimer(&timer, 3);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 3);


/*****************************************************************************/

/*****************************conventional reducescatter*****************************/

        if(PIDComm_lib == 0){
            startTimer(&timer, 5);
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_mid + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, target_offset, feature->ncols * max_rows_per_dpu_A * sizeof(T), DPU_XFER_DEFAULT));

            
            reducescatter_x(new_mid_cycle1, partial_mid, num_comm_dpu, feature->ncols * max_rows_per_dpu_A * sizeof(T), nr_of_partitions, nr_of_partitions, 1);
            stopTimer(&timer, 5);
        }

/*****************************************************************************/
    
/******************************** PIDComm reducescatter **********************************************/

        if(PIDComm_lib == 1){
            start_offset = 63*512*1024;
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T);
            buffer_offset = 16*1024*1024;
            total_data_size = feature->ncols * max_rows_per_dpu_A * sizeof(T);

            startTimer(&timer, 5);
            pidcomm_reduce_scatter(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T));
            stopTimer(&timer, 5);
        }

/*******************************************************************************************************************************/


        total_time += (timer.time[1] + timer.time[2] + timer.time[3] + timer.time[5]) / (1000);


        //the second part

        DPU_ASSERT(dpu_load(dpu_set, GNN_KERNEL_2, NULL));

        //send arguments to dpu
        i = 0;
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

        i = 0;
        DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
            //same data may be sent multiple times => dealt with on the dpu side
            DPU_ASSERT(dpu_prepare_xfer(dpu, weight->val + max_rows_per_dpu_w * weight->ncols  * (transpose_num(i, nr_of_partitions)/nr_of_partitions)));
        } 

        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2 * max_nnz_per_dpu * sizeof(struct elem_t), max_rows_per_dpu_w * weight->ncols * sizeof(T), DPU_XFER_DEFAULT));
 
        //send mid-results to DPU in case of conventional communication
        if(PIDComm_lib == 0){
            i = 0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, new_mid_cycle1[i]));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2*max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + weight->ncols * max_rows_per_dpu_mid * sizeof(T), max_rows_per_dpu_mid * max_cols_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
        }

        stopTimer(&timer, 6);

        // Run kernel on DPUs
        startTimer(&timer, 8);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        stopTimer(&timer, 8);

        /***************************************conventional allreduce******************************************************/

        //retrieve results
        if(PIDComm_lib == 0){
            startTimer(&timer, 9);        
            i=0;
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus){
                DPU_ASSERT(dpu_prepare_xfer(dpu, *(partial_feat + i)));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 2*max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T), weight->ncols * max_rows_per_dpu_mid * sizeof(T), DPU_XFER_DEFAULT));
            

            allreduce_x(new_feat_cycle1, partial_feat, nr_of_partitions, nr_of_dpus, max_rows_per_dpu_feat, feature->ncols);

            stopTimer(&timer, 9);
        }

        /**************************************************************************************************************/


/**************************************** retrieve final data ************************************************/

        if(PIDComm_lib == 1){
            
            start_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T);
            target_offset = 2 * max_nnz_per_dpu * sizeof(struct elem_t) + max_rows_per_dpu_w * weight->ncols * sizeof(T) + feature->ncols * max_rows_per_dpu_A * sizeof(T);
            buffer_offset = 32*1024*1024;
            total_data_size = weight->ncols * max_rows_per_dpu_mid * sizeof(T);

            startTimer(&timer, 9);

            pidcomm_reduce_scatter(hypercube_manager, "100", total_data_size, start_offset, target_offset, buffer_offset, sizeof(T));
            
            //for checking
            DPU_FOREACH_ENTANGLED_GROUP(dpu_set, dpu, i, nr_dpus) {
                //set address to start data sending
                DPU_ASSERT(dpu_prepare_xfer(dpu, (new_feat_cycle1[i/nr_of_partitions] + (i%nr_of_partitions)*total_data_size/(nr_of_partitions*sizeof(T)))));
            }
            DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, target_offset, total_data_size/nr_of_partitions, DPU_XFER_DEFAULT));

            stopTimer(&timer, 9);
        }
     
/**************************************************************************************************************/

        total_time += (timer.time[6] + timer.time[8] + timer.time[9]) / (1000);
    }

    printf("\ntotal exec. time = %f\n", total_time);

EXIT : 

    //add deallocation later

    DPU_ASSERT(dpu_free(dpu_set));

    return 0;


}