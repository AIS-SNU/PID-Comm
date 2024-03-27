/* Copyright 2021 UPMEM.
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef DPU_VPD_ENCODE_H
#define DPU_VPD_ENCODE_H

#include <stdint.h>

#include "dpu_vpd_structures.h"

enum dpu_vpd_error
vpd_encode_len(const int len, uint8_t *encode_buf, const int max_len, int *encoded_len);

enum dpu_vpd_error
vpd_encode_string(const uint8_t *key,
    const uint8_t *value,
    const int value_len,
    const int value_type,
    const int max_buffer_len,
    uint8_t *output_buf,
    int *generated_len);

#endif /* DPU_VPD_ENCODE_H */