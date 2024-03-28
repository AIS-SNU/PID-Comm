#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <dpu.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <dpu_types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <dpu_transfer_matrix.h>
#include <dpu.h>
#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_management.h>
#include <dpu_program.h>
#include <dpu_memory.h>

#include <pthread.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <sys/sysinfo.h>
#include <sys/time.h>

//common.h
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

typedef struct {
    uint32_t start_offset;
    uint32_t target_offset;
    uint32_t total_data_size;
    uint32_t num_comm_dpu;
    uint32_t each_dpu;
    bool no_rotate;
    uint32_t num_row;
    uint32_t comm_type;
    uint32_t a_length;
    uint32_t num_comm_rg;
} dpu_arguments_comm_t;

//Timer.h
typedef struct Timer{
    struct timeval startTime[12];
    struct timeval stopTime[12];
    double         time[12];

} Timer;

static void resetTimer(Timer *timer){
    for(int i=0; i<12; i++){
        timer->time[i] = 0.0;
    }
}

static void startTimer(Timer *timer, int i) {
    timer->time[i]=0.0;
    gettimeofday(&timer->startTime[i], NULL);
}

static void stopTimer(Timer *timer, int i) {
    gettimeofday(&timer->stopTime[i], NULL);
    timer->time[i] += (timer->stopTime[i].tv_sec - timer->startTime[i].tv_sec) * 1000000.0 +
        (timer->stopTime[i].tv_usec - timer->startTime[i].tv_usec);
}

static void printTimer(Timer *timer, int i) { 
    if (i == 0 || i == 2)
        printf("Time (ms): \t\t\t%f\n", timer->time[i] / (1000)); 
    else if (i == 1)
        printf("Time (ms): \t\t%f\n", timer->time[i] / (1000)); 
    else
        printf("Time (ms): \t%f\n", timer->time[i] / (1000)); 
}
#endif