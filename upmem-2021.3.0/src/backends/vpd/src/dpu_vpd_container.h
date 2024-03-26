/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef DPU_VPD_CONTAINER_H
#define DPU_VPD_CONTAINER_H

#include <stdint.h>

#include <dpu_vpd_structures.h>

#define VPD_ADD_BYTE(key, value)                                                                                                 \
    uint8_t key = (uint8_t)value;                                                                                                \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)&key, sizeof(key), VPD_TYPE_BYTE) != DPU_VPD_OK)                          \
        return DPU_VPD_ERR;
#define VPD_ADD_SHORT(key, value)                                                                                                \
    uint16_t key = (uint16_t)value;                                                                                              \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)&key, sizeof(key), VPD_TYPE_SHORT) != DPU_VPD_OK)                         \
        return DPU_VPD_ERR;
#define VPD_ADD_INT(key, value)                                                                                                  \
    uint32_t key = (uint32_t)value;                                                                                              \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)&key, sizeof(key), VPD_TYPE_INT) != DPU_VPD_OK)                           \
        return DPU_VPD_ERR;
#define VPD_ADD_LONG(key, value)                                                                                                 \
    uint64_t key = (uint64_t)value;                                                                                              \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)&key, sizeof(key), VPD_TYPE_LONG) != DPU_VPD_OK)                          \
        return DPU_VPD_ERR;
#define VPD_ADD_BYTEARRAY(key, ...)                                                                                              \
    uint8_t key[] = { __VA_ARGS__ };                                                                                             \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)key, sizeof(key), VPD_TYPE_BYTEARRAY) != DPU_VPD_OK)                      \
        return DPU_VPD_ERR;
#define VPD_ADD_STRING(key, string)                                                                                              \
    char *key = string;                                                                                                          \
    if (vpd_set_string(db, (uint8_t *)#key, (uint8_t *)key, strlen(key), VPD_TYPE_STRING) != DPU_VPD_OK)                         \
        return DPU_VPD_ERR;

enum dpu_vpd_error
vpd_set_string(struct dpu_vpd_database *db, const uint8_t *key, const uint8_t *value, const int value_len, const int value_type);
enum dpu_vpd_error
vpd_encode_container(const struct dpu_vpd_database *db, const int max_buf_len, uint8_t *buf, int *generated);
enum dpu_vpd_error
vpd_decode_to_container(const uint8_t *buf, const int max_buf_len, struct dpu_vpd_database *db);
int
vpd_get_container_length(const struct dpu_vpd_database *db);
void
vpd_init_container(struct dpu_vpd_database *db);
void
vpd_destroy_container(struct dpu_vpd_database *db);

#endif /* DPU_VPD_CONTAINER_H */