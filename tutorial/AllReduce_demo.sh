#gcc --std=c99 -o bin/example_allreduce example_allreduce.c -DT=int8_t -DF=int8_t ../cl_src/cl_alloc_prepare_push.c ../cl_src/cl_communication.c -fopenmp `dpu-pkg-config --cflags --libs dpu`
gcc --std=c99 -o bin/example_allreduce example_allreduce.c -DT=int32_t -DF=int32_t -DUPMEM_HOME=UPMEM_HOME -fopenmp `dpu-pkg-config --cflags --libs dpu`
./bin/example_allreduce