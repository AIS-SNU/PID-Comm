/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef DPU_VPD_DECODE_H
#define DPU_VPD_DECODE_H

#include <stdint.h>

#include "dpu_vpd_structures.h"

/* Callback for vpd_decode_string to invoke */
typedef int
vpd_decode_callback(const uint8_t *key, int32_t key_len, const uint8_t *value, int32_t value_len, int value_type, void *arg);

enum dpu_vpd_error
vpd_decode_len(const int max_len, const uint8_t *in, int *length, int *decoded_len);

enum dpu_vpd_error
vpd_decode_string(const int32_t max_len,
    const uint8_t *input_buf,
    int32_t *consumed,
    vpd_decode_callback callback,
    void *callback_arg);

#endif /* DPU_VPD_DECODE_H */