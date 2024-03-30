# [ISCA'24] PID-Comm: A Fast and Flexible Collective Communication Framework for Commodity Processing-in-DIMMs
This repository contains the tutorial code for PID-Comm, a fast and flexible collective communication framework for commodity Processing-in-DIMMs.
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
- tutorial/ #Turorial code for PID-Comm
- pidcomm_lib/ #Implementations of supported communication primitives and DPU binary codes.
- upmem-2021.3.0_opt/ #Modified UPMEM driver for PID-Comm
- upmem-2021.3.0_opt/include/dpu/pidcomm_lib.h #Declarations of supported communication primitives

## Environment Setup
```
cd {your PID-Comm dir};
source ./upmem-2021.3.0_opt/upmem_env.sh
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

Assume there are eight nodes in communication, each containing 4 integers [0, 1, 2, 3].
After executing pidcomm_all_reduce(), all nodes will contain the same result [0, 8, 16, 24], the sum of the integers in the communicating nodes.

The instructions below provide instructions on how to use PID-Comm. 
First, include the PID-Comm header files with ```#include <pidcomm.h>```.
After the previous step, you need to configure the hypercube settings.
Here is an example:
```
uint32_t nr_dpus = 1024; //The number of DPUs
uint32_t dimension=3; //Dimension of the hypercube
uint32_t axis_len[dimension]; //The number of DPUs for each axis of the hypercube
axis_len[0]=32; //x-axis
axis_len[1]=32; //y-axis
axis_len[2]=1;  //z-axis
```
Then, please set the remaining variables required for the PID-Comm.
```
uint32_t start_offset=0; //Offset of source.
uint32_t target_offset=0; //Offset of destination.
uint32_t data_size_per_dpu = 64*axis_len[0]; //data size for each nodes
uint32_t buffer_offset=1024*1024*32; //For effective communication, PID-Comm required buffer. The buffer's offset must be greater than the sum of the start_offset and the data_size_per_dpu.
```
Allocate the DPUs and then initialize the hypercube manager.
```
DPU_ASSERT(dpu_alloc(nr_dpus, NULL, &dpu_set));
hypercube_manager* hypercube_manager = init_hypercube_manager(dpu_set, dimension, axis_len);
```
Now PID-Comm's settings have been completed.
The following line of code is used to execute pidcomm_allreduce().
The parameter "100" refers to the axis used in communication, which is the x-axis in this case.
```
pidcomm_all_reduce(hypercube_manager, "100", data_size_per_dpu, start_offset, target_offset, buffer_offset, sizeof(T), 0);
```

Note that a dummy binary file, DPU_BINARY_USER, is loaded in the DPUs for the tutorial.
A custom binary file may be used to replace our current dummy binary file.

A script is also available to test the tutorial code.
```
cd tutorial;
./AllReduce_demo.sh;
```
