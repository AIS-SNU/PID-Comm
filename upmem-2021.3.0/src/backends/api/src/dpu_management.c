/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <inttypes.h>

#include <dpu_attributes.h>
#include <dpu_program.h>
#include <dpu_types.h>
#include <dpu_management.h>

#include <dpu_rank.h>
#include <dpu_profile.h>
#include <dpu_api_log.h>
#include <dpu_properties_loader.h>
#include <dpu_internals.h>

/* High-level API default value for threads/asynchronism management */
#define NR_JOBS_PER_RANK_DEFAULT (16)
#define NR_THREADS_PER_RANK_DEFAULT (1)

__API_SYMBOL__ struct dpu_t *
dpu_get(struct dpu_rank_t *rank, dpu_slice_id_t slice_id, dpu_member_id_t dpu_id)
{
    uint8_t nr_slices = rank->description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus = rank->description->hw.topology.nr_of_dpus_per_control_interface;

    if ((slice_id >= nr_slices) || (dpu_id >= nr_dpus)) {
        return NULL;
    }

    return rank->dpus + DPU_INDEX(rank, slice_id, dpu_id);
}

__API_SYMBOL__ struct dpu_rank_t *
dpu_get_rank(struct dpu_t *dpu)
{
    return dpu->rank;
}

__API_SYMBOL__ dpu_slice_id_t
dpu_get_slice_id(struct dpu_t *dpu)
{
    return dpu->slice_id;
}

__API_SYMBOL__ dpu_member_id_t
dpu_get_member_id(struct dpu_t *dpu)
{
    return dpu->dpu_id;
}

__API_SYMBOL__ bool
dpu_is_enabled(struct dpu_t *dpu)
{
    return dpu->enabled;
}

__API_SYMBOL__ dpu_description_t
dpu_get_description(struct dpu_rank_t *rank)
{
    return rank->description;
}

__API_SYMBOL__ dpu_id_t
dpu_get_id(struct dpu_t *dpu)
{
    return (dpu_id_t)((dpu->rank->rank_id << 16) | ((dpu->slice_id & 0xff) << 8) | (dpu->dpu_id & 0xff));
}

__API_SYMBOL__ dpu_id_t
dpu_get_rank_id(struct dpu_rank_t *rank)
{
    return (dpu_id_t)rank->rank_id;
}

__API_SYMBOL__ int
dpu_get_rank_numa_node(struct dpu_rank_t *rank)
{
    return rank->numa_node;
}

__API_SYMBOL__ void
dpu_lock_rank(struct dpu_rank_t *rank)
{
    pthread_mutex_lock(&rank->mutex);
}

__API_SYMBOL__ int
dpu_trylock_rank(struct dpu_rank_t *rank)
{
    return pthread_mutex_trylock(&rank->mutex);
}

__API_SYMBOL__ void
dpu_unlock_rank(struct dpu_rank_t *rank)
{
    pthread_mutex_unlock(&rank->mutex);
}

static dpu_type_t
determine_default_backend_type()
{
    dpu_type_t default_backend = HW;
    dpu_type_t backup_backend = FUNCTIONAL_SIMULATOR;
    dpu_rank_handler_context_t handler_context;
    uint32_t nr_hw_ranks;

    if (!dpu_rank_handler_instantiate(default_backend, &handler_context, false)) {
        default_backend = backup_backend;
        goto end;
    }

    if ((handler_context->handler->get_nr_dpu_ranks != NULL)
        && (handler_context->handler->get_nr_dpu_ranks(&nr_hw_ranks) == DPU_RANK_SUCCESS) && (nr_hw_ranks == 0)) {
        LOG_FN(WARNING,
            "Fallback to SIMULATOR backend as no hardware device was found and no explicit backend is provided "
            "(add 'backend=simulator' to the profile argument when allocating DPUs to suppress this message)");
        default_backend = backup_backend;
    }

    dpu_rank_handler_release(handler_context);

end:
    return default_backend;
}

static dpu_error_t
determine_backend_type_from_complete_profile(dpu_properties_t properties, dpu_type_t *backend)
{
    char *backend_string;

    if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_BACKEND, &backend_string, NULL)) {
        return DPU_ERR_SYSTEM;
    }

    dpu_error_t status = DPU_OK;
    if (backend_string == NULL) {
        *backend = determine_default_backend_type();
    } else if (!dpu_type_from_profile_string(backend_string, backend)) {
        status = DPU_ERR_INVALID_PROFILE;
    }

    free(backend_string);
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_get_profile_description(const char *profile, dpu_description_t *description)
{
    dpu_description_t dpu_description;
    dpu_error_t status;
    dpu_rank_handler_context_t handler_context;
    dpu_properties_t properties;

    if ((dpu_description = malloc(sizeof(*dpu_description))) == NULL) {
        status = DPU_ERR_SYSTEM;
        goto end;
    }

    properties = dpu_properties_load_from_profile(profile);
    if (properties == DPU_PROPERTIES_INVALID) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_description;
    }

    dpu_type_t type;

    if ((status = determine_backend_type_from_complete_profile(properties, &type)) != DPU_OK) {
        goto delete_properties;
    }

    if (!dpu_rank_handler_instantiate(type, &handler_context, false)) {
        status = DPU_ERR_ALLOCATION;
        goto delete_properties;
    }

    if (handler_context->handler->fill_description_from_profile(properties, dpu_description) != DPU_RANK_SUCCESS) {
        status = DPU_ERR_INVALID_PROFILE;
        goto delete_properties;
    }

    dpu_acquire_description(dpu_description);
    dpu_description->type = type;

    *description = dpu_description;

    dpu_properties_delete(properties);

    return DPU_OK;

delete_properties:
    dpu_properties_delete(properties);
free_description:
    free(dpu_description);
end:
    return status;
}

static dpu_error_t
dpu_get_profiling_properties(struct dpu_rank_t *dpu_rank, dpu_properties_t properties)
{
    dpu_slice_id_t slice_id;
    dpu_member_id_t dpu_id;
    char *enable_profiling, *mcount_address, *ret_mcount_address, *thread_profiling_address, *profiling_dpu_id,
        *profiling_slice_id;

    /* Profiling profile */
    if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_ENABLE_PROFILING, &enable_profiling, NULL))
        return DPU_ERR_INVALID_PROFILE;

    if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_PROFILING_DPU_ID, &profiling_dpu_id, NULL))
        return DPU_ERR_INVALID_PROFILE;

    if (profiling_dpu_id) {
        sscanf(profiling_dpu_id, "%" SCNu8, &dpu_id);
        free(profiling_dpu_id);
    } else
        dpu_id = 0;

    if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_PROFILING_SLICE_ID, &profiling_slice_id, NULL))
        return DPU_ERR_INVALID_PROFILE;

    if (profiling_slice_id) {
        sscanf(profiling_slice_id, "%" SCNu8, &slice_id);
        free(profiling_slice_id);
    } else
        slice_id = 0;

    dpu_rank->profiling_context.dpu = DPU_GET_UNSAFE(dpu_rank, slice_id, dpu_id);

    if (!enable_profiling) {
        /* If profiling is not explicitly required by user, we must nopify all mcount references if any */
        dpu_rank->profiling_context.enable_profiling = DPU_PROFILING_NOP;
    } else if (enable_profiling) {
        enum dpu_profiling_type_e profiling_type;

        if (!strcmp(enable_profiling, "statistics")) {
            profiling_type = DPU_PROFILING_STATS;
        } else if (!strcmp(enable_profiling, "nop")) {
            profiling_type = DPU_PROFILING_NOP;
        } else if (!strcmp(enable_profiling, "mcount")) {
            profiling_type = DPU_PROFILING_MCOUNT;
        } else if (!strcmp(enable_profiling, "samples")) {
            profiling_type = DPU_PROFILING_SAMPLES;
            if ((dpu_rank->profiling_context.sample_stats
                    = calloc(dpu_rank->description->hw.memories.iram_size, sizeof(*(dpu_rank->profiling_context.sample_stats))))
                == NULL) {

                return DPU_ERR_SYSTEM;
            }
        } else if (!strcmp(enable_profiling, "sections")) {
            profiling_type = DPU_PROFILING_SECTIONS;
        } else {
            LOG_RANK(WARNING, dpu_rank, "Profiling type %s is unknown", enable_profiling);
            return DPU_ERR_INTERNAL;
        }

        free(enable_profiling);

        dpu_rank->profiling_context.enable_profiling = profiling_type;

        if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_MCOUNT_ADDRESS, &mcount_address, NULL))
            return DPU_ERR_INVALID_PROFILE;

        if (mcount_address) {
            sscanf(mcount_address, "%" SCNx16, &dpu_rank->profiling_context.mcount_address);
            free(mcount_address);
        } else
            dpu_rank->profiling_context.mcount_address = 0;

        if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_RET_MCOUNT_ADDRESS, &ret_mcount_address, NULL))
            return DPU_ERR_INVALID_PROFILE;

        if (ret_mcount_address) {
            sscanf(ret_mcount_address, "%" SCNx16, &dpu_rank->profiling_context.ret_mcount_address);
            free(ret_mcount_address);
        } else
            dpu_rank->profiling_context.ret_mcount_address = 0;

        if (!fetch_string_property(properties, DPU_PROFILE_PROPERTY_THREAD_PROFILING_ADDRESS, &thread_profiling_address, NULL))
            return DPU_ERR_INVALID_PROFILE;

        if (thread_profiling_address) {
            sscanf(thread_profiling_address, "%" SCNx32, &dpu_rank->profiling_context.thread_profiling_address);
            free(thread_profiling_address);
        } else
            dpu_rank->profiling_context.thread_profiling_address = 0;
    }

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_get_rank_of_type(const char *profile, struct dpu_rank_t **rank)
{
    // printf("\tdpu_get_rank_of_type Called\n");
    LOG_FN(DEBUG, "");

    dpu_error_t status;
    struct dpu_rank_t *dpu_rank;
    dpu_properties_t properties;
    bool disable_mux_switch, disable_reset_on_alloc;
    uint64_t debug_cmds_buffer_size;

    properties = dpu_properties_load_from_profile(profile);
    if (properties == DPU_PROPERTIES_INVALID) {
        status = DPU_ERR_INVALID_PROFILE;
        goto end;
    }

    dpu_rank = calloc(1, sizeof(*dpu_rank));
    if (dpu_rank == NULL) {
        status = DPU_ERR_SYSTEM;
        goto delete_properties;
    }

    if ((status = determine_backend_type_from_complete_profile(properties, &dpu_rank->type)) != DPU_OK) {
        goto free_link;
    }

    if (!dpu_rank_handler_instantiate(dpu_rank->type, &dpu_rank->handler_context, true)) {
        status = DPU_ERR_ALLOCATION;
        goto free_link;
    }

    if (!dpu_rank_handler_get_rank(dpu_rank, dpu_rank->handler_context, properties)) {
        status = DPU_ERR_ALLOCATION;
        goto release_handler;
    }

    /* Profiling profile */
    status = dpu_get_profiling_properties(dpu_rank, properties);
    if (status != DPU_OK)
        goto free_dpus;

    /* Mux MRAM disable */
    if (!fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_DISABLE_MUX_SWITCH, &disable_mux_switch, false)) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }

    if (disable_mux_switch) {
        dpu_rank->description->configuration.init_mram_mux = false;
        dpu_rank->description->configuration.api_must_switch_mram_mux = false;
    }

    /* Reset on alloc disable */
    if (!fetch_boolean_property(properties, DPU_PROFILE_PROPERTY_DISABLE_RESET_ON_ALLOC, &disable_reset_on_alloc, false)) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }
    dpu_rank->description->configuration.disable_reset_on_alloc = disable_reset_on_alloc;

    if (!fetch_integer_property(
            properties, DPU_PROFILE_PROPERTY_NR_JOBS_PER_RANK, &dpu_rank->api.nr_jobs, NR_JOBS_PER_RANK_DEFAULT)) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }

    if (!fetch_integer_property(
            properties, DPU_PROFILE_PROPERTY_NR_THREADS_PER_RANK, &dpu_rank->api.nr_threads, NR_THREADS_PER_RANK_DEFAULT)) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }

    if (dpu_rank->api.nr_threads > NR_THREADS_PER_RANK_MAX) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }

    /* Debug commands buffer */
#define DEBUG_CMDS_BUFFER_SIZE_DEFAULT 1000
    if (!fetch_long_property(
            properties, DPU_PROFILE_PROPERTY_CMDS_BUFFER_SIZE, &debug_cmds_buffer_size, DEBUG_CMDS_BUFFER_SIZE_DEFAULT)) {
        status = DPU_ERR_INVALID_PROFILE;
        goto free_dpus;
    }
    dpu_rank->debug.cmds_buffer.size = debug_cmds_buffer_size;
    if (LOGD_ENABLED(get_verbose_control_for("ufi"))) {
        dpu_rank->debug.cmds_buffer.cmds = calloc(debug_cmds_buffer_size, sizeof(struct dpu_command_t));
        if (!dpu_rank->debug.cmds_buffer.cmds) {
            return DPU_ERR_SYSTEM;
        }
        dpu_rank->debug.cmds_buffer.nb = 0;
        dpu_rank->debug.cmds_buffer.idx_last = 0;
        dpu_rank->debug.cmds_buffer.has_wrapped = false;
    } else
        dpu_rank->debug.cmds_buffer.cmds = NULL;

    dpu_properties_log_unused(properties, __vc());

    pthread_mutexattr_t mutex_attr;
    if (pthread_mutexattr_init(&mutex_attr) != 0) {
        status = DPU_ERR_SYSTEM;
        goto free_cmds_buffer;
    }
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&(dpu_rank->mutex), &mutex_attr) != 0) {
        pthread_mutexattr_destroy(&mutex_attr);
        status = DPU_ERR_SYSTEM;
        goto free_cmds_buffer;
    }
    pthread_mutexattr_destroy(&mutex_attr);

    *rank = dpu_rank;
    goto delete_properties;

free_cmds_buffer:
    free(dpu_rank->debug.cmds_buffer.cmds);
free_dpus:
    free(dpu_rank->dpus);
release_handler:
    dpu_rank_handler_release(dpu_rank->handler_context);
free_link:
    free(dpu_rank);
delete_properties:
    dpu_properties_delete(properties);
end:
    return status;
}

__API_SYMBOL__ dpu_error_t
dpu_free_rank(struct dpu_rank_t *rank)
{
    LOG_RANK(DEBUG, rank, "");

    dpu_lock_rank(rank);

    uint8_t nr_dpus = rank->description->hw.topology.nr_of_control_interfaces
        * rank->description->hw.topology.nr_of_dpus_per_control_interface;

    uint8_t nr_threads = rank->description->hw.dpu.nr_of_threads;

    for (int each_dpu = 0; each_dpu < nr_dpus; ++each_dpu) {
        struct dpu_t *dpu = rank->dpus + each_dpu;

        dpu_free_program(dpu->program);
        dpu_free_dpu_context(dpu->debug_context);
        free(dpu->debug_context);
    }
    free(rank->dpus);

    if (rank->profiling_context.mcount_stats) {
        for (uint8_t th_id = 0; th_id < nr_threads; ++th_id)
            if (rank->profiling_context.mcount_stats[th_id] != NULL) {
                free(rank->profiling_context.mcount_stats[th_id]);
            }

        free(rank->profiling_context.mcount_stats);
    }

    if (rank->profiling_context.sample_stats) {
        free(rank->profiling_context.sample_stats);
    }

    free(rank->debug.cmds_buffer.cmds);

    dpu_rank_handler_free_rank(rank, rank->handler_context);

    dpu_unlock_rank(rank);

    pthread_mutex_destroy(&(rank->mutex));
    free(rank);

    return DPU_OK;
}

__API_SYMBOL__ struct dpu_set_t
dpu_set_from_rank(struct dpu_rank_t **rank)
{
    struct dpu_set_t set = { .kind = DPU_SET_RANKS,
        .list = {
            .nr_ranks = 1,
            .ranks = rank,
        } };

    return set;
}

__API_SYMBOL__ struct dpu_set_t
dpu_set_from_dpu(struct dpu_t *dpu)
{
    struct dpu_set_t set = {
        .kind = DPU_SET_DPU,
        .dpu = dpu,
    };

    return set;
}

__API_SYMBOL__ struct dpu_rank_t *
dpu_rank_from_set(struct dpu_set_t set)
{
    if (set.kind != DPU_SET_RANKS || set.list.nr_ranks != 1) {
        return NULL;
    }
    return set.list.ranks[0];
}

__API_SYMBOL__ struct dpu_t *
dpu_from_set(struct dpu_set_t set)
{
    if (set.kind != DPU_SET_DPU) {
        return NULL;
    }
    return set.dpu;
}

static void
advance_to_next_dpu_in_rank(struct dpu_iterator_t *iterator)
{
    struct dpu_rank_t *rank = iterator->rank;
    uint32_t dpu_idx = iterator->next_idx;

    uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;
    uint32_t nr_dpus = nr_cis * nr_dpus_per_ci;

    for (; dpu_idx < nr_dpus; ++dpu_idx) {
        struct dpu_t *dpu = rank->dpus + dpu_idx;
        if (dpu->enabled) {
            iterator->has_next = true;
            iterator->next_idx = dpu_idx + 1;
            iterator->next = dpu;
            return;
        }
    }

    iterator->has_next = false;
}

__API_SYMBOL__ struct dpu_iterator_t
dpu_iterator_from(struct dpu_rank_t *rank)
{
    struct dpu_iterator_t iterator;
    iterator.rank = rank;
    iterator.count = 0;
    iterator.next_idx = 0;
    advance_to_next_dpu_in_rank(&iterator);
    return iterator;
}

__API_SYMBOL__ void
dpu_iterator_next(struct dpu_iterator_t *iterator)
{
    iterator->count++;
    advance_to_next_dpu_in_rank(iterator);
}
