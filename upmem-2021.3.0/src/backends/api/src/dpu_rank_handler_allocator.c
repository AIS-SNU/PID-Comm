/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include <dpu_management.h>
#include <dpu_description.h>
#include <dpu_attributes.h>
#include <dpu_rank_id_allocator.h>
#include <dpu_package.h>
#include <dpu_mask.h>
#include <dpu_profile.h>
#include <dpu_macro_utils.h>
#include <dpu_target_macros.h>

#include <ufi/ufi_config.h>
#include <ufi/ufi_debug.h>
#include <ufi/ufi_memory.h>
#include <ufi/ufi_runner.h>

#include <dpu_rank.h>
#include <dpu_rank_handler.h>

static bool
initialize_static_interface_for_target(dpu_type_t target_type, bool verbose);
static void
dpu_rank_handler_get(dpu_rank_handler_context_t handler_context);
static void
dpu_rank_handler_put(dpu_rank_handler_context_t handler_context);
static void
dpu_rank_handler_free(dpu_rank_handler_context_t handler_context);

static bool
find_library(dpu_type_t target_type, bool verbose);
static bool
find_library_symbol(dpu_type_t target_type, const char *symbol_name, void **symbol_handle, bool verbose);
static bool
find_library_from_string(dpu_type_t target_type, const char *lib_path, bool verbose);

static bool
set_generic_description_for_enabled_dpus(struct dpu_rank_t *rank, dpu_description_t description, dpu_properties_t properties);

static struct _dpu_rank_handler_context_t handler_context[NB_OF_DPU_TYPES];

#define STRING_MAX_SIZE PATH_MAX
static const char *lib_id[NB_OF_DPU_TYPES] = {
    "fsim",
    "casim",
    "modelsim-client",
    "hw",
    "backupspi",
    "scenario",
};

static const char *prefix_lib_id[NB_OF_DPU_TYPES] = {
    "fsim",
    "casim",
    "modelsim",
    "hw",
    "spi",
    "scenario",
};

#define SHARED_LIB_PATH_SUFFIX "/usr/lib"
#define SHARED_LIB64_PATH_SUFFIX "/usr/lib64"

#ifdef _WIN32
#define SHARED_LIB_NAME_PREFIX ""
#define SHARED_LIB_NAME_SUFFIX "dll"
#elif __APPLE__
#define SHARED_LIB_NAME_PREFIX "lib"
#define SHARED_LIB_NAME_SUFFIX "dylib"
#elif __linux__
#define SHARED_LIB_NAME_PREFIX "lib"
#define SHARED_LIB_NAME_SUFFIX "so"
#endif

#ifndef SHARED_LIB_NAME_PREFIX
#error "No shared library prefix defined for the current OS"
#endif

#ifndef SHARED_LIB_NAME_SUFFIX
#error "No shared library suffix defined for the current OS"
#endif

#define BUILD_LIB_NAME_IN(string, path, backend)                                                                                 \
    snprintf(string, STRING_MAX_SIZE, "%s/" SHARED_LIB_NAME_PREFIX "dpu%s." SHARED_LIB_NAME_SUFFIX, path, lib_id[backend])

#define BUILD_LIB_NAME_WITH_VERSION_WITHOUT_SHARED_LIB_PATH_SUFFIX_IN(string, path, backend)                                     \
    snprintf(string,                                                                                                             \
        STRING_MAX_SIZE,                                                                                                         \
        "%s/" SHARED_LIB_NAME_PREFIX "dpu%s-" _STR(DPU_TOOLS_VERSION) "." SHARED_LIB_NAME_SUFFIX,                                \
        path,                                                                                                                    \
        lib_id[backend])

#define BUILD_LIB_NAME_WITHOUT_SHARED_LIB_PATH_SUFFIX_IN(string, path, backend)                                                  \
    snprintf(string, STRING_MAX_SIZE, "%s/" SHARED_LIB_NAME_PREFIX "dpu%s." SHARED_LIB_NAME_SUFFIX, path, lib_id[backend])

#define BUILD_SYMBOL_NAME_IN(string, backend, symbol) sprintf(string, "%s_%s", prefix_lib_id[backend], symbol)

/** To ensure that there is no concurrent allocation/release of DPU handlers. */
static pthread_rwlock_t static_handler_lock = PTHREAD_RWLOCK_INITIALIZER;

static inline void
exclusively()
{
    pthread_rwlock_wrlock(&static_handler_lock);
}

static inline void
exclusively_end()
{
    pthread_rwlock_unlock(&static_handler_lock);
}

static void
exclusively_reader()
{
    pthread_rwlock_rdlock(&static_handler_lock);
}

static void
exclusively_end_reader()
{
    pthread_rwlock_unlock(&static_handler_lock);
}

__API_SYMBOL__ bool
dpu_rank_handler_instantiate(dpu_type_t type, dpu_rank_handler_context_t *ret_handler_context, bool verbose)
{
    exclusively();
    if (handler_context[type].handler == NULL) {
        if (!initialize_static_interface_for_target(type, verbose)) {
            exclusively_end();
            handler_context[type].handler = NULL;
            return false;
        }

        handler_context[type].handler_refcount = 0;
    }
    exclusively_end();

    dpu_rank_handler_get(&handler_context[type]);

    *ret_handler_context = &handler_context[type];

    return true;
}

__API_SYMBOL__ void
dpu_rank_handler_release(dpu_rank_handler_context_t handler_context)
{
    dpu_rank_handler_put(handler_context);
}

static struct dpu_rank_t **dpu_rank_handler_dpu_rank_list;
static struct dpu_rank_t **dpu_rank_handler_next_dpu_rank;
static unsigned int dpu_rank_handler_dpu_rank_list_size;

void __attribute__((constructor)) __setup_dpu_rank_handler_dpu_rank_list()
{
    dpu_rank_handler_dpu_rank_list_size = 1;
    dpu_rank_handler_dpu_rank_list = calloc(dpu_rank_handler_dpu_rank_list_size, sizeof(*dpu_rank_handler_dpu_rank_list));
    dpu_rank_handler_next_dpu_rank = dpu_rank_handler_dpu_rank_list;
}

void __attribute__((destructor)) __cleanup_dpu_rank_handler_dpu_rank_list()
{
    exclusively();
    dpu_rank_handler_dpu_rank_list_size = 0;
    dpu_rank_handler_next_dpu_rank = NULL;
    free(dpu_rank_handler_dpu_rank_list);
    dpu_rank_handler_dpu_rank_list = NULL;
    exclusively_end();
}

void
dpu_rank_handler_start_iteration(struct dpu_rank_t ***ranks, unsigned int *nb_ranks)
{
    exclusively_reader();
    *nb_ranks = dpu_rank_handler_dpu_rank_list_size;
    *ranks = dpu_rank_handler_dpu_rank_list;
}

void
dpu_rank_handler_end_iteration()
{
    exclusively_end_reader();
}

static void
dpu_rank_list_update_next()
{
    for (unsigned int each_rank = 0; each_rank < dpu_rank_handler_dpu_rank_list_size; each_rank++) {
        struct dpu_rank_t **rank = &dpu_rank_handler_dpu_rank_list[each_rank];
        if (*rank == NULL) {
            dpu_rank_handler_next_dpu_rank = rank;
            return;
        }
    }
    unsigned int dpu_rank_handler_next_dpu_rank_id = dpu_rank_handler_dpu_rank_list_size;
    dpu_rank_handler_dpu_rank_list_size = dpu_rank_handler_dpu_rank_list_size * 2 + 1;
    dpu_rank_handler_dpu_rank_list
        = realloc(dpu_rank_handler_dpu_rank_list, dpu_rank_handler_dpu_rank_list_size * sizeof(struct dpu_rank_t **));

    memset(&dpu_rank_handler_dpu_rank_list[dpu_rank_handler_next_dpu_rank_id],
        0,
        (dpu_rank_handler_dpu_rank_list_size - dpu_rank_handler_next_dpu_rank_id) * sizeof(struct dpu_rank_t **));

    dpu_rank_handler_next_dpu_rank = &dpu_rank_handler_dpu_rank_list[dpu_rank_handler_next_dpu_rank_id];
}

static void
dpu_rank_list_add(struct dpu_rank_t *rank)
{
    exclusively();
    *dpu_rank_handler_next_dpu_rank = rank;
    dpu_rank_list_update_next();
    exclusively_end();
}

static void
dpu_rank_list_remove(struct dpu_rank_t *rank)
{
    exclusively();
    for (unsigned int each_rank = 0; each_rank < dpu_rank_handler_dpu_rank_list_size; each_rank++) {
        if (dpu_rank_handler_dpu_rank_list[each_rank] == rank) {
            dpu_rank_handler_dpu_rank_list[each_rank] = NULL;
            break;
        }
    }
    exclusively_end();
}

__API_SYMBOL__ bool
dpu_rank_handler_get_rank(struct dpu_rank_t *rank, dpu_rank_handler_context_t handler_context, dpu_properties_t properties)
{
    dpu_rank_status_e status;
    struct _dpu_description_t *description;

    if (!dpu_get_next_rank_id(&rank->rank_handler_allocator_id)) {
        printf("Err1\n");
        return false;
    }

    if ((description = malloc(sizeof(*description))) == NULL) {
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err2\n");
        return false;
    }

    status = handler_context->handler->fill_description_from_profile(properties, description);
    if (status != DPU_RANK_SUCCESS) {
        free(description);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err3\n");
        return false;
    }

    dpu_acquire_description(description);
    description->type = rank->type;

    if (!set_generic_description_for_enabled_dpus(rank, description, properties)) {
        dpu_free_description(description);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err4\n");
        return false;
    }

    if (!fetch_boolean_property(
            properties, DPU_PROFILE_PROPERTY_DISABLE_SAFE_CHECKS, &description->configuration.disable_api_safe_checks, false)) {
        dpu_free_description(description);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err5\n");
        return false;
    }

    uint8_t nr_cis = description->hw.topology.nr_of_control_interfaces;
    uint8_t nr_dpus_per_ci = description->hw.topology.nr_of_dpus_per_control_interface;
    uint32_t nr_dpus = nr_cis * nr_dpus_per_ci;
    
    if ((rank->dpus = calloc(nr_dpus, sizeof(*(rank->dpus)))) == NULL) 
    {
        dpu_free_description(description);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err6\n");
        return false;
    }

    /* Some backends can override the rank_id.
     * Set the default value, ask the backends to allocate and finally add the rank id header to avoid having two backends with
     * the same rank_id.
     */
    rank->rank_id = rank->rank_handler_allocator_id | (rank->type << DPU_TARGET_SHIFT);
    status = handler_context->handler->allocate(rank, description);

    if (status != DPU_RANK_SUCCESS) {
        free(rank->dpus);
        dpu_free_description(description);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        printf("Err7 %d\n", status);
        return false;
    }

    bool all_disabled = true;
    dpu_bitfield_t mux_init_state = rank->description->configuration.api_must_switch_mram_mux ? 0 : (1 << nr_dpus_per_ci) - 1;
    rank->nr_dpus_enabled = 0;

    for (dpu_slice_id_t each_slice = 0; each_slice < nr_cis; ++each_slice) 
    {
        dpu_selected_mask_t enabled_dpus = rank->runtime.control_interface.slice_info[each_slice].enabled_dpus;
        all_disabled = all_disabled && (enabled_dpus == dpu_mask_empty());
        rank->nr_dpus_enabled += __builtin_popcount(enabled_dpus);

        for (dpu_member_id_t each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) 
        {
            uint32_t dpu_index = (nr_dpus_per_ci * each_slice) + each_dpu;
            struct dpu_t *dpu = rank->dpus + dpu_index;
            dpu->rank = rank;
            dpu->slice_id = each_slice;
            dpu->dpu_id = each_dpu;
            dpu->enabled = dpu_mask_is_selected(enabled_dpus, each_dpu);
            dpu->debug_context = NULL;
        }

        /* Reset the state of MRAM mux */
        rank->runtime.control_interface.slice_info[each_slice].host_mux_mram_state = mux_init_state;
    }

    if (all_disabled) 
    {
        handler_context->handler->free(rank);
        dpu_release_rank_id(rank->rank_handler_allocator_id);
        dpu_free_description(rank->description);
        return false;
    }

    clock_gettime(CLOCK_MONOTONIC, &rank->temperature_sample_time);

    dpu_rank_list_add(rank);

    return status == DPU_RANK_SUCCESS;
}

__API_SYMBOL__ void
dpu_rank_handler_free_rank(struct dpu_rank_t *rank, dpu_rank_handler_context_t handler_context)
{
    dpu_lock_rank(rank);

    dpu_rank_list_remove(rank);

    handler_context->handler->free(rank);
    dpu_release_rank_id(rank->rank_handler_allocator_id);
    dpu_free_description(rank->description);
    dpu_rank_handler_put(handler_context);

    dpu_unlock_rank(rank);
}

#define SET_DEFAULT_IF_NULL(handler, feature, default)                                                                           \
    do {                                                                                                                         \
        if ((handler)->features.feature == NULL) {                                                                               \
            (handler)->features.feature = (default);                                                                             \
        }                                                                                                                        \
    } while (0)

static void
initialize_handler_features(struct dpu_rank_handler *handler)
{
#define FEATURE(feature, ...) SET_DEFAULT_IF_NULL(handler, feature, ci_##feature);
#include <rank_features.def>
#undef FEATURE
}

static bool
initialize_static_interface_for_target(dpu_type_t target_type, bool verbose)
{
    if (!find_library(target_type, verbose))
        return false;

    if (!find_library_symbol(target_type, "dpu_rank_handler", (void *)&handler_context[target_type].handler, verbose))
        goto close_library;

    initialize_handler_features(handler_context[target_type].handler);

    return true;

close_library:
    dlclose(handler_context[target_type].library);

    return false;
}

static void
dpu_rank_handler_get(dpu_rank_handler_context_t handler_context)
{
    __sync_add_and_fetch(&handler_context->handler_refcount, 1);
}

static void
dpu_rank_handler_put(dpu_rank_handler_context_t handler_context)
{
    if (__sync_sub_and_fetch(&handler_context->handler_refcount, 1) == 0)
        dpu_rank_handler_free(handler_context);
}

static void
dpu_rank_handler_free(dpu_rank_handler_context_t handler_context)
{
    handler_context->handler = NULL;
    dlclose(handler_context->library);
}

static bool
find_library(dpu_type_t target_type, bool verbose)
{
    static const char _empty_path[] = "/";
    char string[STRING_MAX_SIZE];
    char *path;
    if ((path = getenv("UPMEM_RUNTIME_LIBRARY_PATH")) != NULL) {
        // Useful to perform tests during maven compilation.
        // Try to use library without version in UPMEM_RUNTIME_LIBRARY_PATH directory.
        BUILD_LIB_NAME_WITHOUT_SHARED_LIB_PATH_SUFFIX_IN(string, path, target_type);
        if (find_library_from_string(target_type, string, false) == true)
            return true;
        // Use library with version in UPMEM_RUNTIME_LIBRARY_PATH directory.
        BUILD_LIB_NAME_WITH_VERSION_WITHOUT_SHARED_LIB_PATH_SUFFIX_IN(string, path, target_type);
        return find_library_from_string(target_type, string, verbose);
    } else {
        if ((path = fetch_package_library_directory()) == NULL) {
            path = (char *)_empty_path;
        }
        BUILD_LIB_NAME_IN(string, path, target_type);
        if (find_library_from_string(target_type, string, false)) {
            free(path);
            return true;
        }
        BUILD_LIB_NAME_IN(string, path, target_type);
        free(path);
        return find_library_from_string(target_type, string, verbose);
    }
}

static bool
find_library_symbol(dpu_type_t target_type, const char *symbol_name, void **symbol_handle, bool verbose)
{
    char string[STRING_MAX_SIZE];
    char *dl_error;

    BUILD_SYMBOL_NAME_IN(string, target_type, symbol_name);
    *symbol_handle = dlsym(handler_context[target_type].library, string);
    if ((dl_error = dlerror()) != NULL) {
        if (verbose) {
            fprintf(stderr, "cannot find symbol %s: %s\n", string, dl_error);
        }
        return false;
    }

    return true;
}

static bool
find_library_from_string(dpu_type_t target_type, const char *lib_path, bool verbose)
{
    char *dl_error;
    dlerror();
    if ((handler_context[target_type].library = dlopen(lib_path, RTLD_LAZY)) == NULL) {
        if (verbose) {
            fprintf(stderr, "cannot open library %s", lib_path);
            if ((dl_error = dlerror()) != NULL) {
                fprintf(stderr, ": %s", dl_error);
            }
            fprintf(stderr, "\n");
        }
        return false;
    }
    return true;
}

static bool
set_generic_description_for_enabled_dpus(struct dpu_rank_t *rank, dpu_description_t description, dpu_properties_t properties)
{
    uint64_t disabled_mask;
    if (!fetch_long_property(properties, DPU_PROFILE_PROPERTY_DISABLED_MASK, &disabled_mask, 0)) {
        return false;
    }

    for (uint8_t each_ci = 0; each_ci < description->hw.topology.nr_of_control_interfaces; ++each_ci) {
        dpu_selected_mask_t disabled_mask_for_ci = (dpu_selected_mask_t)((disabled_mask >> (8 * each_ci)) & 0xFFl);

        rank->runtime.control_interface.slice_info[each_ci].all_dpus_are_enabled = disabled_mask_for_ci == dpu_mask_empty();
        rank->runtime.control_interface.slice_info[each_ci].enabled_dpus
            = dpu_mask_difference(dpu_mask_all(description->hw.topology.nr_of_dpus_per_control_interface), disabled_mask_for_ci);
    }

    return true;
}
