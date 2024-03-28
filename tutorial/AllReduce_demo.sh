# cd ../upmem-2021.3.0_opt/src/backends/
# ./load.sh
# cd ../../../tutorial/

gcc --std=c99 -o bin/example_allreduce example_allreduce.c -DT=int32_t -DF=int32_t -DUPMEM_HOME=UPMEM_HOME -fopenmp `dpu-pkg-config --cflags --libs dpu`
./bin/example_allreduce