/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdint.h>

#include "dpu_vpd_decode.h"
#include "dpu_vpd_structures.h"

enum dpu_vpd_error
vpd_decode_len(const int max_len, const uint8_t *in, int *length, int *decoded_len)
{
    uint8_t more;
    int i = 0;

    *length = 0;
    do {
        if (i >= max_len)
            return DPU_VPD_ERR_OVERFLOW;

        more = in[i] & 0x80;
        *length <<= 7;
        *length |= in[i] & 0x7f;
        ++i;
    } while (more);

    *decoded_len = i;

    return DPU_VPD_OK;
}

enum dpu_vpd_error
vpd_decode_string(const int32_t max_len,
    const uint8_t *input_buf,
    int32_t *consumed,
    vpd_decode_callback callback,
    void *callback_arg)
{
    int type;
    int32_t key_len, value_len;
    int32_t decoded_len;
    const uint8_t *key, *value;

    /* type */
    if (*consumed >= max_len)
        return DPU_VPD_ERR_OVERFLOW;

    type = input_buf[*consumed];
    switch (type) {
        case VPD_TYPE_STRING:
        case VPD_TYPE_BYTE:
        case VPD_TYPE_SHORT:
        case VPD_TYPE_INT:
        case VPD_TYPE_LONG:
        case VPD_TYPE_BYTEARRAY:
            (*consumed)++;
            /* key */
            if (vpd_decode_len(max_len - *consumed, &input_buf[*consumed], &key_len, &decoded_len) != DPU_VPD_OK
                || *consumed + decoded_len >= max_len) {
                return DPU_VPD_ERR_OVERFLOW;
            }

            *consumed += decoded_len;
            key = &input_buf[*consumed];
            *consumed += key_len;

            /* value */
            if (vpd_decode_len(max_len - *consumed, &input_buf[*consumed], &value_len, &decoded_len) != DPU_VPD_OK
                || *consumed + decoded_len > max_len) {
                return DPU_VPD_ERR_OVERFLOW;
            }
            *consumed += decoded_len;
            value = &input_buf[*consumed];
            *consumed += value_len;

            return callback(key, key_len, value, value_len, type, callback_arg);

        default:
            return DPU_VPD_ERR;
            break;
    }

    return DPU_VPD_OK;
}