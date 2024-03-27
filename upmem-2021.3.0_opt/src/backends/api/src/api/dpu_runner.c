/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu.h>
#include <dpu_rank.h>
#include <dpu_management.h>
#include <dpu_polling.h>
#include <dpu_api_verbose.h>
#include <dpu_types.h>
#include <dpu_error.h>
#include <dpu_attributes.h>
#include <dpu_log_utils.h>
#include <dpu_thread_job.h>
#include <dpu_mask.h>

static const char *
dpu_launch_policy_to_string(dpu_launch_policy_t policy)
{
    switch (policy) {
        case DPU_ASYNCHRONOUS:
            return "ASYNCHRONOUS";
        case DPU_SYNCHRONOUS:
            return "SYNCHRONOUS";
        default:
            return "UNKNOWN";
    }
}

__API_SYMBOL__ dpu_error_t
dpu_launch(struct dpu_set_t dpu_set, dpu_launch_policy_t policy)
{
    dpu_error_t status = DPU_OK;
    LOG_FN(DEBUG, "%s", dpu_launch_policy_to_string(policy));

    if (dpu_set.kind != DPU_SET_RANKS && dpu_set.kind != DPU_SET_DPU) {
        return DPU_ERR_INTERNAL;
    }

    uint32_t nr_ranks;
    struct dpu_rank_t **ranks;
    struct dpu_rank_t *rank;
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
    }

    struct dpu_thread_job_sync sync;
    uint32_t nr_jobs_per_rank;
    DPU_THREAD_JOB_GET_JOBS(ranks, nr_ranks, nr_jobs_per_rank, jobs, &sync, policy == DPU_SYNCHRONOUS, status);

    struct dpu_thread_job *job;
    DPU_THREAD_JOB_SET_JOBS(ranks, rank, nr_ranks, jobs, job, &sync, policy == DPU_SYNCHRONOUS, {
        if (dpu_set.kind == DPU_SET_RANKS) {
            job->type = DPU_THREAD_JOB_LAUNCH_RANK;
        } else {
            job->type = DPU_THREAD_JOB_LAUNCH_DPU;
            job->dpu = dpu_set.dpu;
        }
    });

    status = dpu_thread_job_do_jobs(ranks, nr_ranks, nr_jobs_per_rank, jobs, policy == DPU_SYNCHRONOUS, &sync);

    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_status(struct dpu_set_t dpu_set, bool *done, bool *fault)
{
    LOG_FN(DEBUG, "");

    dpu_sync(dpu_set);

    switch (dpu_set.kind) {
        case DPU_SET_RANKS:
            *done = true;
            *fault = false;

            for (uint32_t each_rank = 0; each_rank < dpu_set.list.nr_ranks; ++each_rank) {
                dpu_error_t status;
                bool rank_done;
                bool rank_fault;

                if ((status = dpu_status_rank(dpu_set.list.ranks[each_rank], &rank_done, &rank_fault)) != DPU_OK) {
                    return status;
                }

                *done = *done && rank_done;
                *fault = *fault || rank_fault;
            }

            return DPU_OK;
        case DPU_SET_DPU:
            return dpu_status_dpu(dpu_set.dpu, done, fault);
        default:
            return DPU_ERR_INTERNAL;
    }
}
