# PID-Comm
This repository contains the tutorial code for PID-Comm, a fast and flexible collective communication framework for Commodity Processing-in-DIMMs.
Please cite the paper for more information.

## System Configuration
- 4 DDR4 channels, each with two UPMEM DIMMs
- 1 DDR4 channel with two 64GB DDR4-2400 DIMMs
- Intel Xeon Gold 5215 CPU
- Ubuntu 18.04 (x64)

## Prerequisites
- g++
- UPMEM SDK driver (version 2021.3, available from the [UPMEM website] (https://sdk.upmem.com/).)

## Directories
- ?/ //Turorial code for PID-Comm
- upmem-2021.3.0/ //Modified UPMEM driver for PID-Comm

## Environment Setup
```
cd {your PID-Comm dir};
source ./upmem-2021.3.0/upmem_env.sh
```

## What is Collective Communication?
Collective communication is a communication pattern that incurs interaction between nodes within a communicator.
PID-Comm supports eight communication primitives below:

<img src="https://github.com/AIS-SNU/PID-Comm-tutorial/files/14735831/background_comm_prim.pdf" width=100% height=100%>


## Tutorial: PIDComm_AllReduce()
Many parallel applications, such as dnn, require the reduced results from all nodes.
The function of PIDComm_AllReduce appears as follows:
```
void pidcomm_allreduce(
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
Here is the tutorial code for previous example.
```

```

```
cd tutorial;
./script_allreduce.sh;
```