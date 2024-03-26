/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <dpu_elf.h>

typedef char rte_symbol[64];
typedef struct _runtime_symbol_info {
    rte_symbol name;
    uint32_t default_value;
    size_t container;
} runtime_symbol_info_t;

#define __UNDEF_SYMBOL__ ((uint32_t)(-1))
static runtime_symbol_info_t rte_symbol_info[]
    = { { .name = "__sys_heap_pointer_reset",
            .default_value = __UNDEF_SYMBOL__,
            .container = offsetof(struct _dpu_elf_runtime_info, sys_heap_pointer_reset) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__sys_heap_pointer",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, sys_heap_pointer) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__sys_wq_table",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, sys_wq_table) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__sys_thread_stack_table_ptr",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, sys_stack_table) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__stdout_buffer",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, printf_buffer) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__stdout_buffer_state",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, printf_state) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__open_print_sequence",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, open_print_sequence) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__close_print_sequence",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, close_print_sequence) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "__sys_end",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, sys_end) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "" } };

static runtime_symbol_info_t profiling_symbol_info[]
    = { { .name = "mcount",
            .default_value = 0,
            .container = offsetof(struct _dpu_elf_runtime_info, mcount) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "ret_mcount",
              .default_value = 0,
              .container = offsetof(struct _dpu_elf_runtime_info, ret_mcount) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "thread_profiling",
              .default_value = 0,
              .container = offsetof(struct _dpu_elf_runtime_info, thread_profiling) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "perfcounter_end_value",
              .default_value = 0,
              .container = offsetof(struct _dpu_elf_runtime_info, perfcounter_end_value) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "NR_TASKLETS",
              .default_value = __UNDEF_SYMBOL__,
              .container = offsetof(struct _dpu_elf_runtime_info, nr_threads) / sizeof(dpu_elf_runtime_info_item_t) },
          { .name = "" } };

static void
reset_runtime_info_with(runtime_symbol_info_t *info, dpu_elf_runtime_info_t *runtime_info)
{
    dpu_elf_runtime_info_item_t *items = (dpu_elf_runtime_info_item_t *)runtime_info;
    unsigned int each_symbol;
    for (each_symbol = 0; info[each_symbol].name[0] != '\0'; each_symbol++) {
        dpu_elf_runtime_info_item_t *item = &items[info[each_symbol].container];
        item->value = info[each_symbol].default_value;
        item->size = 0;
        item->exists = false;
    }
}

void
reset_runtime_info(dpu_elf_runtime_info_t *runtime_info)
{
    reset_runtime_info_with(rte_symbol_info, runtime_info);
    reset_runtime_info_with(profiling_symbol_info, runtime_info);
    runtime_info->mutex_info.nr_symbols = 0;
    runtime_info->mutex_info.map = NULL;
    runtime_info->semaphore_info.nr_symbols = 0;
    runtime_info->semaphore_info.map = NULL;
    runtime_info->barrier_info.nr_symbols = 0;
    runtime_info->barrier_info.map = NULL;
}

static bool
get_container_for_symbol(const char *symbol, runtime_symbol_info_t *info, size_t *container)
{
    unsigned int each_symbol;
    for (each_symbol = 0; info[each_symbol].name[0] != '\0'; each_symbol++) {
        if (strcmp(symbol, info[each_symbol].name) == 0) {
            *container = info[each_symbol].container;
            return true;
        }
    }
    return false;
}

#define IRAM_MASK (0x80000000u)
#define MRAM_MASK (0x08000000u)

#define IRAM_ALIGN (3u)

static uint32_t
fixup_address(uint32_t address)
{
    if ((address & IRAM_MASK) == IRAM_MASK) {
        return (address & ~IRAM_MASK) >> IRAM_ALIGN;
    } else if ((address & MRAM_MASK) == MRAM_MASK) {
        return address & ~MRAM_MASK;
    } else {
        return address;
    }
}

static uint32_t
fixup_size(uint32_t address, uint32_t size)
{
    if ((address & IRAM_MASK) == IRAM_MASK) {
        return size >> IRAM_ALIGN;
    } else {
        return size;
    }
}

void
register_runtime_info_if_needed_with(const char *symbol, uint32_t value, uint32_t size, dpu_elf_runtime_info_t *runtime_info)
{
    size_t container;
    dpu_elf_runtime_info_item_t *items = (dpu_elf_runtime_info_item_t *)runtime_info;
    if (get_container_for_symbol(symbol, profiling_symbol_info, &container)) {
        dpu_elf_runtime_info_item_t *item = &items[container];
        item->value = fixup_address(value);
        item->size = fixup_size(value, size);
        item->exists = true;
    } else if (strncmp(symbol, "__", 2) == 0) {
        if (get_container_for_symbol(symbol, rte_symbol_info, &container)) {
            dpu_elf_runtime_info_item_t *item = &items[container];
            item->value = fixup_address(value);
            item->size = fixup_size(value, size);
            item->exists = true;
        } else if (strncmp(symbol + 2, "atomic_bit_mutex_", strlen("atomic_bit_mutex_")) == 0) {
            dpu_elf_symbols_t *map = &runtime_info->mutex_info;

            map->nr_symbols++;
            map->map = (dpu_elf_symbol_t *)realloc(map->map, map->nr_symbols * sizeof(dpu_elf_symbol_t));
            map->map[map->nr_symbols - 1].name = (char *)(symbol + strlen("__atomic_bit_mutex_"));
            map->map[map->nr_symbols - 1].size = size;
            map->map[map->nr_symbols - 1].value = value;
        } else if (strncmp(symbol + 2, "semaphore_", strlen("semaphore_")) == 0) {
            dpu_elf_symbols_t *map = &runtime_info->semaphore_info;

            map->nr_symbols++;
            map->map = (dpu_elf_symbol_t *)realloc(map->map, map->nr_symbols * sizeof(dpu_elf_symbol_t));
            map->map[map->nr_symbols - 1].name = (char *)(symbol + strlen("__semaphore_"));
            map->map[map->nr_symbols - 1].size = size;
            map->map[map->nr_symbols - 1].value = value;
        } else if (strncmp(symbol + 2, "barrier_", strlen("barrier_")) == 0) {
            dpu_elf_symbols_t *map = &runtime_info->barrier_info;

            map->nr_symbols++;
            map->map = (dpu_elf_symbol_t *)realloc(map->map, map->nr_symbols * sizeof(dpu_elf_symbol_t));
            map->map[map->nr_symbols - 1].name = (char *)(symbol + strlen("__barrier_"));
            map->map[map->nr_symbols - 1].size = size;
            map->map[map->nr_symbols - 1].value = value;
        }
    }
}
