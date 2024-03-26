/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <libudev.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <dpu_description.h>

/* Header shared with driver */
#include "dpu_rank_ioctl.h"
#include "dpu_region_address_translation.h"
#include "dpu_region_constants.h"
#include "hw_dpu_sysfs.h"

#include "dpu_log_utils.h"
#include "static_verbose.h"

static struct verbose_control *this_vc;
static struct verbose_control *
__vc()
{
    if (this_vc == NULL) {
        this_vc = get_verbose_control_for("hw");
    }
    return this_vc;
}

#define INVALID_RANK_INDEX 255

#define init_udev_enumerator(enumerate, udev, sysname, subname, parent, devices, label)                                          \
    udev = udev_new();                                                                                                           \
    if (!udev) {                                                                                                                 \
        LOG_FN(WARNING, "can't create udev");                                                                                    \
        goto label;                                                                                                              \
    }                                                                                                                            \
    enumerate = udev_enumerate_new(udev);                                                                                        \
    if ((sysname) != NULL)                                                                                                       \
        udev_enumerate_add_match_sysname(enumerate, sysname);                                                                    \
    else if ((subname) != NULL)                                                                                                  \
        udev_enumerate_add_match_subsystem(enumerate, subname);                                                                  \
    else                                                                                                                         \
        goto label;                                                                                                              \
    if ((parent) != NULL)                                                                                                        \
        udev_enumerate_add_match_parent(enumerate, parent);                                                                      \
    udev_enumerate_scan_devices(enumerate);                                                                                      \
    devices = udev_enumerate_get_list_entry(enumerate);

#define dpu_sys_get_integer_sysattr(name, udev, type, format, default_value)                                                     \
    const char *str;                                                                                                             \
    type value;                                                                                                                  \
                                                                                                                                 \
    str = udev_device_get_sysattr_value(rank_fs->udev.dev, name);                                                                \
    if (str == NULL)                                                                                                             \
        return (type)default_value;                                                                                              \
                                                                                                                                 \
    sscanf(str, format, &value);                                                                                                 \
                                                                                                                                 \
    return value;

#define dpu_sys_set_integer_sysattr(name, udev, value, format)                                                                   \
    char str[32];                                                                                                                \
                                                                                                                                 \
    sprintf(str, format, value);                                                                                                 \
                                                                                                                                 \
    return udev_device_set_sysattr_value(rank_fs->udev.dev, name, str) < 0;

#define dpu_sys_get_string_sysattr(name, udev)                                                                                   \
    const char *str;                                                                                                             \
                                                                                                                                 \
    str = udev_device_get_sysattr_value(rank_fs->udev.dev, name);                                                                \
    if (str == NULL)                                                                                                             \
        return NULL;                                                                                                             \
                                                                                                                                 \
    return str;

uint64_t
dpu_sysfs_get_region_size(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("size", udev_dax, uint64_t, "%" SCNu64, 0) }

uint8_t
    dpu_sysfs_get_channel_id(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("channel_id", udev, uint8_t, "%hhu", -1) }

uint8_t dpu_sysfs_get_rank_id(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("rank_id", udev, uint8_t, "%hhu", 0) }

uint8_t
    dpu_sysfs_get_backend_id(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("backend_id", udev, uint8_t, "%hhu", 0) }

uint8_t dpu_sysfs_get_dpu_chip_id(
    struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("dpu_chip_id", udev, uint8_t, "%hhu", 0) }

uint8_t dpu_sysfs_get_nb_ci(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("nb_ci", udev, uint8_t, "%hhu", 0) }

uint8_t dpu_sysfs_get_nb_dpus_per_ci(
    struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("nb_dpus_per_ci", udev, uint8_t, "%hhu", 0) }

uint32_t
    dpu_sysfs_get_mram_size(struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("mram_size", udev, uint32_t, "%u", 0) }

uint64_t dpu_sysfs_get_capabilities(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("capabilities", udev, uint64_t, "%" SCNx64, 0)
}

int
dpu_sysfs_set_reset_ila(struct dpu_rank_fs *rank_fs, uint8_t val) { dpu_sys_set_integer_sysattr("reset_ila", udev, val, "%hhu") }

uint32_t dpu_sysfs_get_activate_ila(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("activate_ila", udev, uint8_t, "%hhu", 0)
}

int
dpu_sysfs_set_activate_ila(struct dpu_rank_fs *rank_fs,
    uint8_t val) { dpu_sys_set_integer_sysattr("activate_ila", udev, val, "%hhu") }

uint32_t dpu_sysfs_get_activate_filtering_ila(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("activate_filtering_ila", udev, uint8_t, "%hhu", 0)
}

int
dpu_sysfs_set_activate_filtering_ila(struct dpu_rank_fs *rank_fs,
    uint8_t val) { dpu_sys_set_integer_sysattr("activate_filtering_ila", udev, val, "%hhu") }

uint32_t dpu_sysfs_get_activate_mram_bypass(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("activate_mram_bypass", udev, uint8_t, "%hhu", 0)
}

int
dpu_sysfs_set_activate_mram_bypass(struct dpu_rank_fs *rank_fs,
    uint8_t val) { dpu_sys_set_integer_sysattr("activate_mram_bypass", udev, val, "%hhu") }

uint32_t dpu_sysfs_get_mram_refresh_emulation_period(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("mram_refresh_emulation_period", udev, uint32_t, "%u", 0)
}

int
dpu_sysfs_set_mram_refresh_emulation_period(struct dpu_rank_fs *rank_fs, uint32_t val)
{
    dpu_sys_set_integer_sysattr("mram_refresh_emulation_period", udev, val, "%u")
}

int
dpu_sysfs_set_inject_faults(struct dpu_rank_fs *rank_fs,
    uint8_t val) { dpu_sys_set_integer_sysattr("inject_faults", udev, val, "%hhu") }

uint32_t dpu_sysfs_get_fck_frequency(
    struct dpu_rank_fs *rank_fs) { dpu_sys_get_integer_sysattr("fck_frequency", udev, uint32_t, "%u", 0) }

uint32_t dpu_sysfs_get_clock_division(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("clock_division", udev, uint32_t, "%u", 0)
}

int
dpu_sysfs_get_numa_node(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_integer_sysattr("numa_node", udev_dax, int, "%d", 0)
}

const char *
dpu_sysfs_get_byte_order(struct dpu_rank_fs *rank_fs)
{
    dpu_sys_get_string_sysattr("byte_order", udev);
}

static int
dpu_sysfs_try_to_allocate_rank(const char *dev_rank_path, struct dpu_rank_fs *rank_fs)
{
    struct udev_list_entry *dev_dax_list_entry;
    uint64_t capabilities;

    /* Whatever the mode, we keep an fd to dpu_rank so that
     * we have infos about how/who uses the rank
     */
    rank_fs->fd_rank = open(dev_rank_path, O_RDWR);
    if (rank_fs->fd_rank < 0)
        return -errno;

    /* udev_device_get_parent does not take a reference as stated in header */
    rank_fs->udev_parent.dev = udev_device_get_parent(rank_fs->udev.dev);

    /* Dax device only exists if backend supports PERF mode */
    capabilities = dpu_sysfs_get_capabilities(rank_fs);

    if (capabilities & CAP_PERF) {
        /* There's only one dax device associated to the region,
         * but we use the enumerate match facility to find it.
         */
        init_udev_enumerator(rank_fs->udev_dax.enumerate,
            rank_fs->udev_dax.udev,
            "dax*",
            NULL,
            rank_fs->udev_parent.dev,
            rank_fs->udev_dax.devices,
            err);

        udev_list_entry_foreach(dev_dax_list_entry, rank_fs->udev_dax.devices)
        {
            const char *path_dax, *dev_dax_path;

            path_dax = udev_list_entry_get_name(dev_dax_list_entry);
            rank_fs->udev_dax.dev = udev_device_new_from_syspath(rank_fs->udev_dax.udev, path_dax);
            dev_dax_path = udev_device_get_devnode(rank_fs->udev_dax.dev);

            rank_fs->fd_dax = open(dev_dax_path, O_RDWR);
            if (rank_fs->fd_dax >= 0)
                return 0;

            LOG_FN(WARNING, "Error (%d: '%s') opening dax device '%s'", errno, strerror(errno), dev_dax_path);
            udev_device_unref(rank_fs->udev_dax.dev);
        }

        udev_enumerate_unref(rank_fs->udev_dax.enumerate);
        udev_unref(rank_fs->udev_dax.udev);
    } else
        return 0;

err:
    close(rank_fs->fd_rank);

    return -EINVAL;
}

void
dpu_sysfs_free_rank_fs(struct dpu_rank_fs *rank_fs)
{
    if (rank_fs->fd_dax)
        close(rank_fs->fd_dax);

    close(rank_fs->fd_rank);

    if (rank_fs->udev_dax.udev) {
        udev_unref(rank_fs->udev_dax.udev);
        udev_device_unref(rank_fs->udev_dax.dev);
        udev_enumerate_unref(rank_fs->udev_dax.enumerate);
    }

    udev_unref(rank_fs->udev.udev);
    udev_device_unref(rank_fs->udev.dev);
    udev_enumerate_unref(rank_fs->udev.enumerate);
}

void
dpu_sysfs_free_rank(struct dpu_rank_fs *rank_fs)
{
    dpu_sysfs_free_rank_fs(rank_fs);
}

uint8_t
dpu_sysfs_get_nb_physical_ranks()
{
    struct dpu_rank_udev udev;
    struct udev_list_entry *dev_rank_list_entry;
    uint8_t nb_ranks = 0;

    init_udev_enumerator(udev.enumerate, udev.udev, NULL, "dpu_rank", NULL, udev.devices, end);

    udev_list_entry_foreach(dev_rank_list_entry, udev.devices) { nb_ranks++; }

    udev_enumerate_unref(udev.enumerate);
    udev_unref(udev.udev);

end:
    return nb_ranks;
}

// TODO allocation must be smarter than just allocating rank "one by one":
// it is better (at memory bandwidth point of view) to allocate ranks
// from unused channels rather than allocating ranks of a same channel
// (memory bandwidth is limited at a channel level)
int
dpu_sysfs_get_available_rank(const char *rank_path, struct dpu_rank_fs *rank_fs)
{
    struct udev_list_entry *dev_rank_list_entry;
    int eacces_count = 0;

    init_udev_enumerator(rank_fs->udev.enumerate, rank_fs->udev.udev, NULL, "dpu_rank", NULL, rank_fs->udev.devices, end);

    udev_list_entry_foreach(dev_rank_list_entry, rank_fs->udev.devices)
    {
        const char *path_rank, *dev_rank_path;

        path_rank = udev_list_entry_get_name(dev_rank_list_entry);
        rank_fs->udev.dev = udev_device_new_from_syspath(rank_fs->udev.udev, path_rank);
        dev_rank_path = udev_device_get_devnode(rank_fs->udev.dev);

        if (strlen(rank_path)) {
            if (!strcmp(dev_rank_path, rank_path)) {
                if (!dpu_sysfs_try_to_allocate_rank(dev_rank_path, rank_fs)) {
                    strcpy(rank_fs->rank_path, dev_rank_path);
                    return 0;
                } else {
                    LOG_FN(WARNING, "Allocation of requested %s rank failed", rank_path);
                    udev_device_unref(rank_fs->udev.dev);
                    break;
                }
            }
        } else {
            int res = dpu_sysfs_try_to_allocate_rank(dev_rank_path, rank_fs);
            if (!res) {
                strcpy(rank_fs->rank_path, dev_rank_path);
                return 0;
            }
            /* record whether we have found something that we cannot access */
            if (res == -EACCES)
                eacces_count++;
        }

        udev_device_unref(rank_fs->udev.dev);
    }
    udev_enumerate_unref(rank_fs->udev.enumerate);
    udev_unref(rank_fs->udev.udev);

end:
    return eacces_count ? -EACCES : -ENODEV;
}

// We assume there is only one chip id per machine: so we just need to get that info
// from the first rank.
int
dpu_sysfs_get_hardware_chip_id(uint8_t *chip_id)
{
    struct udev_list_entry *dev_rank_list_entry;
    struct dpu_rank_fs rank_fs;

    init_udev_enumerator(rank_fs.udev.enumerate, rank_fs.udev.udev, NULL, "dpu_rank", NULL, rank_fs.udev.devices, end);

    udev_list_entry_foreach(dev_rank_list_entry, rank_fs.udev.devices)
    {
        const char *path_rank;

        path_rank = udev_list_entry_get_name(dev_rank_list_entry);
        rank_fs.udev.dev = udev_device_new_from_syspath(rank_fs.udev.udev, path_rank);

        /* Get the chip id from the driver */
        *chip_id = dpu_sysfs_get_dpu_chip_id(&rank_fs);

        udev_device_unref(rank_fs.udev.dev);

        break;
    }
    udev_enumerate_unref(rank_fs.udev.enumerate);
    udev_unref(rank_fs.udev.udev);

    return 0;

end:
    return -1;
}

// We assume the topology is identical for all the ranks: so we just need to get that info
// from the first rank.
int
dpu_sysfs_get_hardware_description(dpu_description_t description, uint8_t *capabilities_mode)
{
    struct udev_list_entry *dev_rank_list_entry;
    struct dpu_rank_fs rank_fs;

    init_udev_enumerator(rank_fs.udev.enumerate, rank_fs.udev.udev, NULL, "dpu_rank", NULL, rank_fs.udev.devices, end);

    udev_list_entry_foreach(dev_rank_list_entry, rank_fs.udev.devices)
    {
        const char *path_rank;

        path_rank = udev_list_entry_get_name(dev_rank_list_entry);
        rank_fs.udev.dev = udev_device_new_from_syspath(rank_fs.udev.udev, path_rank);

        /* Get the real topology from the driver */
        uint8_t clock_division;
        uint32_t fck_frequency_in_mhz;
        description->hw.topology.nr_of_dpus_per_control_interface = dpu_sysfs_get_nb_dpus_per_ci(&rank_fs);
        description->hw.topology.nr_of_control_interfaces = dpu_sysfs_get_nb_ci(&rank_fs);
        description->hw.memories.mram_size = dpu_sysfs_get_mram_size(&rank_fs);
        clock_division = dpu_sysfs_get_clock_division(&rank_fs);
        fck_frequency_in_mhz = dpu_sysfs_get_fck_frequency(&rank_fs);
        /* Keep clock_division and fck_frequency_in_mhz default value if sysfs returns 0 */
        if (clock_division)
            description->hw.timings.clock_division = clock_division;
        if (fck_frequency_in_mhz)
            description->hw.timings.fck_frequency_in_mhz = fck_frequency_in_mhz;

        *capabilities_mode = dpu_sysfs_get_capabilities(&rank_fs);

        udev_device_unref(rank_fs.udev.dev);

        break;
    }
    udev_enumerate_unref(rank_fs.udev.enumerate);
    udev_unref(rank_fs.udev.udev);

    return 0;

end:
    return -1;
}

int
dpu_sysfs_get_kernel_module_version(unsigned int *major, unsigned int *minor)
{
    FILE *fp;

    if ((fp = fopen("/sys/module/dpu/version", "r")) == NULL)
        return -errno;

    if (fscanf(fp, "%u.%u", major, minor) != 2) {
        fclose(fp);
        return (errno != 0) ? -errno : -1; // errno can be 0 is there is no matching character
    }

    fclose(fp);
    return 0;
}

int
dpu_sysfs_get_dimm_serial_number(const char *rank_path, char **serial_number)
{
    unsigned int rank_id;
    char *serial_number_path = NULL;

    sscanf(rank_path, "/dev/dpu_rank%u", &rank_id);

    if (asprintf(&serial_number_path, "/sys/class/dpu_rank/dpu_rank%u/serial_number", rank_id) == -1) {
        return -ENOMEM;
    }

    FILE *sn;

    if ((sn = fopen(serial_number_path, "r")) == NULL) {
        free(serial_number_path);
        return -errno;
    }
    free(serial_number_path);

    size_t len = 0;
    if (getline(serial_number, &len, sn) == -1) {
        fclose(sn);
        return -errno;
    }
    fclose(sn);

    (*serial_number)[strlen(*serial_number) - 1] = '\0';
    if (strlen(*serial_number) == 0) {
        /* Could not request DIMM serial number from MCU */
        free(*serial_number);
        return -1;
    }

    return 0;
}

int
dpu_sysfs_get_rank_index(const char *rank_path, int *index)
{
    unsigned int rank_id;
    char *rank_index_path = NULL;

    sscanf(rank_path, "/dev/dpu_rank%u", &rank_id);

    if (asprintf(&rank_index_path, "/sys/class/dpu_rank/dpu_rank%u/rank_index", rank_id) == -1) {
        return -ENOMEM;
    }

    FILE *idx;

    if ((idx = fopen(rank_index_path, "r")) == NULL) {
        free(rank_index_path);
        return -errno;
    }
    free(rank_index_path);

    if (fscanf(idx, "%d", index) != 1) {
        fclose(idx);
        return (errno != 0) ? -errno : -1;
    }
    fclose(idx);

    if (*index == INVALID_RANK_INDEX) {
        /* Could not request rank index from MCU */
        return -1;
    }

    return 0;
}
