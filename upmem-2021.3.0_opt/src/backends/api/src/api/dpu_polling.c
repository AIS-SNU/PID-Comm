/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <numa.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <dpu_api_verbose.h>
#include <dpu_log_utils.h>
#include <dpu_types.h>
#include <dpu_runner.h>
#include <dpu_management.h>
#include <dpu_rank_handler.h>
#include <dpu_rank.h>
#include <dpu_target_macros.h>
#include <dpu_internals.h>
#include <dpu_mask.h>
#include <dpu_attributes.h>
#include <dpu_program.h>
#include <dpu_thread_job.h>

struct polling_thread {
    /*
     * The goal of this mutex is to protect 'cond'.
     * It makes sure that the polling thread receives a signal when it needs to from the thread calling dpu_boot_dpu/rank
     * functions. All uses of 'cond' need to be protected by 'mutex'.
     */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t id;
    bool is_dpu_running;
    bool is_running;
};

#define POLLING_THREAD_INITIALIZER                                                                                               \
    {                                                                                                                            \
        .mutex = PTHREAD_MUTEX_INITIALIZER, .is_dpu_running = false, .is_running = false,                                        \
    }

static struct polling_thread polling_threads[NR_QUEUES] = { [0 ...(NR_QUEUES - 1)] = POLLING_THREAD_INITIALIZER };

static bool
polling_thread_rank(struct dpu_rank_t *rank)
{
    /* Check if the DPU is running without having taken the lock.
     * If another set nb_dpu_running between the time we check and the time we fail to take the lock, it is not a issue because
     * the thread that launch the rank will set polling_thread_is_dpu_running to true and so the polling thread will not go to
     * sleep.
     */
    dpu_run_context_t run_context = dpu_get_run_context(rank);
    uint32_t nb_dpu_running = run_context->nb_dpu_running;
    if (nb_dpu_running == 0) {
        return false;
    }
    /* If we cannot take the mutex, don't wait for the rank, just skip it, we will try to poll at the next loop iteration.
     */
    if (dpu_trylock_rank(rank) != 0) {
        return true;
    }

    run_context->poll_status = dpu_poll_rank(rank);
    if (run_context->poll_status != DPU_OK) {
        LOG_RANK(WARNING, rank, "Failed to poll");
        pthread_cond_broadcast(&rank->api.poll_cond);
        dpu_unlock_rank(rank);
        return false;
    }

    uint32_t nb_dpu_was_running = nb_dpu_running;
    nb_dpu_running = run_context->nb_dpu_running;
    if (nb_dpu_was_running > nb_dpu_running) {
        pthread_cond_broadcast(&rank->api.poll_cond);
    }

    dpu_unlock_rank(rank);

    return nb_dpu_running != 0;
}

static void *
polling_thread_fct(void *arg)
{
    // printf("%s:%d polling_thread_fct Called\n", __FILE__, __LINE__);
    uint32_t idx = (uint32_t)((uintptr_t)arg);
    struct polling_thread *polling_thread = &polling_threads[idx];

    if (((numa_available() < 0) && (idx != 0)) || ((int)idx > numa_max_node())) {
        polling_thread->is_running = false;
        return NULL;
    }

    numa_run_on_node(idx);

    bool one_rank_running = false;
    while (polling_thread->is_running) 
    {
        pthread_mutex_lock(&polling_thread->mutex);
        if (!polling_thread->is_dpu_running && !one_rank_running && polling_thread->is_running) 
        {
            // waiting for a dpu to be running...
            pthread_cond_wait(&polling_thread->cond, &polling_thread->mutex);
        } 
        else 
        {
            polling_thread->is_dpu_running = false;
            one_rank_running = false;
        }
        pthread_mutex_unlock(&polling_thread->mutex);

        unsigned int nb_ranks;
        struct dpu_rank_t **ranks;
        dpu_rank_handler_start_iteration(&ranks, &nb_ranks);
        // for every rank in ranklist
        // 
        for (unsigned int each_rank = 0; each_rank < nb_ranks; each_rank++) 
        {
            if ((ranks[each_rank] == NULL) || (ranks[each_rank]->api.thread_info.queue_idx != idx)) 
            {
                continue;
            }
            one_rank_running |= polling_thread_rank(ranks[each_rank]);
        }
        dpu_rank_handler_end_iteration();
    }
    return NULL;
}

dpu_error_t
polling_thread_create()
{
    // printf("%s:%d polling_thread_create Called\n", __FILE__, __LINE__);
    /* The polling thread is a singleton which is only created if the user call dpu_alloc
     */
    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) 
    {
        struct polling_thread *polling_thread = &polling_threads[each_queue];

        pthread_mutex_lock(&polling_thread->mutex);
        if (polling_thread->is_running) {
            pthread_mutex_unlock(&polling_thread->mutex);
            return DPU_OK;
        }
        polling_thread->is_running = true;
        pthread_mutex_unlock(&polling_thread->mutex);

        int ret;
        ret = pthread_cond_init(&polling_thread->cond, NULL);
        if (ret != 0) {
            fprintf(stderr, "Could not init polling thread cond (%i)\n", ret);
            return DPU_ERR_SYSTEM;
        }
        ret = pthread_create(&polling_thread->id, NULL, polling_thread_fct, (void *)((uintptr_t)each_queue));
        if (ret != 0) {
            fprintf(stderr, "Could not create polling thread (%i)\n", ret);
            return DPU_ERR_SYSTEM;
        }
        char thread_name[16];
        snprintf(thread_name, 16, "DPU_POLL_%04x", each_queue);
        pthread_setname_np(polling_thread->id, thread_name);
    }

    return DPU_OK;
}

void
polling_thread_set_dpu_running(uint32_t idx)
{
    struct polling_thread *polling_thread = &polling_threads[idx];

    pthread_mutex_lock(&polling_thread->mutex);
    polling_thread->is_dpu_running = true;
    pthread_cond_signal(&polling_thread->cond);
    pthread_mutex_unlock(&polling_thread->mutex);
}

__attribute__((destructor)) static void
polling_thread_destroy()
{
    for (uint32_t each_queue = 0; each_queue < NR_QUEUES; ++each_queue) {
        struct polling_thread *polling_thread = &polling_threads[each_queue];

        pthread_mutex_lock(&polling_thread->mutex);
        if (!polling_thread->is_running) {
            pthread_mutex_unlock(&polling_thread->mutex);
            return;
        }
        polling_thread->is_running = false;
        pthread_cond_signal(&polling_thread->cond);
        pthread_mutex_unlock(&polling_thread->mutex);
        int ret = pthread_join(polling_thread->id, NULL);
        if (ret != 0) {
            fprintf(stderr, "Could not join polling thread at end of execution (%i)\n", ret);
        }
    }
}

static void
dpu_print_lldb_message_on_fault(struct dpu_t *dpu, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id)
{
    const unsigned int nb_dpu_in_fault_to_print = 16;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static unsigned int nb_dpu_in_fault = 0;

    pthread_mutex_lock(&lock);

    bool info_level = false;
    dpu_rank_id_t rank_id = dpu->rank->rank_id & DPU_TARGET_MASK;
    char *dpu_program = dpu->program->program_path;

    nb_dpu_in_fault++;
    if (nb_dpu_in_fault == 1 && dpu->rank->type == HW) {
        LOG(WARNING)
        (__vc(), "One or more DPUs are in fault, attach to them by running: 'dpu-lldb-attach-dpu x.x.x <program_path>'");
    } else if (nb_dpu_in_fault == nb_dpu_in_fault_to_print) {
        LOG(WARNING)
        (__vc(), "Too many DPUs in fault, stop printing. Set \"UPMEM_VERBOSE=-,I\" in your environment see them all");
        LOG(INFO)(__vc(), "Too many DPUs in fault, start printing them in INFO log");
        info_level = true;
    } else if (nb_dpu_in_fault > nb_dpu_in_fault_to_print) {
        info_level = true;
    }

    if (info_level) {
        LOG(INFO)(__vc(), "DPU %u.%u.%u '%s' is in fault", rank_id, slice_id, dpu_id, dpu_program);
    } else {
        LOG(WARNING)(__vc(), "DPU %u.%u.%u '%s' is in fault", rank_id, slice_id, dpu_id, dpu_program);
    }

    pthread_mutex_unlock(&lock);
}

static dpu_error_t
dpu_check_fault_rank(struct dpu_rank_t *rank, dpu_bitfield_t *dpu_in_fault, dpu_error_t status)
{
    dpu_run_context_t run_context = dpu_get_run_context(rank);
    uint8_t nr_of_control_interfaces;

    if (run_context->poll_status != DPU_OK)
        return run_context->poll_status;

    nr_of_control_interfaces = dpu_get_description(rank)->hw.topology.nr_of_control_interfaces;

    for (dpu_slice_id_t each_ci = 0; each_ci < nr_of_control_interfaces; each_ci++) {
        if (dpu_in_fault[each_ci] != run_context->dpu_in_fault[each_ci]) {
            dpu_bitfield_t new_dpu_in_fault = run_context->dpu_in_fault[each_ci] & (~dpu_in_fault[each_ci]);
            dpu_in_fault[each_ci] = run_context->dpu_in_fault[each_ci];
            status = DPU_ERR_DPU_FAULT;
            while (new_dpu_in_fault != 0) {
                uint32_t dpu_id = __builtin_ctz(new_dpu_in_fault);
                dpu_print_lldb_message_on_fault(DPU_GET_UNSAFE(rank, each_ci, dpu_id), each_ci, dpu_id);
                new_dpu_in_fault &= (~(1 << dpu_id));
            }
        }
    }
    return status;
}

dpu_error_t
dpu_sync_rank(struct dpu_rank_t *rank)
{
    LOG_RANK(VERBOSE, rank, "");

    dpu_error_t status = DPU_OK;
    dpu_run_context_t run_context = dpu_get_run_context(rank);
    dpu_bitfield_t dpu_in_fault[DPU_MAX_NR_CIS] = { 0 };
    while (run_context->nb_dpu_running != 0 && (status == DPU_OK || status == DPU_ERR_DPU_FAULT)) {
        pthread_cond_wait(&rank->api.poll_cond, &rank->mutex);
        status = dpu_check_fault_rank(rank, dpu_in_fault, status);
    }
    status = dpu_check_fault_rank(rank, dpu_in_fault, status);

    if (status == DPU_OK) {
        status = dpu_custom_for_rank(rank, DPU_COMMAND_ALL_POSTEXECUTION, NULL);
    }

    return status;
}

dpu_error_t
dpu_sync_dpu(struct dpu_t *dpu)
{
    LOG_DPU(VERBOSE, dpu, "");

    dpu_error_t status;
    struct dpu_rank_t *rank = dpu_get_rank(dpu);
    dpu_run_context_t run_context = dpu_get_run_context(rank);
    dpu_slice_id_t slice_id = dpu_get_slice_id(dpu);
    dpu_member_id_t member_id = dpu_get_member_id(dpu);

    while (dpu_mask_is_selected(run_context->dpu_running[slice_id], member_id)) {
        pthread_cond_wait(&rank->api.poll_cond, &rank->mutex);
    }
    if (dpu_mask_is_selected(run_context->dpu_in_fault[slice_id], member_id)) {
        status = DPU_ERR_DPU_FAULT;
        dpu_print_lldb_message_on_fault(dpu, slice_id, member_id);
        goto exit;
    }

    status = dpu_custom_for_dpu(dpu, DPU_COMMAND_DPU_POSTEXECUTION, NULL);

exit:
    return status;
}

static void
dpu_sync_set_sync_job(struct dpu_thread_job_sync *sync, struct dpu_thread_job *job)
{
    job->type = DPU_THREAD_JOB_SYNC;
    job->sync = sync;
}

__API_SYMBOL__ dpu_error_t
dpu_sync(struct dpu_set_t dpu_set)
{
    LOG_FN(DEBUG, "");

    dpu_error_t status;
    struct dpu_thread_job_sync sync_job;
    struct dpu_rank_t *rank;
    struct dpu_rank_t **ranks;
    uint32_t nr_ranks, nr_jobs_per_rank;
    switch (dpu_set.kind) {
        case DPU_SET_RANKS:
            nr_ranks = dpu_set.list.nr_ranks;
            ranks = dpu_set.list.ranks;
            break;
        case DPU_SET_DPU:
            nr_ranks = 1;
            rank = dpu_get_rank(dpu_set.dpu);
            ranks = &rank;
            break;
        default:
            return DPU_ERR_INTERNAL;
    }
    dpu_thread_job_init_sync_job(&sync_job, nr_ranks);

    DPU_THREAD_JOB_GET_JOBS(ranks, nr_ranks, nr_jobs_per_rank, jobs, &sync_job, false, status);

    switch (dpu_set.kind) {
        case DPU_SET_RANKS: {
            for (uint32_t each_rank = 0; each_rank < nr_ranks; ++each_rank) {
                struct dpu_thread_job *job = jobs[each_rank];
                dpu_sync_set_sync_job(&sync_job, job);
            }
        } break;
        case DPU_SET_DPU: {
            dpu_sync_set_sync_job(&sync_job, jobs[0]);
        } break;
    }

    return dpu_thread_job_do_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs, true, &sync_job);
}
