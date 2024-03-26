/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdint.h>
#include <string.h>

#include "dpu_vpd_decode.h"
#include "dpu_vpd_encode.h"
#include "dpu_vpd_structures.h"

/* Encodes the len into multiple bytes with the following format.
 *
 *    7   6 ............ 0
 *  +----+------------------+
 *  |More|      Length      |  ...
 *  +----+------------------+
 *
 * Returns fail if the buffer is not long enough.
 */
enum dpu_vpd_error
vpd_encode_len(const int len, uint8_t *encode_buf, const int max_len, int *encoded_len)
{
    unsigned int shifting;
    unsigned int reversed_7bits = 0;
    int out_index = 0;

    if (len < 0)
        return DPU_VPD_ERR;
    shifting = len;
    /* reverse the len for every 7-bit. The little endian. */
    for (*encoded_len = 0; shifting; (*encoded_len)++) {
        reversed_7bits = (reversed_7bits << 7) | (shifting & 0x7f);
        shifting >>= 7;
    }
    if (*encoded_len > max_len)
        return DPU_VPD_ERR_OVERFLOW;
    if (!*encoded_len)
        *encoded_len = 1; /* output at least 1 byte */

    /* Output in reverse order, now big endian. */
    while (out_index < *encoded_len) {
        /* always set MORE flag */
        encode_buf[out_index++] = 0x80 | (reversed_7bits & 0x7f);
        reversed_7bits >>= 7;
    }
    encode_buf[out_index - 1] &= 0x7f; /* clear the MORE flag in last byte */

    return DPU_VPD_OK;
}

enum dpu_vpd_error
vpd_encode_string(const uint8_t *key,
    const uint8_t *value,
    const int value_len,
    const int value_type,
    const int max_buffer_len,
    uint8_t *output_buf,
    int *generated_len)
{
    int key_len;
    int ret_len;

    key_len = strlen((char *)key);
    output_buf += *generated_len; /* move cursor to end of string */

    /* encode type */
    if (*generated_len >= max_buffer_len)
        return DPU_VPD_ERR_OVERFLOW;
    *(output_buf++) = (uint8_t)value_type;
    (*generated_len)++;

    /* encode key len */
    if (vpd_encode_len(key_len, output_buf, max_buffer_len - *generated_len, &ret_len) != DPU_VPD_OK)
        return DPU_VPD_ERR;
    output_buf += ret_len;
    *generated_len += ret_len;
    /* encode key string */
    if (*generated_len + key_len > max_buffer_len)
        return DPU_VPD_ERR_OVERFLOW;
    memcpy(output_buf, key, key_len);
    output_buf += key_len;
    *generated_len += key_len;

    /* encode value len */
    if (vpd_encode_len(value_len, output_buf, max_buffer_len - *generated_len, &ret_len) != DPU_VPD_OK)
        return DPU_VPD_ERR;
    output_buf += ret_len;
    *generated_len += ret_len;
    /* encode value string */
    if (*generated_len + value_len > max_buffer_len)
        return DPU_VPD_ERR_OVERFLOW;
    memcpy(output_buf, value, value_len);
    output_buf += value_len;
    *generated_len += value_len;

    return DPU_VPD_OK;
}
