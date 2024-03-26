/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <dpu_api_verbose.h>
#include <dpu_rank.h>
#include <dpu_transfer_matrix.h>
#include <dpu_thread_job.h>

#include <dpu_memory.h>
#include <dpu_program.h>
#include <dpu_error.h>
#include <dpu_types.h>
#include <dpu_internals.h>
#include <dpu_log_utils.h>
#include <dpu_management.h>
#include <dpu_attributes.h>
#include <dpu_api_memory.h>
#include <dpu_api_load.h>
#include <dpu_polling.h>
#include <numa.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/queue.h>

STAILQ_HEAD(dpu_thread_job_rank_list, dpu_thread_job);

struct job_queue {
    struct dpu_thread_job_rank_list list;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

#define QUEUE_INITIALIZER(queue)                                                                                                 \
    {                                                                                                                            \
        .list = STAILQ_HEAD_INITIALIZER(queue.list), .mutex = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER,       \
    }

static struct job_queue queues[NR_QUEUES] = {
    QUEUE_INITIALIZER(queues[0]),
    QUEUE_INITIALIZER(queues[1]),
    QUEUE_INITIALIZER(queues[2]),
    QUEUE_INITIALIZER(queues[3]),
};

static struct dpu_thread_job *
dpu_thread_job_peek_unlocked(struct dpu_thread_job_list *jobs)
{
    return STAILQ_FIRST(jobs);
}

static struct dpu_thread_job *
dpu_thread_job_pop_unlocked(struct dpu_thread_job_list *jobs)
{
    struct dpu_thread_job *job = dpu_thread_job_peek_unlocked(jobs);
    STAILQ_REMOVE_HEAD(jobs, next_job);
    return job;
}

static void
dpu_thread_release_job_slot_unlocked(struct dpu_rank_t *rank, struct dpu_thread_job *job)
{
    bool stailq_empty = STAILQ_EMPTY(&rank->api.available_jobs);
    STAILQ_INSERT_TAIL(&rank->api.available_jobs, job, next_job);
    rank->api.available_jobs_length++;
    if (stailq_empty) {
        pthread_cond_signal(&rank->api.available_jobs_cond);
    }
}

static void
dpu_thread_insert_rank_list(struct dpu_thread_job *first_job, uint32_t queue_idx)
{
    if (first_job != NULL) {
        struct job_queue *queue = &queues[queue_idx];
        pthread_mutex_lock(&queue->mutex);
        STAILQ_INSERT_TAIL(&queue->list, first_job, next_rank);
        pthread_cond_broadcast(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
    }
}

static struct dpu_thread_job *
dpu_thread_advance_to_next_job(struct dpu_rank_t *rank)
{
    struct dpu_thread_job *job;

    pthread_mutex_lock(&rank->api.jobs_mutex);

    job = dpu_thread_job_pop_unlocked(&rank->api.jobs);
    dpu_thread_release_job_slot_unlocked(rank, job);
    job = dpu_thread_job_peek_unlocked(&rank->api.jobs);

    pthread_mutex_unlock(&rank->api.jobs_mutex);
    return job;
}

static void
dpu_thread_handle_global_block(struct dpu_rank_t *rank, struct dpu_rank_t **ranks, uint32_t nr_ranks)
{
    bool queue_is_used[NR_QUEUES] = { [0 ...(NR_QUEUES - 1)] = false };

    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
        struct dpu_rank_t *other_rank = ranks[each_rank];

        if (other_rank != rank) {
            uint32_t queue_idx = other_rank->api.thread_info.queue_idx;
            struct job_queue *queue = &queues[queue_idx];
            if (!queue_is_used[queue_idx]) {
                pthread_mutex_lock(&queue->mutex);
                queue_is_used[queue_idx] = true;
            }

            struct dpu_thread_job *next_rank_job = dpu_thread_advance_to_next_job(other_rank);
            STAILQ_INSERT_TAIL(&queue->list, next_rank_job, next_rank);
        }
    }

    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) {
        if (queue_is_used[each_queue]) {
            struct job_queue *queue = &queues[each_queue];
            pthread_cond_broadcast(&queue->cond);
            pthread_mutex_unlock(&queue->mutex);
        }
    }
}

static void
do_sync_job(struct dpu_thread_job_sync *sync)
{
    if (__sync_sub_and_fetch(&sync->nr_ranks, 1) == 0) {
        pthread_mutex_lock(&sync->mutex);
        pthread_cond_signal(&sync->cond);
        pthread_mutex_unlock(&sync->mutex);
    }
}

static dpu_error_t
dpu_thread_compute_job(struct dpu_rank_t *rank, struct dpu_thread_job *job, bool *keep_job_list)
{
    dpu_error_t status = DPU_OK;
    *keep_job_list = true;

    switch (job->type) {
        case DPU_THREAD_JOB_LAUNCH_RANK: {
            status = dpu_boot_rank(rank);
            if (status != DPU_OK) {
                break;
            }
            polling_thread_set_dpu_running(rank->api.thread_info.queue_idx);
            // Executing 'dpu_launch' inside a 'dpu_callback' performs two lock of the rank.
            // In order to let the polling doing its job, we need to unlock it once before going in the 'dpu_sync_rank' function
            if (rank->api.callback_tid_set) {
                dpu_unlock_rank(rank);
            }
            status = dpu_sync_rank(rank);
            if (rank->api.callback_tid_set) {
                dpu_lock_rank(rank);
            }
        } break;
        case DPU_THREAD_JOB_LAUNCH_DPU: {
            status = dpu_boot_dpu(job->dpu);
            if (status != DPU_OK) {
                break;
            }
            polling_thread_set_dpu_running(job->dpu->rank->api.thread_info.queue_idx);
            if (job->dpu->rank->api.callback_tid_set) {
                dpu_unlock_rank(job->dpu->rank);
            }
            status = dpu_sync_dpu(job->dpu);
            if (job->dpu->rank->api.callback_tid_set) {
                dpu_lock_rank(job->dpu->rank);
            }
        } break;
        case DPU_THREAD_JOB_SYNC:
            do_sync_job(job->sync);
            break;
        case DPU_THREAD_JOB_COPY_WRAM_TO_MATRIX:
            status = dpu_copy_to_wram_for_matrix(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_IRAM_TO_MATRIX:
            status = dpu_copy_to_iram_for_matrix(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_MRAM_TO_MATRIX:
            status = dpu_copy_to_mrams(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_WRAM_FROM_MATRIX:
            status = dpu_copy_from_wram_for_matrix(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_IRAM_FROM_MATRIX:
            status = dpu_copy_from_iram_for_matrix(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_MRAM_FROM_MATRIX:
            status = dpu_copy_from_mrams(rank, &job->matrix);
            break;
        case DPU_THREAD_JOB_COPY_WRAM_TO_RANK:
            status = dpu_copy_to_wram_for_rank(rank, job->address, job->buffer, job->length);
            break;
        case DPU_THREAD_JOB_COPY_IRAM_TO_RANK:
            status = dpu_copy_to_iram_for_rank(rank, job->address, job->buffer, job->length);
            break;
        case DPU_THREAD_JOB_COPY_MRAM_TO_RANK: {
            struct dpu_transfer_matrix matrix;
            dpu_transfer_matrix_set_all(rank, &matrix, (void *)job->buffer);
            matrix.offset = job->address;
            matrix.size = job->length;
            status = dpu_copy_to_mrams(rank, &matrix);
        } break;
        case DPU_THREAD_JOB_LOAD: {
            status = dpu_load_rank(rank, job->load_info.runtime, job->load_info.elf_info);
        } break;
        case DPU_THREAD_JOB_CALLBACK: {
            struct dpu_thread_job *master = job->callback.master_job;
            bool master_job;
            if (!(master_job = (master != NULL))) {
                master = job;
            }

            bool is_sync = master->callback.is_sync;
            bool is_single_call = master->callback.is_single_call;
            bool is_nonblocking = master->callback.is_nonblocking;

            if (__sync_sub_and_fetch(&master->callback.sync.nr_ranks, 1) == 0) {
                struct dpu_set_t dpu_set = master->callback.dpu_set;
                dpu_error_t (*function)(struct dpu_set_t, uint32_t, void *) = master->callback.function;
                uint32_t rank_idx = master->callback.rank_idx;
                void *args = master->callback.args;

                uint32_t nr_ranks;
                struct dpu_rank_t **ranks;
                struct dpu_rank_t *rank_dpu_set_dpu;
                switch (dpu_set.kind) {
                    case DPU_SET_RANKS:
                        nr_ranks = dpu_set.list.nr_ranks;
                        ranks = dpu_set.list.ranks;
                        break;
                    case DPU_SET_DPU:
                        nr_ranks = 1;
                        rank_dpu_set_dpu = dpu_get_rank(dpu_set.dpu);
                        ranks = &rank_dpu_set_dpu;
                        break;
                    default:
                        return DPU_ERR_INTERNAL;
                }

                // For nonblocking job, we need to free the rank FIFO is order for it to continue computing job in parallel of its
                // current job
                if (is_nonblocking) {
                    *keep_job_list = false;
                    dpu_unlock_rank(rank);
                    struct dpu_thread_job *next_rank_job = dpu_thread_advance_to_next_job(rank);
                    dpu_thread_insert_rank_list(next_rank_job, rank->api.thread_info.queue_idx);
                }

                // Lock every rank to sync with them
                if (is_single_call && !is_nonblocking) {
                    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
                        dpu_lock_rank(ranks[each_rank]);
                    }
                }

                if (!is_nonblocking) {
                    pthread_t tid = pthread_self();
                    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
                        ranks[each_rank]->api.callback_tid = tid;
                        ranks[each_rank]->api.callback_tid_set = true;
                    }
                }

                status = function(dpu_set, rank_idx, args);

                if (!is_nonblocking) {
                    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
                        ranks[each_rank]->api.callback_tid_set = false;
                    }
                }

                if (is_single_call && !is_nonblocking) {
                    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
                        dpu_unlock_rank(ranks[each_rank]);
                    }
                }

                // For blocking single-call we need to wake-up every rank FIFO at the of the callback
                if (!is_nonblocking && is_single_call && !is_sync) {
                    dpu_thread_handle_global_block(rank, ranks, nr_ranks);
                }

                // If a master_job was used, it needs to be free
                if (master_job) {
                    pthread_mutex_lock(&master->callback.slot_owner->api.jobs_mutex);
                    dpu_thread_release_job_slot_unlocked(master->callback.slot_owner, master);
                    pthread_mutex_unlock(&master->callback.slot_owner->api.jobs_mutex);
                }
            } else if (!is_nonblocking && is_single_call && !is_sync) {
                dpu_unlock_rank(rank);
                *keep_job_list = false;
            }

            break;
        }
        case DPU_THREAD_JOB_COPY_MRAM_TO_MRAM_CUSTOM:
        {
            printf("JOB DPU_THREAD_JOB_COPY_MRAM_TO_MRAM_CUSTOM Called\n");
            break;
        }
    
    }

    return status;
}

static void
dpu_thread_update_status(dpu_error_t status, struct dpu_rank_t *rank, struct dpu_thread_job *job)
{
    pthread_mutex_lock(&rank->api.jobs_mutex);

    status = DPU_ERR_ASYNC_JOBS | (job->type << DPU_ERROR_ASYNC_JOB_TYPE_SHIFT)
        | (status & ((1 << DPU_ERROR_ASYNC_JOB_TYPE_SHIFT) - 1));
    rank->api.job_error = status;

    // Notify every sync job enqueue in the rank's job list and clean the list
    while (!STAILQ_EMPTY(&rank->api.jobs)) {
        job = dpu_thread_job_pop_unlocked(&rank->api.jobs);
        if (job->type == DPU_THREAD_JOB_SYNC) {
            pthread_mutex_lock(&job->sync->mutex);
            if (job->sync->status == DPU_OK) {
                job->sync->status = status;
            }
            pthread_mutex_unlock(&job->sync->mutex);
            do_sync_job(job->sync);
        }
        dpu_thread_release_job_slot_unlocked(rank, job);
    }
    pthread_mutex_unlock(&rank->api.jobs_mutex);
}

static void *
dpu_thread_job_fct(void *arg)
{
    // printf("%s:%d dpu_thread_job_fct Created\n", __FILE__, __LINE__);
    
    struct dpu_thread_job_info *args = (struct dpu_thread_job_info *)arg;
    bool *should_stop = &args->should_stop;
    struct job_queue *queue = &queues[args->queue_idx];

    if (numa_available() >= 0) 
    {
        numa_run_on_node(args->queue_idx);
    }

    while (true) 
    {
    main_loop:
        pthread_mutex_lock(&queue->mutex);
        // Look for a rank on which to perform async operations
        while (STAILQ_EMPTY(&queue->list) && !*should_stop) 
        {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        // Check if we have been awaken to stop or to compute
        if (*should_stop) 
        {
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
        
        // Get the first job of the rank on which to perform async operations
        struct dpu_thread_job *job = STAILQ_FIRST(&queue->list);
        STAILQ_REMOVE_HEAD(&queue->list, next_rank);
        pthread_mutex_unlock(&queue->mutex);

        // Lock the rank and do async operations as long as the FIFO is not empty
        struct dpu_rank_t *rank = job->rank;
        
        dpu_lock_rank(rank);
        
        while (job != NULL) 
        {
            bool keep_job_list;

            dpu_error_t status = dpu_thread_compute_job(rank, job, &keep_job_list);
            if (!keep_job_list) 
            {
                goto main_loop;
            }
            if (status != DPU_OK) 
            {
                dpu_thread_update_status(status, rank, job);
                break;
            }

            job = dpu_thread_advance_to_next_job(rank);
        }
        
        dpu_unlock_rank(rank);
    }
    return NULL;
}

void
dpu_thread_job_unget_jobs(struct dpu_rank_t **ranks, uint32_t nr_ranks, uint32_t nr_jobs, struct dpu_thread_job **job_list)
{
    for (uint32_t each_rank = 0; each_rank < nr_ranks; each_rank++) {
        struct dpu_rank_t *rank = ranks[each_rank];
        pthread_mutex_lock(&rank->api.jobs_mutex);
        for (uint32_t each_job = 0; each_job < nr_jobs; each_job++) {
            struct dpu_thread_job *job = job_list[each_rank * nr_jobs + each_job];
            if (job != NULL) {
                STAILQ_INSERT_HEAD(&rank->api.available_jobs, job, next_job);
                rank->api.available_jobs_length++;
            }
        }
        pthread_mutex_unlock(&rank->api.jobs_mutex);
    }
}

dpu_error_t
dpu_thread_job_get_job_unlocked(struct dpu_rank_t *rank, struct dpu_thread_job **job)
{
    do {
        if (rank->api.job_error != DPU_OK) {
            return rank->api.job_error;
        }
        if (!STAILQ_EMPTY(&rank->api.available_jobs)) {
            break;
        }
        pthread_cond_wait(&rank->api.available_jobs_cond, &rank->api.jobs_mutex);
    } while (true);

    *job = dpu_thread_job_pop_unlocked(&rank->api.available_jobs);
    rank->api.available_jobs_length--;

    return DPU_OK;
}

dpu_error_t
dpu_thread_job_get_jobs(struct dpu_rank_t **ranks, uint32_t nr_ranks, uint32_t nr_jobs, struct dpu_thread_job **job_list)
{
    for (uint32_t each_rank = 0; each_rank < nr_ranks; each_rank++) {
        struct dpu_rank_t *rank = ranks[each_rank];
        pthread_mutex_lock(&rank->api.jobs_mutex);
        for (uint32_t each_job = 0; each_job < nr_jobs; each_job++) {
            struct dpu_thread_job *job;
            dpu_error_t status = dpu_thread_job_get_job_unlocked(rank, &job);
            if (status != DPU_OK) {
                dpu_thread_job_unget_jobs(ranks, each_rank, nr_jobs, job_list);
                pthread_mutex_unlock(&rank->api.jobs_mutex);
                return status;
            }
            job_list[each_rank * nr_jobs + each_job] = job;
        }
        pthread_mutex_unlock(&rank->api.jobs_mutex);
    }
    return DPU_OK;
}

static void
dpu_thread_job_add_jobs(struct dpu_rank_t **ranks, uint32_t nr_ranks, uint32_t nr_jobs, struct dpu_thread_job **jobs_to_add)
{
    // printf("\tnr_jobs: %d\n", nr_jobs);
    
    // for (uint32_t i = 0; i < nr_jobs; i++)
    // {
    //     printf("\t\tJob Type: %d\n", jobs_to_add[i]->type); 
    // }

    bool signal_queue[NR_QUEUES] = { [0 ...(NR_QUEUES - 1)] = false };

    for (uint32_t each_rank = 0; each_rank < nr_ranks; each_rank++) 
    {
        struct dpu_rank_t *rank = ranks[each_rank];
        uint32_t job_id = each_rank * nr_jobs;
        pthread_mutex_lock(&rank->api.jobs_mutex);
        
        if (STAILQ_EMPTY(&rank->api.jobs)) 
        {
            uint32_t queue_idx = rank->api.thread_info.queue_idx;
            struct job_queue *queue = &queues[queue_idx];
            if (!signal_queue[queue_idx]) 
            {
                signal_queue[queue_idx] = true;
                pthread_mutex_lock(&queue->mutex);
            }
            STAILQ_INSERT_TAIL(&queue->list, jobs_to_add[job_id], next_rank);
        }

        for (uint32_t each_job = 0; each_job < nr_jobs; each_job++) 
        {
            STAILQ_INSERT_TAIL(&rank->api.jobs, jobs_to_add[job_id + each_job], next_job);
        }
        pthread_mutex_unlock(&rank->api.jobs_mutex);
    }

    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) 
    {
        if (signal_queue[each_queue]) 
        {
            struct job_queue *queue = &queues[each_queue];
            pthread_cond_broadcast(&queue->cond);
            pthread_mutex_unlock(&queue->mutex);
        }
    }
}

static dpu_error_t
dpu_thread_job_wait_sync_job(struct dpu_thread_job_sync *sync)
{
    do {
        pthread_cond_wait(&sync->cond, &sync->mutex);
    } while (sync->nr_ranks != 0);
    return sync->status;
}

dpu_error_t
dpu_thread_job_do_jobs(struct dpu_rank_t **ranks,
    uint32_t nr_ranks,
    uint32_t nr_job_per_rank,
    struct dpu_thread_job **jobs,
    bool synchronous,
    struct dpu_thread_job_sync *sync)
{

    dpu_error_t status = DPU_OK;
    if (synchronous && (nr_ranks == 1) && (ranks[0]->api.callback_tid_set) && (jobs[0]->type != DPU_THREAD_JOB_SYNC)) 
    {
        bool ignored;
        dpu_lock_rank(ranks[0]);
        status = dpu_thread_compute_job(ranks[0], jobs[0], &ignored);
        dpu_unlock_rank(ranks[0]);
    } 
    else if (synchronous) 
    {
        // DBG
        dpu_thread_job_add_jobs(ranks, nr_ranks, nr_job_per_rank, jobs);
        status = dpu_thread_job_wait_sync_job(sync) & ((1 << DPU_ERROR_ASYNC_JOB_TYPE_SHIFT) - 1);
        if (status != DPU_OK) 
        {
            /* The job failed on at least one rank.
             * In the general async case, we do not handle a clean restart (we need to allocate the rank again).
             * However, for synchronous jobs, the job is the only one in the queue (API assumption). We know
             * exactly what jobs have been executed, thus we can clean the queue to a coherent state!
             * The error status will be returned to the caller via status, we can set back job_error to DPU_OK.
             * This is safe to do because no other sync job is waiting (again because this is the only job in
             * the queue).
             */
            for (uint32_t rank = 0; rank < nr_ranks; ++rank) 
            {
                ranks[rank]->api.job_error = DPU_OK;
            }
        }
    } else 
    {
        dpu_thread_job_add_jobs(ranks, nr_ranks, nr_job_per_rank, jobs);
    }
    return status;
}

void
dpu_thread_job_init_sync_job(struct dpu_thread_job_sync *sync, uint32_t nr_ranks)
{
    pthread_mutex_init(&sync->mutex, NULL);
    pthread_mutex_lock(&sync->mutex);
    pthread_cond_init(&sync->cond, NULL);
    sync->nr_ranks = nr_ranks;
    sync->status = DPU_OK;
}

dpu_error_t
dpu_thread_job_create(struct dpu_rank_t **ranks, uint32_t nr_ranks)
{
    // printf("%s:%d dpu_thread_job_create Called\n", __FILE__, __LINE__);
    
    dpu_error_t status = DPU_OK;
    bool done[nr_ranks];

    memset(done, false, sizeof(done));

    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) 
    {
        pthread_mutex_lock(&queues[each_queue].mutex);
    }

    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) 
    {
        struct dpu_rank_t *rank = ranks[each_rank];
        rank->api.jobs_table = calloc(rank->api.nr_jobs, sizeof(struct dpu_thread_job));
        
        if (rank->api.jobs_table == NULL) 
        {
            status = DPU_ERR_SYSTEM;
            goto free_unlock_and_exit;
        }

        STAILQ_INIT(&rank->api.available_jobs);
        STAILQ_INIT(&rank->api.jobs);

        pthread_cond_init(&rank->api.available_jobs_cond, NULL);
        pthread_mutex_init(&rank->api.jobs_mutex, NULL);

        rank->api.job_error = DPU_OK;

        for (uint32_t each_job = 0; each_job < rank->api.nr_jobs; each_job++) 
        {
            rank->api.jobs_table[each_job].rank = rank;
            STAILQ_INSERT_HEAD(&rank->api.available_jobs, &rank->api.jobs_table[each_job], next_job);
        }
        rank->api.available_jobs_length = rank->api.nr_jobs;

        // We need to clean the matrix so that 'dpu-profiling memory-transfer' can follow how many DPUs are used for 1 transfer
        dpu_transfer_matrix_clear_all(rank, &(rank->api.matrix));
        dpu_transfer_matrix_clear_all(rank, &(rank->api.callback_matrix));

        rank->api.callback_tid_set = false;
        rank->api.thread_info.should_stop = false;
        rank->api.thread_info.queue_idx = (rank->numa_node == -1) ? 0 : rank->numa_node;

        if (rank->api.thread_info.queue_idx >= NR_QUEUES) 
        {
            LOG_RANK(WARNING, rank, "the API does not support %d queues", rank->api.thread_info.queue_idx + 1);
            status = DPU_ERR_SYSTEM;
            goto free_unlock_and_exit;
        }

        done[each_rank] = true;

        char thread_name[16];

        memset(rank->api.threads, -1, sizeof(rank->api.threads[0]) * rank->api.nr_threads);

        // printf("%s:%d rank->api.nr_threads: %d\n", __FILE__, __LINE__, rank->api.nr_threads);
        
        for (uint32_t each_thread = 0; each_thread < rank->api.nr_threads; each_thread++) 
        {
            if (pthread_create(&rank->api.threads[each_thread], NULL, dpu_thread_job_fct, (void *)&rank->api.thread_info) != 0) 
            {
                status = DPU_ERR_SYSTEM;
                goto free_unlock_and_exit;
            }
            snprintf(thread_name, 16, "DPU_WORK_%04x", ((each_rank & 0xff) << 8) | (each_thread & 0xff));
            pthread_setname_np(rank->api.threads[each_thread], thread_name);
        }
    }
    goto unlock_and_exit;

free_unlock_and_exit:

    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) 
    {
        struct dpu_rank_t *rank = ranks[each_rank];

        if (!done[each_rank]) 
        {
            continue;
        }

        rank->api.thread_info.should_stop = true;
        struct job_queue *queue = &queues[rank->api.thread_info.queue_idx];
        
        pthread_cond_broadcast(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
        
        for (uint32_t each_thread = 0; each_thread < rank->api.nr_threads; each_thread++) 
        {
            if (rank->api.threads[each_thread] != (pthread_t)-1) 
            {
                pthread_join(rank->api.threads[each_thread], NULL);
            }
        }
        pthread_mutex_lock(&queue->mutex);

        free(rank->api.jobs_table);
        rank->api.nr_threads = 0;
    }
unlock_and_exit:

    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) 
    {
        pthread_mutex_unlock(&queues[each_queue].mutex);
    }
    return status;
}

void
dpu_thread_job_free(struct dpu_rank_t **ranks, uint32_t nr_ranks)
{
    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) {
        pthread_mutex_lock(&queues[each_queue].mutex);
    }
    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
        struct dpu_rank_t *rank = ranks[each_rank];

        rank->api.thread_info.should_stop = true;
    }
    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) {
        struct job_queue *queue = &queues[each_queue];
        pthread_cond_broadcast(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
    }

    for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
        struct dpu_rank_t *rank = ranks[each_rank];

        for (uint32_t each_thread = 0; each_thread < rank->api.nr_threads; each_thread++) {
            pthread_join(rank->api.threads[each_thread], NULL);
        }

        free(rank->api.jobs_table);
    }
}
