/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * To perform multiple-rank operations, the High-level Host API is using threads to do it in parallel.
 *
 * The operation to perform is given to the threads through a linked-list of struct dpu_job (made with <sys/queue.h>).
 * This linked-list is protected by a mutex. Jobs are added through the dpu_thread_job_add_jobs function which has a helper macro
 * (DPU_THREAD_ADD_JOBS_AND_WAIT) which ensure that the arguments given to the function are well formated. It also wait for the
 * completion of all the added operations.
 *
 * Wait for completion is done with the struct dpu_event which contains:
 * - the dpu_error_t to be returned by the function performing the multiple-rank operation.
 * - a counter (management with gcc atomic primitives) to count how many jobs are still to be perform.
 * - a condition to wake up the thread waiting for the multiple-rank operation and a mutex to protect the condition.
 *
 * The default number of threads to perform thoses operations is one per allocated rank. But it can be change with the
 * nrThreadPerRank properties in the UPMEM_PROFILE.
 */

#ifndef DPU_THREAD_JOB_H
#define DPU_THREAD_JOB_H

#include <stdint.h>
#include <pthread.h>

#include <dpu_rank.h>
#include <dpu_error.h>
#include <dpu_types.h>

#include <dpu.h>

#define NR_QUEUES (4)

dpu_error_t
dpu_thread_job_create(struct dpu_rank_t **ranks, uint32_t nr_ranks);

void
dpu_thread_job_free(struct dpu_rank_t **ranks, uint32_t nr_ranks);

void
dpu_thread_job_unget_jobs(struct dpu_rank_t **ranks, uint32_t nr_ranks, uint32_t nr_jobs, struct dpu_thread_job **job_list);

dpu_error_t
dpu_thread_job_get_job_unlocked(struct dpu_rank_t *rank, struct dpu_thread_job **job);

dpu_error_t
dpu_thread_job_get_jobs(struct dpu_rank_t **ranks, uint32_t nr_ranks, uint32_t nr_jobs, struct dpu_thread_job **job_list);

dpu_error_t
dpu_thread_job_do_jobs(struct dpu_rank_t **ranks,
    uint32_t nr_ranks,
    uint32_t nr_job_per_rank,
    struct dpu_thread_job **jobs,
    bool synchronous,
    struct dpu_thread_job_sync *sync);

void
dpu_thread_job_init_sync_job(struct dpu_thread_job_sync *sync, uint32_t nr_ranks);

#define DPU_THREAD_JOB_GET_JOBS(ranks, nr_ranks, nr_jobs_per_rank, jobs, struct_sync, bool_sync, status)                         \
    if ((bool_sync) && (((nr_ranks) != 1) || !(ranks)[0]->api.callback_tid_set)) {                                               \
        dpu_thread_job_init_sync_job((struct_sync), (nr_ranks));                                                                 \
        (nr_jobs_per_rank) = 2;                                                                                                  \
    } else {                                                                                                                     \
        (nr_jobs_per_rank) = 1;                                                                                                  \
    }                                                                                                                            \
    struct dpu_thread_job *jobs[(nr_ranks) * (nr_jobs_per_rank)];                                                                \
    struct dpu_thread_job static_job;                                                                                            \
    memset(jobs, 0, sizeof(struct dpu_thread_job *) * ((nr_ranks) * (nr_jobs_per_rank)));                                        \
    if ((bool_sync) && ((nr_ranks) == 1) && (ranks)[0]->api.callback_tid_set) {                                                  \
        jobs[0] = &static_job;                                                                                                   \
        status = DPU_OK;                                                                                                         \
    } else {                                                                                                                     \
        status = dpu_thread_job_get_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs);                                               \
        if (status != DPU_OK) {                                                                                                  \
            return status;                                                                                                       \
        }                                                                                                                        \
    }

#define DPU_THREAD_JOB_GET_JOBS_RNS(ranks, nr_ranks, nr_jobs_per_rank, jobs, static_job, struct_sync, bool_sync, status)                         \
    if ((bool_sync) && (((nr_ranks) != 1) || !(ranks)[0]->api.callback_tid_set)) {                                               \
        dpu_thread_job_init_sync_job((struct_sync), (nr_ranks));                                                                 \
        (nr_jobs_per_rank) = 2;                                                                                                  \
    } else {                                                                                                                     \
        (nr_jobs_per_rank) = 1;                                                                                                  \
    }                                                                                                                            \
    struct dpu_thread_job *jobs[(nr_ranks) * (nr_jobs_per_rank)];                                                                \
    struct dpu_thread_job static_job;                                                                                            \
    memset(jobs, 0, sizeof(struct dpu_thread_job *) * ((nr_ranks) * (nr_jobs_per_rank)));                                        \
    if ((bool_sync) && ((nr_ranks) == 1) && (ranks)[0]->api.callback_tid_set) {                                                  \
        jobs[0] = &static_job;                                                                                                   \
        status = DPU_OK;                                                                                                         \
    } else {                                                                                                                     \
        status = dpu_thread_job_get_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs);                                               \
        if (status != DPU_OK) {                                                                                                  \
            return status;                                                                                                       \
        }                                                                                                                        \
    }

#define DPU_THREAD_JOB_SET_JOBS(ranks, rank, nr_ranks, jobs, job, struct_sync, bool_sync, statement)                             \
    do {                                                                                                                         \
        if ((bool_sync) && (((nr_ranks) != 1) || !(ranks)[0]->api.callback_tid_set)) {                                           \
            for (uint32_t _each_rank = 0, _job_id = 0; _each_rank < (nr_ranks); _each_rank++) {                                  \
                rank = ranks[_each_rank];                                                                                        \
                job = (jobs)[_job_id++];                                                                                         \
                { statement };                                                                                                   \
                job = (jobs)[_job_id++];                                                                                         \
                job->type = DPU_THREAD_JOB_SYNC;                                                                                 \
                job->sync = (struct_sync);                                                                                       \
            }                                                                                                                    \
        } else {                                                                                                                 \
            for (uint32_t _each_rank = 0; _each_rank < (nr_ranks); _each_rank++) {                                               \
                rank = ranks[_each_rank];                                                                                        \
                job = (jobs)[_each_rank];                                                                                        \
                { statement };                                                                                                   \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)

#endif /* DPU_THREAD_JOB_H */
