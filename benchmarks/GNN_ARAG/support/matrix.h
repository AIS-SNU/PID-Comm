#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdio.h>

#include "common.h"
#include "partition.h"

/** 
 * brief Comparator for Quicksort
 */
int comparator(void *a, void *b) {
    if (((struct elem_t *)a)->rowind < ((struct elem_t *)b)->rowind) {
        return -1;
    } else if (((struct elem_t *)a)->rowind > ((struct elem_t *)b)->rowind) {
        return 1;
    } else {
        return (((struct elem_t *)a)->colind - ((struct elem_t *)b)->colind);
    }
}

/**
 * @brief Sort Input Matrix
 * @param matrix in COO format
 */
static void sortCOOMatrix(struct COOMatrix *cooMtx) {

    qsort(cooMtx->nnzs, cooMtx->nnz, sizeof(struct elem_t), comparator);

}

// find which dpu an element belongs to
uint32_t dpu_num_for_element(struct partition_info_t *partition_info, uint32_t rowind, uint32_t colind, uint32_t nr_of_partitions){
    uint32_t rowind_dpu, colind_dpu;

    for(colind_dpu = 0; colind_dpu < nr_of_partitions; colind_dpu++){
        if(partition_info->COO_col_split[colind_dpu+1] > colind) break;
    }

    for(rowind_dpu = 0; rowind_dpu < nr_of_partitions; rowind_dpu++){
        if(partition_info->COO_row_split[rowind_dpu+1] > rowind) break;
    }

    return (rowind_dpu * nr_of_partitions + colind_dpu);
}

//copy COO matrix
struct COOMatrix *copy_COOMatrix(struct COOMatrix *mat, uint32_t nr_of_dpus){
    struct COOMatrix *new_mat = (struct COOMatrix *) malloc(sizeof(struct COOMatrix));

    new_mat->nrows = mat->nrows;
    new_mat->ncols = mat->ncols;
    new_mat->nnz = mat->nnz;
    new_mat->partitions = (uint32_t *) calloc((nr_of_dpus), sizeof(uint32_t));
    new_mat->nnzs = (struct elem_t *) calloc((new_mat->nnz+8), sizeof(struct elem_t));

    for(int i=0; i<nr_of_dpus; i++){
        new_mat->partitions[i] = mat->partitions[i];
    }

    for(int i=0; i<new_mat->nnz; i++){
        new_mat->nnzs[i].rowind = mat->nnzs[i].rowind;
        new_mat->nnzs[i].colind = mat->nnzs[i].colind;
        new_mat->nnzs[i].val = mat->nnzs[i].val; 
    }

    return new_mat;
}

//make random Matrix of given size
struct Matrix *create_matrix(uint32_t nrows, uint32_t ncols, bool row_major){
    struct Matrix *mat = malloc(2 * sizeof(uint32_t) + sizeof(T*));
    mat->val = malloc(nrows * ncols * sizeof(T));
    mat->nrows = nrows;
    mat->ncols = ncols;
    if(row_major){
        for(int row =0; row < nrows; row++){
            for(int col =0; col < ncols; col++){
                mat->val[row * ncols + col] = ((row * ncols + col) % 3 -1);
            }
        }
    }
    else {
        for(int col =0; col < ncols; col++){
            for(int row =0; row < nrows; row++){
                mat->val[col * nrows + row] = ((col * nrows + row) % 3 -1);
            }
        }
    }
    return mat;
}

// for adding zero-padding to feature & weight when aligning their row size to 8 bytes
// adds extra columns of zero for the extended columns
void zeropad_feat(struct Matrix *mat, uint32_t padding_amount, bool rowwise){
    uint32_t ncols = mat->ncols;
    uint32_t nrows = mat->nrows;
    T* new_val = (T*) calloc((ncols+padding_amount) * nrows, sizeof(T));
    if(rowwise){
        for(int row = 0; row < nrows; row++){
            for(int col = 0; col<ncols; col++){
                new_val[row*(ncols+padding_amount) + col] = mat->val[row*ncols + col];
            }
            for(int col = ncols; col<ncols + padding_amount; col++){
                new_val[row*(ncols+padding_amount) + col] = 0;
            }
        }
    }

    else{
        for(int col=0; col<ncols; col++){
            for(int row=0; row<nrows; row++){
                new_val[col*nrows + row] = mat->val[col*nrows + row];
            }
        }
        for(int col=ncols; col<ncols + padding_amount; col++){
            for(int row=0; row<nrows; row++){
                new_val[col*nrows + row] = 0;
            }
        }
    }

    free(mat->val);
    mat->val = new_val;
    mat->ncols = ncols+padding_amount;

    return;
}

// for adding zero-padding to feature & weight when aligning their row size to 8 bytes
// adds extra columns of zero for the extended columns
void zeropad_weight(struct Matrix *mat, uint32_t padding_amount, bool rowwise){
    uint32_t ncols = mat->ncols;
    uint32_t nrows = mat->nrows;
    T* new_val = (T*) calloc((ncols+padding_amount) * (nrows + padding_amount), sizeof(T));
    if(rowwise){
        for(int row = 0; row < nrows; row++){
            for(int col = 0; col<ncols; col++){
                new_val[row*(ncols+padding_amount) + col] = mat->val[row*ncols + col];
            }
            for(int col = ncols; col<ncols + padding_amount; col++){
                new_val[row*(ncols+padding_amount) + col] = 0;
            }
        }
        for(int row = nrows; row < nrows + padding_amount; row++){
            for(int col = 0; col<ncols + padding_amount; col++){
                new_val[row*(ncols+padding_amount) + col] = 0;
            }
        }
    }

    else{
        for(int col=0; col<ncols; col++){
            for(int row=0; row<nrows; row++){
                new_val[col*(nrows + padding_amount) + row] = mat->val[col*nrows + row];
            }
            for(int row=0; row<nrows + padding_amount; row++){
                new_val[col*(nrows + padding_amount) + row] = 0;
            }
        }
        for(int col=ncols; col<ncols + padding_amount; col++){
            for(int row=0; row<nrows; row++){
                new_val[col*(nrows + padding_amount) + row] = 0;
            }
        }
    }

    free(mat->val);
    mat->val = new_val;
    mat->ncols = ncols+padding_amount;
    mat->nrows = ncols+padding_amount;

    return;
}


/* restructure row-major matrix into partitioned row-major matrices
 * make address follow row major adressing inside each partition
 * for sending data to each dpu
 * resizes matrix according to the new max_cols_per_dpu
 **/
void reconstruct_matrix(struct Matrix *mat, struct dpu_info_t *dpu_info, uint32_t max_rows_per_dpu, uint32_t nr_of_partitions){
    uint32_t nrows = mat->nrows;
    uint32_t ncols = mat->ncols;
    if(ncols % (8/byte_dt) != 0) ncols = ncols + ((8 / byte_dt) - (ncols % (8 / byte_dt)));
    uint32_t rows_per_dpu, prev_rows_dpu;
    int index;

    T* temp = malloc(ncols * max_rows_per_dpu * nr_of_partitions * sizeof(T));
    for(int i=0;i<nr_of_partitions; i++){
        rows_per_dpu = dpu_info[i].rows_per_dpu;
        prev_rows_dpu = dpu_info[i].prev_rows_dpu;

        // if there are no zero-padding in the partition, pass
        //otherwise set the zero-padded column in between
        if(rows_per_dpu == max_rows_per_dpu){
            for(int k=0; k<max_rows_per_dpu; k++){
                for(int j=0; j<mat->ncols; j++){
                    //fill matrix in order
                    temp[(i * ncols * max_rows_per_dpu) + (k * ncols) + j] = mat->val[(ncols * prev_rows_dpu) + (k * ncols) + j];
                }
                for(int j=mat->ncols; j<ncols; j++){
                    temp[(i * ncols * max_rows_per_dpu) + (k * ncols) + j] = 0;
                }
            }
        }
        
        else{
            for(int k=0; k<rows_per_dpu; k++){
                for(int j=0; j<mat->ncols; j++){
                    //fill matrix in order
                    temp[(i * ncols * max_rows_per_dpu) + (k * ncols) + j] = mat->val[(ncols * prev_rows_dpu) + (k * ncols) + j];
                }
                for(int j=mat->ncols; j<ncols; j++){
                    //fill matrix in order
                    temp[(i * ncols * max_rows_per_dpu) + (k * ncols) + j] = 0;
                }
            }
            
            index = (i * ncols * max_rows_per_dpu + ncols * rows_per_dpu);
            for(int k = rows_per_dpu; k<max_rows_per_dpu;k++){
                for(int j=0; j<ncols; j++){
                    temp[index] = 0;
                    index++;
                }
            }
            
        }
    }
    free(mat->val);
    mat->val = temp;

    return;
}

//reconstruct COO matrix to follow row major ordering in each partition
// reconstruction is needed to enable matrix stripping / blocking for efficient computation
void reconstruct_COO_matrix(struct COOMatrix *mat, struct dpu_info_t *dpu_info, struct partition_info_t *partition_info, dpu_arguments_t *input_args, uint32_t max_rows_per_dpu,
                                                            uint32_t num_of_partitions, uint32_t max_cols_per_dpu, uint32_t nr_of_partitions, uint32_t nr_of_dpus, uint32_t max_nnz_per_dpu){
    uint32_t nrows = mat->nrows;
    uint32_t ncols = mat->ncols;
    int index, dpu_index, nnz_index;
    int i, j, rowind, colind;
    
    struct elem_t temp;

    struct elem_t* new_nnz[nr_of_dpus];
    int new_nnz_index[nr_of_dpus];


    for(i=0;i<nr_of_dpus;i++){
        new_nnz[i] = malloc(max_nnz_per_dpu * sizeof(struct elem_t));
    }

    //realloc mat->nnzs to fit into the new COO matrix
    struct elem_t* new_nnzs = (struct elem_t*) malloc((max_nnz_per_dpu * nr_of_dpus) * sizeof(struct elem_t));

    //initialize new_nnz_index
    for(i=0;i<nr_of_dpus;i++){
        new_nnz_index[i] = 0;
    }

    //traverse through the nnzs and find the appropriate place for each of them
    for(nnz_index = 0; nnz_index < mat->nnz; nnz_index++){

        rowind = mat->nnzs[nnz_index].rowind;
        colind = mat->nnzs[nnz_index].colind;

        dpu_index = dpu_num_for_element(partition_info, rowind, colind, nr_of_partitions);

        new_nnz[dpu_index][new_nnz_index[dpu_index]] = mat->nnzs[nnz_index];
        new_nnz_index[dpu_index]++;
        
    }

    nnz_index = 0;
    for(i=0; i < nr_of_dpus; i++){
        for(j=0; j<new_nnz_index[i]; j++){
            new_nnzs[nnz_index].rowind = new_nnz[i][j].rowind;
            new_nnzs[nnz_index].colind = new_nnz[i][j].colind;
            new_nnzs[nnz_index].val = new_nnz[i][j].val;
            nnz_index++;
        }
    }


    free(mat->nnzs);
    mat->nnzs = new_nnzs;

    //free used resources
    for(i=0;i<nr_of_dpus;i++){
        free(new_nnz[i]);
    }


    int curr_nnz;//
    int curr_row, nnz_sum;//

    uint32_t start_col_partition[num_of_partitions+1];//
    struct elem_t* new_nnz_2[2][num_of_partitions];
    for(i=0;i<num_of_partitions;i++){
        new_nnz_2[0][i] = malloc(max_nnz_per_dpu * sizeof(struct elem_t));
        new_nnz_2[1][i] = malloc(max_nnz_per_dpu * sizeof(struct elem_t));
    }
    int new_nnz_index_2[2][num_of_partitions];

    uint32_t threshold_row[nr_of_dpus];//
    uint32_t dummy_nnz_num_1[nr_of_dpus], dummy_nnz_num_2[nr_of_dpus];//

    for(i=0; i<nr_of_dpus; i++){
        dummy_nnz_num_1[i]=0;
        dummy_nnz_num_2[i]=0;
    }
    
    // add zero elements to appropriate places so that each DPU will receive equal number of elements 
    for(dpu_index=0; dpu_index < nr_of_dpus; dpu_index++){
        curr_nnz = mat->partitions[dpu_index];

        curr_row = dpu_info[dpu_index].prev_rows_dpu;
        nnz_sum = mat->rows[dpu_index % nr_of_partitions][curr_row];

        while(nnz_sum < (curr_nnz/2)){
            curr_row++;
            nnz_sum += mat->rows[dpu_index % nr_of_partitions][curr_row];
        }
        
        if(nnz_sum - (curr_nnz/2) > (curr_nnz/2) - (nnz_sum - mat->rows[dpu_index % nr_of_partitions][curr_row])){
            nnz_sum -= mat->rows[dpu_index % nr_of_partitions][curr_row];
            curr_row--;
        }
        threshold_row[dpu_index] = curr_row;
        if(curr_nnz == max_nnz_per_dpu){
            dummy_nnz_num_1[dpu_index] = 0;
            dummy_nnz_num_2[dpu_index] = 0;
        }
        else if(curr_nnz - nnz_sum > max_nnz_per_dpu/2 ){
            dummy_nnz_num_1[dpu_index] = max_nnz_per_dpu - curr_nnz;
            dummy_nnz_num_2[dpu_index] = 0;
        }
        else if(nnz_sum > max_nnz_per_dpu/2 ){
            dummy_nnz_num_1[dpu_index] = 0;
            dummy_nnz_num_2[dpu_index] = max_nnz_per_dpu - curr_nnz;
        }
        else{
            dummy_nnz_num_1[dpu_index] = max_nnz_per_dpu/2 - nnz_sum;
            dummy_nnz_num_2[dpu_index] = max_nnz_per_dpu/2 - (curr_nnz - nnz_sum);
        }

        //save needed data in dpu_info
        input_args[dpu_index].tasklet_group_nnz_offset = nnz_sum + dummy_nnz_num_1[dpu_index];
    }

    struct elem_t* new_nnzs_2 = (struct elem_t*) malloc((max_nnz_per_dpu * nr_of_dpus) * sizeof(struct elem_t));



    uint32_t prev_nnz_curr=0;
    uint32_t cols_per_dpu;
    uint32_t prev_cols_dpu;

    for(dpu_index=0; dpu_index < nr_of_dpus; dpu_index++){
        cols_per_dpu = dpu_info[dpu_index].cols_per_dpu;
        prev_cols_dpu = dpu_info[dpu_index].prev_cols_dpu;
        if(dpu_index>=1) prev_nnz_curr += mat->partitions[dpu_index-1];

        
        i=prev_cols_dpu;
        start_col_partition[0] = i;
        
        for(index = 0; index < num_of_partitions; index++){
            i += cols_per_dpu/num_of_partitions;
            if(index < (cols_per_dpu % num_of_partitions)) i++;
            start_col_partition[index+1] = i;
        }
        
        //initialize new_nnz_index
        for(i=0;i<num_of_partitions;i++){
            new_nnz_index_2[0][i] = 0;
            new_nnz_index_2[1][i] = 0;
        }

        for(nnz_index = prev_nnz_curr; nnz_index < prev_nnz_curr + mat->partitions[dpu_index]; nnz_index++){
            //check which partition the nnz belongs to and put element in the appropriate partition
            for(i=0; i<num_of_partitions; i++){
                if(new_nnzs[nnz_index].colind < start_col_partition[i+1] && new_nnzs[nnz_index].colind >= start_col_partition[i]) {
                    if(new_nnzs[nnz_index].rowind <= threshold_row[dpu_index]){
                        new_nnz_2[0][i][new_nnz_index_2[0][i]] = new_nnzs[nnz_index];
                        new_nnz_index_2[0][i]++;
                        break;
                    }
                    else{
                        new_nnz_2[1][i][new_nnz_index_2[1][i]] = new_nnzs[nnz_index];
                        new_nnz_index_2[1][i]++;
                        break;
                    }
                }
            }
        }

        //write back data in new order
        nnz_index = dpu_index * max_nnz_per_dpu;
       
        for(i=0; i < num_of_partitions; i++){
            for(j=0;j<new_nnz_index_2[0][i];j++){
                new_nnzs_2[nnz_index].rowind = new_nnz_2[0][i][j].rowind;
                new_nnzs_2[nnz_index].colind = new_nnz_2[0][i][j].colind;
                new_nnzs_2[nnz_index].val = new_nnz_2[0][i][j].val;
                nnz_index++;
            }
        }
        temp.rowind = threshold_row[dpu_index];
        temp.colind = temp.colind = dpu_info[(dpu_index)].prev_cols_dpu + dpu_info[(dpu_index)].cols_per_dpu-1;
        temp.val = 0;
        for(i=0; i<dummy_nnz_num_1[dpu_index]; i++){
            new_nnzs_2[nnz_index].rowind = temp.rowind;
            new_nnzs_2[nnz_index].colind = temp.colind;
            new_nnzs_2[nnz_index].val = 0;
            nnz_index++;
        }
        for(i=0; i < num_of_partitions; i++){
            for(j=0;j<new_nnz_index_2[1][i];j++){
                new_nnzs_2[nnz_index].rowind = new_nnz_2[1][i][j].rowind;
                new_nnzs_2[nnz_index].colind = new_nnz_2[1][i][j].colind;
                new_nnzs_2[nnz_index].val = new_nnz_2[1][i][j].val;
                nnz_index++;
            }
        }
        temp.rowind = dpu_info[(dpu_index)].prev_rows_dpu + dpu_info[(dpu_index)].rows_per_dpu-1;
        temp.colind = dpu_info[(dpu_index)].prev_cols_dpu + dpu_info[(dpu_index)].cols_per_dpu-1;
        temp.val = 0;
        for(i=0; i<dummy_nnz_num_2[dpu_index]; i++){
            new_nnzs_2[nnz_index].rowind = temp.rowind;
            new_nnzs_2[nnz_index].colind = temp.colind;
            new_nnzs_2[nnz_index].val = 0;
            nnz_index++;
        }
    }


    free(mat->nnzs);
    mat->nnzs = new_nnzs_2;

    mat->nnz = max_nnz_per_dpu * nr_of_dpus;

    //free used resources
    for(i=0;i<num_of_partitions;i++){
        free(new_nnz_2[0][i]);
        free(new_nnz_2[1][i]);
    }
    

    return;
}

// => may have to split the matrix in pieces for later use
void reconstruct_weight(struct Matrix *mat, struct dpu_info_t *dpu_info, uint32_t max_cols_per_dpu, uint32_t nr_of_partitions){
    uint32_t nrows = mat->nrows;
    uint32_t ncols = mat->ncols;
    uint32_t cols_per_dpu, prev_cols_dpu;
    int index;

    T* temp = malloc(nrows * max_cols_per_dpu * nr_of_partitions * sizeof(T));
    for(int i=0;i<nr_of_partitions; i++){
        cols_per_dpu = dpu_info[i].cols_per_dpu;
        prev_cols_dpu = dpu_info[i].prev_cols_dpu;

        // if there are no zero-padding in the partition, pass
        //otherwise set the zero-padded column in between
        if(cols_per_dpu != max_cols_per_dpu){
            index = (i * nrows*max_cols_per_dpu);
            for(int k=0; k<cols_per_dpu; k++){
                for(int j=0; j<nrows; j++){
                    //fill matrix in order
                    temp[index] = mat->val[(nrows*prev_cols_dpu) + (k * nrows) + j];
                    index++;
                }
            }
            for(int k = cols_per_dpu; k<max_cols_per_dpu;k++){
                for(int j=0; j<nrows; j++){
                    temp[index] = 0;
                    index++;
                }
            }
        }
        else{
            index = (i * nrows*max_cols_per_dpu);
            for(int k=0; k<cols_per_dpu; k++){
                for(int j=0; j<nrows; j++){
                    //fill matrix in order
                    temp[index] = mat->val[(nrows*prev_cols_dpu) + (k * nrows) + j];
                    index++;
                }
            }
        }
    }
    free(mat->val);
    mat->val = temp;

    return;
}


// => may have to split the matrix in pieces for later use
void reconstruct_mid(struct Matrix *mat, struct dpu_info_t *dpu_info, uint32_t max_rows_per_dpu, uint32_t nr_of_partitions){
    uint32_t nrows = mat->nrows;
    uint32_t ncols = mat->ncols;
    uint32_t rows_per_dpu, prev_rows_dpu;
    int index;

    T* temp = malloc(ncols * max_rows_per_dpu * nr_of_partitions * sizeof(T));
    for(int i=0;i<nr_of_partitions; i++){
        rows_per_dpu = dpu_info[i*nr_of_partitions].rows_per_dpu;
        prev_rows_dpu = dpu_info[i*nr_of_partitions].prev_rows_dpu;

        // if there are no zero-padding in the partition, pass
        //otherwise set the zero-padded column in between
        if(rows_per_dpu != max_rows_per_dpu){
            index = (i * ncols*max_rows_per_dpu);
            for(int k=0; k<rows_per_dpu; k++){
                for(int j=0; j<ncols; j++){
                    //fill matrix in order
                    temp[index] = mat->val[(ncols*prev_rows_dpu) + (k * ncols) + j];
                    index++;
                }
            }
            for(int k = rows_per_dpu; k<max_rows_per_dpu;k++){
                for(int j=0; j<ncols; j++){
                    temp[index] = 0;
                    index++;
                }
            }
        }
        else {
            index = (i * ncols*max_rows_per_dpu);
            for(int k=0; k<rows_per_dpu; k++){
                for(int j=0; j<ncols; j++){
                    //fill matrix in order
                    temp[index] = mat->val[(ncols*prev_rows_dpu) + (k * ncols) + j];
                    index++;
                }
            }
        }
    }

    free(mat->val);
    mat->val = temp;

    return;
}


struct COOMatrix *readCOOMatrix(const char* fileName, uint32_t nr_of_dpus, uint32_t nr_of_partitions, struct partition_info_t *partition_info) {
    struct COOMatrix *cooMtx;
    cooMtx = (struct COOMatrix *) malloc(sizeof(struct COOMatrix));
    FILE* fp = fopen(fileName, "r");
    uint32_t rowindx, colindx;
    int32_t val;
    uint32_t dpu_num;
    char *line; 
    char *token; 
    line = (char *) malloc(1000 * sizeof(char));
    int done = false;
    int i = 0;

    while(fgets(line, 1000, fp) != NULL){
        token = strtok(line, " ");

        if(token[0] == '%'){
            ;
        } else if (done == false) {
            cooMtx->nrows = atoi(token);
            token = strtok(NULL, " ");
            cooMtx->ncols = atoi(token);
            token = strtok(NULL, " ");

            cooMtx->nnz = atoi(token);
            printf("[INFO] %s: %u Rows, %u Cols, %u NNZs\n", strrchr(fileName, '/')+1, cooMtx->nrows, cooMtx->ncols, cooMtx->nnz);
            if((cooMtx->nrows % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->nrows += ((8 / byte_dt) - (cooMtx->nrows % (8 / byte_dt)));
            }
            if((cooMtx->ncols % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->ncols += ((8 / byte_dt) - (cooMtx->ncols % (8 / byte_dt)));
            }

            if((cooMtx->nrows % 1024) != 0) { // Padding needed
                cooMtx->nrows += (1024 - (cooMtx->nrows % 1024));
            }
            if((cooMtx->ncols % 1024) != 0) { // Padding needed
                cooMtx->ncols += (1024 - (cooMtx->ncols % 1024));
            }

            //partition A first
            COO_partition(cooMtx, partition_info, nr_of_partitions);


            cooMtx->rows = (uint32_t **) malloc((nr_of_partitions) * sizeof(uint32_t*));
            for(int j=0; j<nr_of_partitions; j++){
                cooMtx->rows[j] = (uint32_t *) calloc((cooMtx->nrows), sizeof(uint32_t));
            }
            cooMtx->partitions = (uint32_t *) calloc((nr_of_dpus), sizeof(uint32_t));
            cooMtx->nnzs = (struct elem_t *) calloc((cooMtx->nnz+8), sizeof(struct elem_t));
            done = true;
        } else {
            rowindx = atoi(token);
            token = strtok(NULL, " ");
            colindx = atoi(token);
            token = strtok(NULL, " ");
            val = (T) (rand()%2 + 1);

            cooMtx->nnzs[i].rowind = rowindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].colind = colindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].val = val; 

            dpu_num = dpu_num_for_element(partition_info, rowindx - 1, colindx - 1, nr_of_partitions);

            cooMtx->rows[dpu_num % nr_of_partitions][rowindx - 1]++;

            cooMtx->partitions[dpu_num]++;
            i++;
        }
    }

    free(line);
    fclose(fp);

    sortCOOMatrix(cooMtx);

    return cooMtx;
}

#endif