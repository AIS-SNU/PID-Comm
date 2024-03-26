/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_API_COMMON_H
#define DPU_API_COMMON_H

#include <static_verbose.h>

static struct verbose_control *this_vc;
static inline struct verbose_control *
__vc()
{
    if (this_vc == NULL) {
        this_vc = get_verbose_control_for("api");
    }
    return this_vc;
}

#endif /* DPU_API_COMMON_H */
