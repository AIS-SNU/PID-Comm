#source ../upmem-2021.3.0_opt/upmem_env.sh
dpu-upmem-dpurte-clang -Wall -Wextra -g -Isupport -O2 -DNR_TASKLETS=16 -o bin/dpu_user dpu_user.c
gcc --std=c99 -o bin/example_allreduce example_allreduce.c -DT=int32_t -DF=int32_t -DUPMEM_HOME=UPMEM_HOME -fopenmp `dpu-pkg-config --cflags --libs dpu`
./bin/example_allreduce