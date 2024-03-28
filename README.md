# [ISCA'24] PID-Comm: A Fast and Flexible Collective Communication Framework for Commodity Processing-in-DIMMs
This repository contains the tutorial code for PID-Comm, a fast and flexible collective communication framework for Commodity Processing-in-DIMMs.
Please cite the paper for more information.

## Required Hardware
- Multicore x86_64 CPU with support for AVX512 instructions.
- DDR4 channels equipped with UPMEM DIMMs

## Prerequisites
- g++
- python3.6
- matplotlib
- numpy
- Pandas
- Scipy
- UPMEM SDK driver (version 2021.3.0, available from the [UPMEM website] (https://sdk.upmem.com/).)

## Directories
- pidcomm_lib/ #Implementations of supported communication primitives
- tutorial/ #Turorial code for PID-Comm
- upmem-2021.3.0/ #Modified UPMEM driver for PID-Comm

## Environment Setup
```
cd {your PID-Comm dir};
source ./upmem-2021.3.0/upmem_env.sh
```

## What is Collective Communication?
Collective communication is a communication pattern that incurs interaction between nodes within a communicator.
PID-Comm supports eight communication primitives below:

<img src="https://github.com/AIS-SNU/PID-Comm/assets/47795554/217239e2-66fc-4b64-bfed-bed93401cbc3" width=100% height=100%>


## Tutorial: PIDComm_All_Reduce()
Many parallel applications, such as dnn, require the reduced results from all nodes.
The function of PIDComm_AllReduce appears as follows:
```
void pidcomm_all_reduce(
    hypercube_manager* manager,
    char* comm_dimensions,
    int total_data_size,
    int src_mram_offset,
    int dst_mram_offset,
    int data_type,
    PIDCOMM_OPERATOR reduction_type)
```

Assume that all eight nodes have a list [0, 1, 2, 3].
When we use pidcomm_allreduce(), each of the eight nodes will have a list [0, 8, 16, 24].
Here is the tutorial code below:
```
...
//For supported communication primitives.
#include <pidcomm.h>

int main(){

    ...

    //Hypercube Configuration
    uint32_t nr_dpus = 32; //the number of DPUs
    uint32_t dimension=3; //hypercuve dimension
    uint32_t axis_len[3]; //The number of DPUs for each axis of the hypercube

    ...

    //Set the variables for the PID-Comm.
    uint32_t start_offset=0; //Offset of source.
    uint32_t target_offset=0; //Offset of destination.
    uint32_t buffer_offset=1024*1024*32; //To ensure effective communication, PID-Comm required buffer. Please ensure that the offset of the buffer is larger than the data size.
    dpu_arguments_comm_t dpu_argument[nr_dpus];

    ...

    //Initialize the hypercube manager
    hypercube_manager* hypercube_manager;
    hypercube_manager = init_hypercube_manager(dpu_set, dimension, axis_len);

    ...


    //Perform Scatter
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, each_dpu, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, original_data+each_dpu*data_num_per_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, data_size_per_dpu, DPU_XFER_DEFAULT));

    //Perform AllReduce by utilizing PID-Comm
    pidcomm_all_reduce(hypercube_manager, "100", data_size_per_dpu, start_offset, target_offset, buffer_offset, sizeof(T), 0);

    //Perform Gather
    DPU_FOREACH_ROTATE_GROUP(dpu_set, dpu, each_dpu, nr_dpus){
        DPU_ASSERT(dpu_prepare_xfer(dpu, original_data+each_dpu*data_num_per_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, data_size_per_dpu, DPU_XFER_DEFAULT));

    ...

    //Print reduced result
    for(int element=0; element<4; element++){
        for(int i=0; i<nr_dpus; i++){
            printf("dpu%d: %2d", i, original_data[i*data_num_per_dpu + element]);
            if(i!=nr_dpus-1) printf(", ");
        }
        printf("\n");
    }

    ...

    return 0;
}
```

A script is available to test the tutorial code.
```
cd tutorial;
./AllReduce_demo.sh;
```