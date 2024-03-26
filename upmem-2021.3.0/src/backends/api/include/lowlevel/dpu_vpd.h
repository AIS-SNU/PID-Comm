/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_VPD_H
#define DPU_VPD_H

#include <stdio.h>
#include <stdint.h>

#include <dpu_vpd_structures.h>

/**
 * @file dpu_vpd.h
 * @brief Definition of libdpuvpd interface
 */

/**
 * @brief Get vpd file path from rank device path
 * @param dev_rank_path Path to the rank device
 * @param vpd_path Pointer to an allocated char array to hold vpd path
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_get_vpd_path(const char *dev_rank_path, char *vpd_path);

/**
 * @brief Init vpd content from the vpd passed in argument: if NULL, the content
 * will be initialised to default values.
 * @param vpd_path Path to the vpd file
 * @param vpd Pointer to the vpd file structure
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_init(const char *vpd_path, struct dpu_vpd *vpd);

/**
 * @brief Init vpd database from the file passed in argument: if NULL content
 * will be initialized to defaults value.
 * @param dual_rank Hint for default values (1: P21 default values: 0: E19 default values; other: empty database)
 * @param db_path Path to the vpd database file
 * @param db Pointer to the vpd database
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_db_init(int dual_rank, const char *db_path, struct dpu_vpd_database *db);

/**
 * @brief Add key/value pair to vpd database.
 * @param db Pointer to the vpd database
 * @param key Pointer to the key string.
 * @param value Pointer to the value
 * @param value_len Value length
 * @param value_type Value type (See VPD_TYPE_* enum)
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_db_update(struct dpu_vpd_database *db, const char *key, const uint8_t *value, const int value_len, const int value_type);

/**
 * @brief Destroy vpd database.
 * @param db Pointer to the vpd database
 */
void
dpu_vpd_db_destroy(struct dpu_vpd_database *db);

/**
 * @brief Write vpd content to file
 * @param vpd Pointer to the vpd file structure
 * @param vpd_path Path to the file
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_write(struct dpu_vpd *vpd, const char *vpd_path);

/**
 * @brief Write vpd database content to file
 * @param db Pointer to the vpd databse
 * @param db_path Path to the file
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_db_write(const struct dpu_vpd_database *db, const char *db_path);

/**
 * @brief Commit changes to MCU flash
 * @param vpd Pointer to the vpd file structure
 * @param device_name Device name is either the path to char device (ie "/dev/dpu_rankX") or
 * the USB serial name (ie "UPMXXXXXXXX")
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_commit_to_device(struct dpu_vpd *vpd, const char *device_name);

/**
 * @brief Commit vpd database to MCU flash. MCU should be in DFU mode.
 * @param db Pointer to the vpd database
 * @param device_name Device name is either the path to char device (ie "/dev/dpu_rankX") or
 * the USB serial name (ie "UPMXXXXXXXX")
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_db_commit_to_device(const struct dpu_vpd_database *db, const char *device_name);

/**
 * @brief Commit changes to MCU flash
 * @param vpd_path Path to the vpd content
 * @param device_name Device name is either the path to char device (ie "/dev/dpu_rankX") or
 * the USB serial name (ie "UPMXXXXXXXX")
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_commit_to_device_from_file(const char *vpd_path, const char *device_name);

/**
 * @brief Commit changes to MCU flash. MCU should be in DFU mode
 * @param db_path Path to the vpd database content
 * @param device_name Device name is either the path to char device (ie "/dev/dpu_rankX") or
 * the USB serial name (ie "UPMXXXXXXXX")
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_db_commit_to_device_from_file(const char *db_path, const char *device_name);

/**
 * @brief Commit changes to file or MCU flash, required to be called to make
 * commit changes
 * @param vpd Pointer to the vpd file structure
 * @param repair_entry Pointer to the repair entry to add
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_add_repair_entry(struct dpu_vpd *vpd, struct dpu_vpd_repair_entry *repair_entry);

/**
 * @brief Disable given DPU in vpd
 * @param vpd Pointer to the vpd file structure
 * @param rank_index_in_dimm Index of the rank in the DIMM
 * @param slice_id Slice id of the DPU
 * @param dpu_id DPU id of the DPU
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_disable_dpu(struct dpu_vpd *vpd, uint8_t rank_index_in_dimm, uint8_t slice_id, uint8_t dpu_id);

/**
 * @brief Enable given DPU in vpd
 * @param vpd Pointer to the vpd file structure
 * @param rank_index_in_dimm Index of the rank in the DIMM
 * @param slice_id Slice id of the DPU
 * @param dpu_id DPU id of the DPU
 * @return DPU_VPD_OK in case of success, otherwise the corresponding error code.
 */
enum dpu_vpd_error
dpu_vpd_enable_dpu(struct dpu_vpd *vpd, uint8_t rank_index_in_dimm, uint8_t slice_id, uint8_t dpu_id);

#endif /* DPU_VPD_H */
