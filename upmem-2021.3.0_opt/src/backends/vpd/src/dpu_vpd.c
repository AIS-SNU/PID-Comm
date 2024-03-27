/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <dpu_vpd.h>
#include <dpu_vpd_container.h>
#include <dpu_types.h>
#include <dpu_attributes.h>
#include <dpu_flash_partition.h>

#include <static_verbose.h>
#include <dpu_log_utils.h>

static struct verbose_control *this_vc;
static inline struct verbose_control *
__vc()
{
    if (this_vc == NULL) {
        this_vc = get_verbose_control_for("vpd");
    }
    return this_vc;
}

/* clang-format off */
#define MIN(x, y) ({                    \
        __typeof(x) _x = x;             \
        __typeof(y) _y = y;             \
        (((_x) < (_y)) ? (_x) : (_y));  \
    })
/* clang-format on */

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_get_vpd_path(const char *dev_rank_path, char *vpd_path)
{
    LOG_FN(VERBOSE, "");
    uint32_t rank_id;

    if (!sscanf(dev_rank_path, "/dev/dpu_rank%u", &rank_id))
        return DPU_VPD_ERR;

    if (sprintf(vpd_path, "/sys/class/dpu_rank/dpu_rank%u/dimm_vpd", rank_id) == -1)
        return DPU_VPD_ERR;

    return DPU_VPD_OK;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_init(const char *vpd_path, struct dpu_vpd *vpd)
{
    LOG_FN(VERBOSE, "");
    FILE *f;
    enum dpu_vpd_error ret = DPU_VPD_OK;

    if (vpd_path == NULL) {
        memset(&vpd->vpd_header, 0, sizeof(struct dpu_vpd_header));
        memcpy(&vpd->vpd_header.struct_id, VPD_STRUCT_ID, sizeof(vpd->vpd_header.struct_id));
        vpd->vpd_header.struct_ver = VPD_STRUCT_VERSION;
        vpd->vpd_header.struct_size = sizeof(struct dpu_vpd_header);
        vpd->vpd_header.repair_count = (uint16_t)VPD_UNDEFINED_REPAIR_COUNT;

        return DPU_VPD_OK;
    }

    // vpd->vpd_header.ranks[0].dpu_disabled = 0;
    // vpd->vpd_header.ranks[1].dpu_disabled = 0;
    // vpd->vpd_header.ranks[2].dpu_disabled = 0;
    // vpd->vpd_header.ranks[3].dpu_disabled = 0;

    f = fopen(vpd_path, "r");
    if (f == NULL)
        return DPU_VPD_ERR;

    if (fread(&vpd->vpd_header, sizeof(struct dpu_vpd_header), 1, f) != 1) {
        ret = DPU_VPD_ERR;
        goto end;
    }

    /* Validate the header format and compatibility */
    if (memcmp(vpd->vpd_header.struct_id, VPD_STRUCT_ID, sizeof(vpd->vpd_header.struct_id)) != 0) {
        ret = DPU_VPD_ERR_HEADER_FORMAT;
        goto end;
    }

    if (vpd->vpd_header.struct_ver != VPD_STRUCT_VERSION) {
        ret = DPU_VPD_ERR_HEADER_VERSION;
        goto end;
    }

    if (vpd->vpd_header.repair_count != VPD_UNDEFINED_REPAIR_COUNT) {
        uint16_t repair_count = 0;

        while (fread(&vpd->repair_entries[repair_count], sizeof(struct dpu_vpd_repair_entry), 1, f) == 1) {
            repair_count++;
        }
    }

end:
    fclose(f);

    return ret;
}

/* Helpers to initialize database with default values */
static enum dpu_vpd_error
dpu_vpd_init_db_e19(struct dpu_vpd_database *db)
{
#include "E19.vpd"
    return DPU_VPD_OK;
}

static enum dpu_vpd_error
dpu_vpd_init_db_p21(struct dpu_vpd_database *db)
{
#include "P21.vpd"
    return DPU_VPD_OK;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_db_init(int dual_rank, const char *db_path, struct dpu_vpd_database *db)
{
    LOG_FN(VERBOSE, "");
    FILE *f;
    int read_size;
    uint8_t buf[FLASH_SIZE_VPD_DB];
    enum dpu_vpd_error err = DPU_VPD_OK;

    vpd_init_container(db);

    if (db_path == NULL) {
        if (dual_rank == 0)
            err = dpu_vpd_init_db_e19(db);
        else if (dual_rank == 1)
            err = dpu_vpd_init_db_p21(db);
        return err;
    }

    f = fopen(db_path, "r");
    if (f == NULL)
        return DPU_VPD_ERR;

    read_size = fread(buf, 1, FLASH_SIZE_VPD_DB, f);
    err = vpd_decode_to_container(buf, read_size, db);
    fclose(f);

    return err;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_db_update(struct dpu_vpd_database *db, const char *key, const uint8_t *value, const int value_len, const int value_type)
{
    LOG_FN(VERBOSE, "");
    return vpd_set_string(db, (uint8_t *)key, value, value_len, value_type);
}

__API_SYMBOL__ void
dpu_vpd_db_destroy(struct dpu_vpd_database *db)
{
    LOG_FN(VERBOSE, "");
    vpd_destroy_container(db);
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_write(struct dpu_vpd *vpd, const char *vpd_path)
{
    LOG_FN(VERBOSE, "");
#define VPD_SIZE_CHUNK_MULTIPLE 272
    FILE *f;
    int repair;
    enum dpu_vpd_error err = DPU_VPD_OK;
    size_t size_written = 0;
    uint8_t empty_buffer[VPD_MAX_SIZE] = { 0 };

    if (access(vpd_path, F_OK))
        f = fopen(vpd_path, "w+");
    else
        f = fopen(vpd_path, "r+");

    if (f == NULL)
        return DPU_VPD_ERR;

    if (fwrite(&vpd->vpd_header, sizeof(struct dpu_vpd_header), 1, f) != 1) {
        err = DPU_VPD_ERR;
        goto end;
    }
    size_written += sizeof(struct dpu_vpd_header);

    if (vpd->vpd_header.repair_count != VPD_UNDEFINED_REPAIR_COUNT) {
        for (repair = 0; repair < vpd->vpd_header.repair_count; ++repair) {
            if (fwrite(&vpd->repair_entries[repair], sizeof(struct dpu_vpd_repair_entry), 1, f) != 1) {
                err = DPU_VPD_ERR;
                goto end;
            }
            size_written += sizeof(struct dpu_vpd_repair_entry);
        }
    }

    /*
     * Align size to write to VPD_SIZE_CHUNK_MULTIPLE but make sure that we do
     * not cross VPD_MAX_SIZE limit.
     */
    size_t size_align = MIN(VPD_MAX_SIZE - size_written, VPD_SIZE_CHUNK_MULTIPLE - size_written % VPD_SIZE_CHUNK_MULTIPLE);

    if (fwrite(empty_buffer, size_align, 1, f) != 1) {
        err = DPU_VPD_ERR;
        goto end;
    }

end:
    fclose(f);

    return err;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_db_write(const struct dpu_vpd_database *db, const char *db_path)
{
    LOG_FN(VERBOSE, "");
    FILE *f;
    uint8_t buf[FLASH_SIZE_VPD_DB];
    int generated;
    int size_written;
    enum dpu_vpd_error err;

    err = vpd_encode_container(db, FLASH_SIZE_VPD_DB, buf, &generated);
    if (err != DPU_VPD_OK)
        return err;

    f = fopen(db_path, "w+");
    if (f == NULL)
        return DPU_VPD_ERR;

    /* Make sure file size is a multiple of 2, otherwise the file cannot be flashed */
    if (generated % 2 != 0) {
        if (generated >= FLASH_SIZE_VPD_DB) {
            /* Out of space. This should never happen since vpd_encode_container succeeded */
            err = DPU_VPD_ERR;
            goto end;
        }
        buf[generated++] = 0xFF;
    }

    size_written = fwrite(buf, 1, generated, f);
    if (size_written != generated) {
        err = DPU_VPD_ERR;
        goto end;
    }

end:
    fclose(f);
    return err;
}

static enum dpu_vpd_error
__dpu_vpd_fork(const char *exec, char **args)
{
    LOG_FN(VERBOSE, "%s", exec);
    pid_t pid;
    int status;

    pid = fork();
    if (!pid) {
        close(STDOUT_FILENO);
        /* If verbose log is not enabled, redirect to /dev/null */
        if (LOGV_ENABLED(__vc()))
            dup2(fileno(this_vc->output_file), STDOUT_FILENO);
        else
            open("/dev/null", O_WRONLY);

        if (execvp(exec, args))
            exit(-1);

        exit(0);
    }

    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status))
            return DPU_VPD_ERR_FLASH;
    } else
        return DPU_VPD_ERR_FLASH;

    return DPU_VPD_OK;
}

static enum dpu_vpd_error
__dpu_vpd_commit_to_device_from_dfu(const char *vpd_path, bool is_db)
{
    LOG_FN(VERBOSE, "");
#define FLASH_VPD_FROM_DFU_TOOL_NAME "dfu-util"
#define FLASH_VPD_FROM_DFU_ARG_COUNT 9
    enum dpu_vpd_error ret = DPU_VPD_OK;
    char *args[FLASH_VPD_FROM_DFU_ARG_COUNT] = { NULL };
    unsigned int address;
    unsigned int size;

    if (is_db) {
        address = FLASH_BASE_ADDRESS + FLASH_OFF_VPD_DB;
        size = FLASH_SIZE_VPD_DB;
    } else {
        address = FLASH_BASE_ADDRESS + FLASH_OFF_VPD;
        size = FLASH_SIZE_VPD;
    }

    if (asprintf(&args[0], FLASH_VPD_FROM_DFU_TOOL_NAME) == -1 || asprintf(&args[1], "-a") == -1 || asprintf(&args[2], "0") == -1
        || asprintf(&args[3], "-s") == -1 || asprintf(&args[4], "%u:%u", address, size) == -1 || asprintf(&args[5], "-D") == -1
        || asprintf(&args[6], "%s", vpd_path) == -1) {
        ret = DPU_VPD_ERR;
        goto end;
    }

    ret = __dpu_vpd_fork(FLASH_VPD_FROM_DFU_TOOL_NAME, args);

end:
    for (int arg = 0; arg < FLASH_VPD_FROM_DFU_ARG_COUNT; ++arg)
        free(args[arg]);

    return ret;
}

static enum dpu_vpd_error
__dpu_vpd_commit_to_device_from_rank(const char *vpd_path, const char *device_name, bool is_db)
{
    LOG_FN(VERBOSE, "");
#define FLASH_VPD_FROM_RANK_TOOL_NAME "ectool"
#define FLASH_VPD_FROM_RANK_ARG_COUNT 7
    enum dpu_vpd_error ret = DPU_VPD_OK;
    char *args[FLASH_VPD_FROM_RANK_ARG_COUNT] = { NULL };
    unsigned int offset;
    unsigned int size;

    if (is_db) {
        offset = FLASH_OFF_VPD_DB;
        size = FLASH_SIZE_VPD_DB;
    } else {
        offset = FLASH_OFF_VPD;
        size = FLASH_SIZE_VPD;
    }

    if (asprintf(&args[0], FLASH_VPD_FROM_RANK_TOOL_NAME) == -1 || asprintf(&args[1], "--interface=ci") == -1
        || asprintf(&args[2], "--name=%s", device_name) == -1) {
        ret = DPU_VPD_ERR;
        goto end;
    }

    if (asprintf(&args[3], "flasherase") == -1 || asprintf(&args[4], "%u", offset) == -1
        || asprintf(&args[5], "%u", size) == -1) {
        ret = DPU_VPD_ERR;
        goto end;
    }

    ret = __dpu_vpd_fork(FLASH_VPD_FROM_RANK_TOOL_NAME, args);
    if (ret != DPU_VPD_OK)
        goto end;

    free(args[3]);
    free(args[4]);
    free(args[5]);
    if (asprintf(&args[3], "flashwrite") == -1 || asprintf(&args[4], "%u", offset) == -1
        || asprintf(&args[5], "%s", vpd_path) == -1) {
        ret = DPU_VPD_ERR;
        goto end;
    }

    ret = __dpu_vpd_fork(FLASH_VPD_FROM_RANK_TOOL_NAME, args);
    if (ret != DPU_VPD_OK)
        goto end;

end:
    for (int arg = 0; arg < FLASH_VPD_FROM_RANK_ARG_COUNT; ++arg)
        free(args[arg]);

    return ret;
}

static enum dpu_vpd_error
__dpu_vpd_commit_to_device(const char *vpd_path, const char *device_name, bool is_db)
{
    LOG_FN(VERBOSE, "");

    if (device_name)
        return __dpu_vpd_commit_to_device_from_rank(vpd_path, device_name, is_db);

    return __dpu_vpd_commit_to_device_from_dfu(vpd_path, is_db);
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_commit_to_device_from_file(const char *vpd_path, const char *device_name)
{
    LOG_FN(VERBOSE, "");
    return __dpu_vpd_commit_to_device(vpd_path, device_name, false);
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_db_commit_to_device_from_file(const char *db_path, const char *device_name)
{
    LOG_FN(VERBOSE, "");
    return __dpu_vpd_commit_to_device(db_path, device_name, true);
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_commit_to_device(struct dpu_vpd *vpd, const char *device_name)
{
    LOG_FN(VERBOSE, "");
    enum dpu_vpd_error ret;
    char tmp_path[32];
    int fd;

    /* Create a temp file to pass to flash_vpd script */
    strcpy(tmp_path, "/tmp/UPMXXXXXX");

    fd = mkstemp(tmp_path);
    if (fd == -1)
        return DPU_VPD_ERR;

    ret = dpu_vpd_write(vpd, tmp_path);
    if (ret != DPU_VPD_OK)
        goto unlink_temp;

    ret = __dpu_vpd_commit_to_device(tmp_path, device_name, false);

unlink_temp:
    unlink(tmp_path);
    close(fd);

    return ret;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_db_commit_to_device(const struct dpu_vpd_database *db, const char *device_name)
{
    LOG_FN(VERBOSE, "");
    enum dpu_vpd_error ret;
    char tmp_path[32];
    int fd;

    /* Create a temp file */
    strcpy(tmp_path, "/tmp/UPMXXXXXX");

    fd = mkstemp(tmp_path);
    if (fd == -1)
        return DPU_VPD_ERR;

    ret = dpu_vpd_db_write(db, tmp_path);
    if (ret != DPU_VPD_OK)
        goto unlink_temp;

    ret = __dpu_vpd_commit_to_device(tmp_path, device_name, true);

unlink_temp:
    unlink(tmp_path);
    close(fd);

    return ret;
}

static bool
__dpu_vpd_compare_repair_entry(struct dpu_vpd_repair_entry *repair_a, struct dpu_vpd_repair_entry *repair_b)
{
    LOG_FN(VERBOSE, "");
    return (repair_a->iram_wram == repair_b->iram_wram && repair_a->rank == repair_b->rank && repair_a->ci == repair_b->ci
        && repair_a->dpu == repair_b->dpu && repair_a->bank == repair_b->bank && repair_a->address == repair_b->address
        && repair_a->bits == repair_b->bits);
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_add_repair_entry(struct dpu_vpd *vpd, struct dpu_vpd_repair_entry *repair_entry)
{
    LOG_FN(VERBOSE, "");
    uint64_t *repair_field;
    int repair;
    enum dpu_vpd_repair_type repair_type = repair_entry->iram_wram;

    if (vpd->vpd_header.repair_count == NB_MAX_REPAIR)
        return DPU_VPD_ERR_NB_MAX_REPAIR;

    if (vpd->vpd_header.repair_count == VPD_UNDEFINED_REPAIR_COUNT)
        vpd->vpd_header.repair_count = 0;

    for (repair = 0; repair < vpd->vpd_header.repair_count; ++repair)
        if (__dpu_vpd_compare_repair_entry(&vpd->repair_entries[repair], repair_entry))
            return DPU_VPD_OK;

    repair_field = (repair_type == DPU_VPD_REPAIR_IRAM) ? &vpd->vpd_header.ranks[repair_entry->rank].iram_repair
                                                        : &vpd->vpd_header.ranks[repair_entry->rank].wram_repair;

    *repair_field |= (1L << ((repair_entry->ci * DPU_MAX_NR_DPUS_PER_CI) + repair_entry->dpu));

    memcpy(&vpd->repair_entries[vpd->vpd_header.repair_count], repair_entry, sizeof(struct dpu_vpd_repair_entry));

    vpd->vpd_header.repair_count++;

    return DPU_VPD_OK;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_disable_dpu(struct dpu_vpd *vpd, uint8_t rank_index_in_dimm, uint8_t slice_id, uint8_t dpu_id)
{
    LOG_FN(VERBOSE, "");
    uint64_t mask = (1ULL << ((slice_id * DPU_MAX_NR_DPUS_PER_CI) + dpu_id));

    if ((vpd->vpd_header.ranks[rank_index_in_dimm].dpu_disabled & mask) == 0) {
        vpd->vpd_header.ranks[rank_index_in_dimm].dpu_disabled |= mask;
        return DPU_VPD_OK;
    }

    return DPU_VPD_ERR_DPU_ALREADY_DISABLED;
}

__API_SYMBOL__ enum dpu_vpd_error
dpu_vpd_enable_dpu(struct dpu_vpd *vpd, uint8_t rank_index_in_dimm, uint8_t slice_id, uint8_t dpu_id)
{
    LOG_FN(VERBOSE, "");
    uint64_t mask = (1ULL << ((slice_id * DPU_MAX_NR_DPUS_PER_CI) + dpu_id));

    if (vpd->vpd_header.ranks[rank_index_in_dimm].dpu_disabled & mask) {
        vpd->vpd_header.ranks[rank_index_in_dimm].dpu_disabled &= ~mask;
        return DPU_VPD_OK;
    }

    return DPU_VPD_ERR_DPU_ALREADY_ENABLED;
}
