/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <dpu_api_log.h>
#include <dpu_attributes.h>
#include <dpu_program.h>
#include <dpu_rank.h>
#include <dpu_memory.h>
#include <dpu_log_internals.h>
#include <dpu_management.h>

#define MAX_FMT_ARGS 64

/* Contains a printf format definition (i.e. %...). */
typedef char format_t[16];

/*
 * Maintains an internal reader state.
 *
 * The reader is basically a data pump, behaving as a sequential
 * byte reader (no pre-fetching into a temporary buffer).
 */
typedef struct {
    // Contains the log buffer fetched from DPU.
    uint8_t *buffer;
    // The end of the log buffer fetched from DPU (first address out of the buffer).
    uint8_t *buffer_end;
    // Format of the current displayed message.
    format_t format[MAX_FMT_ARGS];
    // Number of effective entries in format
    unsigned int format_count;
} dpulog_reader_t;

typedef enum {
    PARSE_FORMAT_SECTION_SUCCESS,
    PARSE_FORMAT_SECTION_ERROR,
    PARSE_FORMAT_SECTION_END_OF_BUFFER,
} parse_format_section_ret_t;

#define DPU_LOG_CHECK(statement)                                                                                                 \
    do {                                                                                                                         \
        dpu_error_t _dpu_log_error = (statement);                                                                                \
        if (_dpu_log_error != DPU_OK) {                                                                                          \
            return _dpu_log_error;                                                                                               \
        }                                                                                                                        \
    } while (0)

static parse_format_section_ret_t
parse_format_section(dpulog_reader_t *reader)
{
    uint8_t *str = reader->buffer;

    while ((str < reader->buffer_end) && (*str != '%'))
        str++;
    if (str >= reader->buffer_end)
        return PARSE_FORMAT_SECTION_END_OF_BUFFER;

    format_t *format = NULL;
    int char_count = 0;
    reader->format_count = 0;

    for (; *str != 0 && (str < reader->buffer_end); str++) {
        if (*str != '%') {
            if (format == NULL)
                return PARSE_FORMAT_SECTION_ERROR;
            (*format)[char_count++] = *str;
            (*format)[char_count] = '\0';
        } else {
            format = &reader->format[reader->format_count++];
            char_count = 0;
            (*format)[char_count++] = '%';
            (*format)[char_count] = '\0';
        }
    }

    if (str >= reader->buffer_end)
        return PARSE_FORMAT_SECTION_END_OF_BUFFER;

    reader->buffer = str + 1;

    return PARSE_FORMAT_SECTION_SUCCESS;
}

static dpu_error_t
parse_and_print_argument_section(dpulog_reader_t *reader, dpu_log_print_fct_t print_fct, void *print_fct_arg)
{
    unsigned int fmt_index;
    char l_fmt;

    for (fmt_index = 0; (fmt_index < reader->format_count); fmt_index++) {
        size_t l_fmt_index = strlen(reader->format[fmt_index]) - 1;
        l_fmt = reader->format[fmt_index][l_fmt_index];
        switch (l_fmt) {
            case 's':
                DPU_LOG_CHECK(print_fct(print_fct_arg, reader->format[fmt_index], reader->buffer));
                reader->buffer += strlen((const char *)reader->buffer) + 1;
                break;

            case 'c': {
                DPU_LOG_CHECK(print_fct(print_fct_arg, reader->format[fmt_index], *reader->buffer));
                reader->buffer++;
            } break;

            case 'b': {
                // custom formatter for binary output

                int i = *(int *)(reader->buffer);
                for (int each_bit = 0; each_bit < 32; ++each_bit) {
                    DPU_LOG_CHECK(print_fct(print_fct_arg, "%u", (i >> each_bit) & 1));
                }
                reader->buffer += sizeof(int);
            } break;

            case 'd':
            case 'i':
            case 'x':
            case 'X':
            case 'o':
            case 'p':
            case 'u': {
                bool is_64_bit = false;

                for (unsigned int each_char = 0; each_char < l_fmt_index; ++each_char) {
                    if (reader->format[fmt_index][each_char] == 'l') {
                        is_64_bit = true;
                        break;
                    }
                }

                if (is_64_bit) {
                    long l = *(long *)(reader->buffer);
                    DPU_LOG_CHECK(print_fct(print_fct_arg, reader->format[fmt_index], l));
                    reader->buffer += sizeof(long);
                } else {
                    int i = *(int *)(reader->buffer);
                    DPU_LOG_CHECK(print_fct(print_fct_arg, reader->format[fmt_index], i));
                    reader->buffer += sizeof(int);
                }
            } break;
            case 'e':
            case 'E':
            case 'f': {
                double d = *(double *)(reader->buffer);
                DPU_LOG_CHECK(print_fct(print_fct_arg, reader->format[fmt_index], d));
                reader->buffer += sizeof(double);
            } break;
            default: {
                // do not display anything for unsupported formats (A, p, e, etc)
                DPU_LOG_CHECK(print_fct(print_fct_arg, "%s", reader->format[fmt_index]));
                reader->buffer += sizeof(int);
            }
        }
    }
    return DPU_OK;
}

dpu_error_t __API_SYMBOL__
dpulog_read_and_display_contents_of(void *input, size_t input_size, dpu_log_print_fct_t print_fct, void *print_fct_arg)
{
    dpulog_reader_t reader = { .buffer = input, .buffer_end = (uint8_t *)input + input_size };

    while (true) {
        switch (parse_format_section(&reader)) {
            case PARSE_FORMAT_SECTION_SUCCESS:
                DPU_LOG_CHECK(parse_and_print_argument_section(&reader, print_fct, print_fct_arg));
                break;
            case PARSE_FORMAT_SECTION_ERROR:
                return DPU_ERR_LOG_FORMAT;
            case PARSE_FORMAT_SECTION_END_OF_BUFFER:
                return DPU_OK;
        };
    }
}

static dpu_error_t
get_printf_context(struct dpu_t *dpu,
    uint32_t *printf_buffer_address,
    uint32_t *printf_buffer_size,
    uint32_t *printf_write_pointer,
    uint32_t *printf_buffer_has_wrapped)
{
    uint32_t printf_write_pointer_address;
    uint32_t printf_buffer_has_wrapped_address;
    dpu_error_t api_status;

    if ((dpu->program == NULL) || ((*printf_buffer_address = dpu->program->printf_buffer_address) == (uint32_t)-1)
        || ((*printf_buffer_size = dpu->program->printf_buffer_size) == (uint32_t)-1)
        || ((printf_write_pointer_address = dpu->program->printf_write_pointer_address) == (uint32_t)-1)
        || ((printf_buffer_has_wrapped_address = dpu->program->printf_buffer_has_wrapped_address) == (uint32_t)-1)) {
        return DPU_ERR_LOG_CONTEXT_MISSING;
    }

    api_status = dpu_copy_from_wram_for_dpu(dpu, printf_write_pointer, (wram_addr_t)(printf_write_pointer_address / 4), 1);
    if (api_status != DPU_OK) {
        return api_status;
    }

    api_status
        = dpu_copy_from_wram_for_dpu(dpu, printf_buffer_has_wrapped, (wram_addr_t)(printf_buffer_has_wrapped_address / 4), 1);

    return api_status;
}

dpu_error_t __API_SYMBOL__
dpulog_read_for_dpu_(struct dpu_t *dpu, dpu_log_print_fct_t print_fct, void *print_fct_arg)
{
    if (!dpu->enabled) {
        LOG_DPU(WARNING, dpu, "dpu is disabled");
        return DPU_ERR_DPU_DISABLED;
    }

    dpu_error_t api_status;
    uint32_t printf_buffer_address;
    uint32_t printf_buffer_size;
    uint32_t printf_write_pointer;
    uint32_t printf_buffer_has_wrapped;
    api_status
        = get_printf_context(dpu, &printf_buffer_address, &printf_buffer_size, &printf_write_pointer, &printf_buffer_has_wrapped);
    if (api_status != DPU_OK) {
        if (api_status == DPU_ERR_LOG_CONTEXT_MISSING) {
            LOG_DPU(WARNING, dpu, "dpu program might not use printf or the information has been corrupted");
        } else {
            LOG_DPU(WARNING, dpu, "Could not read dpu context in wram ('%s')", dpu_error_to_string(api_status));
        }
        return api_status;
    }

    if (printf_buffer_has_wrapped) {
        LOG_DPU(WARNING, dpu, "Could not display log buffer because the buffer was too small to contain all messages");
        return DPU_ERR_LOG_BUFFER_TOO_SMALL;
    }

    uint32_t buffer_size = printf_write_pointer;
    uint32_t buffer_index = 0;
    uint8_t *buffer = malloc(sizeof(char) * buffer_size);
    if (buffer == NULL) {
        LOG_DPU(WARNING, dpu, "Could not allocate memory for buffer to get log from dpu");
        return DPU_ERR_SYSTEM;
    }

    api_status = dpu_copy_from_mram(dpu, &buffer[buffer_index], printf_buffer_address, buffer_size);
    if (api_status != DPU_OK) {
        free(buffer);
        LOG_DPU(WARNING, dpu, "Could not read log buffer in mram ('%s')", dpu_error_to_string(api_status));
        return api_status;
    }

    // Write log buffer to stream
    api_status = dpulog_read_and_display_contents_of(buffer, buffer_size, print_fct, print_fct_arg);
    if (api_status != DPU_OK) {
        free(buffer);
        LOG_DPU(WARNING, dpu, "Could not display log buffer in stream ('%s')", dpu_error_to_string(api_status));
        return api_status;
    }

    free(buffer);
    return DPU_OK;
}

dpu_error_t __API_SYMBOL__
dpulog_c_print_fct(void *arg, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf((FILE *)arg, fmt, ap);
    va_end(ap);
    return DPU_OK;
}

dpu_error_t __API_SYMBOL__
dpulog_read_for_dpu(struct dpu_t *dpu, FILE *stream)
{
    return dpulog_read_for_dpu_(dpu, dpulog_c_print_fct, (void *)stream);
}

__API_SYMBOL__ dpu_error_t
dpu_log_read(struct dpu_set_t set, FILE *stream)
{
    fprintf(stream, DPU_LOG_FORMAT_HEADER, dpu_get_id(dpu_from_set(set)));
    switch (set.kind) {
        case DPU_SET_DPU:
            return dpulog_read_for_dpu(set.dpu, stream);
        default:
            return DPU_ERR_INVALID_DPU_SET;
    }
}
