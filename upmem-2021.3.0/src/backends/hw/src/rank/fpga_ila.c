/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <dpu_rank_ioctl.h>

#include "dpu_fpga_ila.h"
#include "hw_dpu_sysfs.h"

#define ILA_FIFO_DEPTH (1 << 16)
#define ILA_OUTPUT_ENTRY_LENGTH 79

struct ila_dump_information_t {
    uint32_t buffer_size;
    uint8_t *buffer;
};

bool
reset_ila(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_reset_ila(rank_fs, 1))
        goto end;

    result = true;
end:
    return result;
}

bool
activate_filter_ila(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_activate_filtering_ila(rank_fs, 1))
        goto end;

    result = true;
end:
    return result;
}

bool
deactivate_filter_ila(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_activate_filtering_ila(rank_fs, 0))
        goto end;

    result = true;
end:
    return result;
}

bool
activate_ila(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_activate_ila(rank_fs, 1))
        goto end;

    result = true;
end:
    return result;
}

bool
deactivate_ila(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_activate_ila(rank_fs, 0))
        goto end;

    result = true;
end:
    return result;
}

bool
set_mram_bypass_to(struct dpu_rank_fs *rank_fs, bool mram_bypass_is_enabled)
{
    bool result = false;
    uint8_t temp = mram_bypass_is_enabled ? 1 : 0;

    if (dpu_sysfs_set_activate_mram_bypass(rank_fs, temp))
        goto end;

    result = true;
end:
    return result;
}

bool
disable_refresh_emulation(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_mram_refresh_emulation_period(rank_fs, 0))
        goto end;

    result = true;
end:
    return result;
}

bool
enable_refresh_emulation(struct dpu_rank_fs *rank_fs, uint32_t threshold)
{
    bool result = false;

    if (dpu_sysfs_set_mram_refresh_emulation_period(rank_fs, threshold))
        goto end;

    result = true;
end:
    return result;
}

bool
dump_ila_report(struct dpu_rank_fs *rank_fs, const char *report_file_name)
{
    bool result = false;
    int32_t nb_of_entries;
    FILE *csv;
    int fd;
    struct ila_dump_information_t ila_dump_information;
    char iladump_path[256];
    uint8_t rank_id;

    csv = fopen(report_file_name, "w");
    if (csv == NULL)
        return false;

    ila_dump_information.buffer_size = ILA_FIFO_DEPTH * ILA_OUTPUT_ENTRY_LENGTH * sizeof(*(ila_dump_information.buffer));
    if ((ila_dump_information.buffer = malloc(ila_dump_information.buffer_size)) == NULL) {
        goto end;
    }

    rank_id = dpu_sysfs_get_rank_id(rank_fs);

    sprintf(iladump_path, "/sys/kernel/debug/dpu_rank%d/iladump", rank_id);

    fd = open(iladump_path, O_RDONLY);
    if (fd < 0) {
        printf("iladump debugfs entry is not available");
        goto free_ila_buffer;
    }

    if ((nb_of_entries = read(fd, ila_dump_information.buffer, ila_dump_information.buffer_size)) < 0) {
        goto free_fd;
    }

    fwrite(ila_dump_information.buffer, 1, nb_of_entries * ILA_OUTPUT_ENTRY_LENGTH * sizeof(*(ila_dump_information.buffer)), csv);

    result = true;

free_fd:
    close(fd);
free_ila_buffer:
    free(ila_dump_information.buffer);
end:
    fflush(csv);
    fclose(csv);
    return result;
}

bool
inject_faults(struct dpu_rank_fs *rank_fs)
{
    bool result = false;

    if (dpu_sysfs_set_inject_faults(rank_fs, 1))
        goto end;

    result = true;
end:
    return result;
}
