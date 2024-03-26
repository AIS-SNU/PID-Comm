/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_LOG_INTERNALS_H
#define DPU_LOG_INTERNALS_H

#include <dpu_error.h>
#include <dpu_types.h>

/**
 * @file dpu_log_internals.h
 * @brief Internals functions for logging of the toolchain (needed for LLDB).
 */

/**
 * @brief Header string used in the different log functions.
 * @private
 * @hideinitializer
 */
#define DPU_LOG_FORMAT_HEADER ("=== DPU#0x%x ===\n")

/**
 * @brief Function that outputs DPU print content.
 */
typedef dpu_error_t (*dpu_log_print_fct_t)(void *arg, const char *format, ...);

/**
 * @brief Fetch the DPU print content and output it using the given function.
 * @param dpu the DPU
 * @param print_fct function called to output the DPU print content
 * @param arg context for print_fct
 * @return Whether the operation was successful.
 */
dpu_error_t
dpulog_read_for_dpu_(struct dpu_t *dpu, dpu_log_print_fct_t print_fct, void *arg);

/**
 * @brief Parse and output the DPU print content.
 * @param input the DPU content
 * @param input_size the number of bytes of the DPU content
 * @param print_fct function called to output the DPU print content
 * @param print_fct_arg context for print_fct
 * @return Whether the operation was successful.
 */
dpu_error_t
dpulog_read_and_display_contents_of(void *input, size_t input_size, dpu_log_print_fct_t print_fct, void *print_fct_arg);

/**
 * @brief Instance of dpu_log_print_fct_t for the C API.
 * @param arg context for this printer
 * @param fmt the content to print
 * @return Whether the operation was successful.
 */
dpu_error_t
dpulog_c_print_fct(void *arg, const char *fmt, ...);

#endif /* DPU_LOG_INTERNALS_H */
