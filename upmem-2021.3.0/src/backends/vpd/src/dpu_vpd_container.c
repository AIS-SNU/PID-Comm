/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dpu_vpd_container.h"
#include "dpu_vpd_decode.h"
#include "dpu_vpd_encode.h"
#include "dpu_vpd_structures.h"

static struct dpu_vpd_string_pair *
vpd_find_string(struct dpu_vpd_database *db, const uint8_t *key)
{
    struct dpu_vpd_string_pair *current;

    for (current = db->first; current; current = current->next) {
        if (!strcmp((char *)key, (char *)current->key)) {
            return current;
        }
    }
    return NULL;
}

/* Just a helper function for vpd_set_string() */
static enum dpu_vpd_error
vpd_fill_string_pair(struct dpu_vpd_string_pair *pair,
    const uint8_t *key,
    const uint8_t *value,
    const int value_len,
    const int value_type)
{
    if ((pair->key = malloc(strlen((char *)key) + 1)) == NULL)
        return DPU_VPD_ERR;
    strcpy((char *)pair->key, (char *)key);

    if (value_type == VPD_TYPE_STRING) {
        /* add room for '\0' */
        if ((pair->value = malloc(value_len + 1)) == NULL) {
            goto err_value;
        }
        memcpy(pair->value, value, value_len);
        pair->value[value_len] = '\0';
    } else {
        if ((pair->value = malloc(value_len)) == NULL) {
            goto err_value;
        }
        memcpy(pair->value, value, value_len);
    }

    pair->value_len = value_len;
    pair->value_type = value_type;
    return DPU_VPD_OK;

err_value:
    free(pair->key);
    return DPU_VPD_ERR;
}

/* If key already exists, its value will be replaced.
 * If not, it creates new entry.
 */
enum dpu_vpd_error
vpd_set_string(struct dpu_vpd_database *db, const uint8_t *key, const uint8_t *value, const int value_len, const int value_type)
{
    struct dpu_vpd_string_pair *found;
    struct dpu_vpd_string_pair *new_pair;

    found = vpd_find_string(db, key);
    if (found) {
        free(found->key);
        free(found->value);
        found->value_len = 0;
        return vpd_fill_string_pair(found, key, value, value_len, value_type);
    }

    if ((new_pair = malloc(sizeof(struct dpu_vpd_string_pair))) == NULL)
        return DPU_VPD_ERR;
    memset(new_pair, 0, sizeof(struct dpu_vpd_string_pair));

    if (vpd_fill_string_pair(new_pair, key, value, value_len, value_type) != DPU_VPD_OK) {
        free(new_pair);
        return DPU_VPD_ERR;
    }

    /* append this pair to the end of list. to keep the order */
    if ((found = db->first)) {
        while (found->next)
            found = found->next;
        found->next = new_pair;
    } else {
        db->first = new_pair;
    }
    new_pair->next = NULL;
    return DPU_VPD_OK;
}

enum dpu_vpd_error
vpd_encode_container(const struct dpu_vpd_database *db, const int max_buf_len, uint8_t *buf, int *generated)
{
    struct dpu_vpd_string_pair *current;

    *generated = 0;
    for (current = db->first; current; current = current->next) {
        if (vpd_encode_string(current->key, current->value, current->value_len, current->value_type, max_buf_len, buf, generated)
            != DPU_VPD_OK) {
            return DPU_VPD_ERR;
        }
    }
    return DPU_VPD_OK;
}

static int
vpd_decode_cb(const uint8_t *key, int32_t key_len, const uint8_t *value, int32_t value_len, int value_type, void *arg)
{
    struct dpu_vpd_database *db = (struct dpu_vpd_database *)arg;
    uint8_t *key_string;
    enum dpu_vpd_error ret;

    /* vpd_set_string requires a string */
    if ((key_string = malloc(key_len + 1)) == NULL)
        return DPU_VPD_ERR;
    memcpy(key_string, key, key_len);
    key_string[key_len] = '\0';

    ret = vpd_set_string(db, key_string, value, value_len, value_type);
    free(key_string);
    return (int)ret;
}

enum dpu_vpd_error
vpd_decode_to_container(const uint8_t *buf, const int max_buf_len, struct dpu_vpd_database *db)
{
    int32_t consumed;
    enum dpu_vpd_error ret;

    consumed = 0;
    do {
        ret = vpd_decode_string(max_buf_len, buf, &consumed, vpd_decode_cb, db);
    } while (ret == DPU_VPD_OK);

    return DPU_VPD_OK;
}

int
vpd_get_container_length(const struct dpu_vpd_database *db)
{
    int count;
    struct dpu_vpd_string_pair *current;

    for (count = 0, current = db->first; current; count++, current = current->next)
        ;
    return count;
}

void
vpd_init_container(struct dpu_vpd_database *db)
{
    db->first = NULL;
}

void
vpd_destroy_container(struct dpu_vpd_database *db)
{
    struct dpu_vpd_string_pair *current;

    for (current = db->first; current;) {
        struct dpu_vpd_string_pair *next;
        if (current->key)
            free(current->key);
        if (current->value)
            free(current->value);
        next = current->next;
        free(current);
        current = next;
    }
}