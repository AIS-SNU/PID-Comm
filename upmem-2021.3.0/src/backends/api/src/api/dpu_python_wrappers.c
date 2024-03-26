/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// #include <Python.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include <dpu_error.h>
#include <dpu_program.h>
#include <dpu.h>
#include <dpu_attributes.h>
#include <dpu_api_log.h>
#include <dpu_internals.h>
#include <dpu_log_internals.h>
#include <dpu_management.h>
#include <dpu_rank.h>
#include <dpu_api_memory.h>

// static dpu_error_t
// Py_print_fct(void *arg, const char *fmt, ...)
// {
//     char *str;
//     va_list ap;
//     va_start(ap, fmt);
//     if (vasprintf(&str, fmt, ap) == -1) {
//         LOG_FN(WARNING, "vasprintf failed");
//         return DPU_ERR_SYSTEM;
//     }
//     PyAPI_FUNC(PyGILState_STATE) gil_state = PyGILState_Ensure();
//     PyFile_WriteString(str, (PyObject *)arg);
//     PyGILState_Release(gil_state);
//     free(str);
//     va_end(ap);
//     return DPU_OK;
// }

// dpu_error_t __API_SYMBOL__
// Py_dpu_log_read(struct dpu_set_t set, PyObject *Py_stream)
// {
//     Py_print_fct(Py_stream, DPU_LOG_FORMAT_HEADER, dpu_get_id(dpu_from_set(set)));
//     switch (set.kind) {
//         case DPU_SET_DPU: {
//             return dpulog_read_for_dpu_(set.dpu, Py_print_fct, (void *)Py_stream);
//         }
//         default:
//             return DPU_ERR_INVALID_DPU_SET;
//     }
// }

// static dpu_error_t
// Py_get_buffer_at_index(PyObject *buffers, uint32_t index, void **buffer, uint32_t *size)
// {
//     PyObject *item = PyList_GET_ITEM(buffers, index);

//     if (item == Py_None) {
//         *buffer = NULL;
//         *size = UINT_MAX;
//     } else {
//         Py_buffer py_buffer;

//         if (PyObject_GetBuffer(item, &py_buffer, PyBUF_WRITABLE) == -1) {
//             return DPU_ERR_INVALID_MEMORY_TRANSFER;
//         }

//         *buffer = py_buffer.buf;
//         *size = py_buffer.len;
//         PyBuffer_Release(&py_buffer);
//     }

//     return DPU_OK;
// }

// dpu_error_t __API_SYMBOL__
// Py_dpu_prepare_xfers(struct dpu_set_t dpu_set, PyObject *Py_buffers, size_t *Py_buffer_size)
// {
//     dpu_error_t status = DPU_OK;

//     if (!PyList_Check(Py_buffers)) {
//         return DPU_ERR_INVALID_MEMORY_TRANSFER;
//     }

//     Py_ssize_t nr_buffers = PyList_GET_SIZE(Py_buffers);
//     uint32_t nr_dpus;
//     if ((status = dpu_get_nr_dpus(dpu_set, &nr_dpus)) != DPU_OK) {
//         return status;
//     }
//     if (nr_buffers != nr_dpus) {
//         return DPU_ERR_INVALID_MEMORY_TRANSFER;
//     }

//     uint32_t buffer_size = UINT_MAX;
//     switch (dpu_set.kind) {
//         case DPU_SET_RANKS: {
//             uint32_t buffer_idx = 0;
//             for (uint32_t each_rank = 0; each_rank < dpu_set.list.nr_ranks; ++each_rank) {
//                 struct dpu_rank_t *rank = dpu_set.list.ranks[each_rank];
//                 uint8_t nr_cis = rank->description->hw.topology.nr_of_control_interfaces;
//                 uint8_t nr_dpus_per_ci = rank->description->hw.topology.nr_of_dpus_per_control_interface;

//                 for (uint8_t each_ci = 0; each_ci < nr_cis; ++each_ci) {
//                     for (uint8_t each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
//                         struct dpu_t *dpu = DPU_GET_UNSAFE(rank, each_ci, each_dpu);

//                         if (!dpu_is_enabled(dpu)) {
//                             continue;
//                         }

//                         dpu_error_t buffer_status;

//                         void *buff_ptr;
//                         uint32_t buff_size;
//                         if ((buffer_status = Py_get_buffer_at_index(Py_buffers, buffer_idx++, &buff_ptr, &buff_size)) != DPU_OK) {
//                             status = buffer_status;
//                             continue;
//                         }
//                         if (buff_size != UINT_MAX && buffer_size != UINT_MAX && buffer_size != buff_size) {
//                             return DPU_ERR_INVALID_MEMORY_TRANSFER;
//                         } else if (buff_size != UINT_MAX && buffer_size == UINT_MAX) {
//                             buffer_size = buff_size;
//                         }

//                         dpu_transfer_matrix_add_dpu(dpu, dpu_get_transfer_matrix(rank), buff_ptr);
//                     }
//                 }
//             }

//             break;
//         }
//         case DPU_SET_DPU: {
//             struct dpu_t *dpu = dpu_set.dpu;

//             if (!dpu_is_enabled(dpu)) {
//                 return DPU_ERR_DPU_DISABLED;
//             }

//             void *buff_ptr;
//             if ((status = Py_get_buffer_at_index(Py_buffers, 0, &buff_ptr, &buffer_size)) != DPU_OK) {
//                 return status;
//             }

//             dpu_transfer_matrix_add_dpu(dpu, dpu_get_transfer_matrix(dpu_get_rank(dpu)), buff_ptr);

//             break;
//         }
//         default:
//             return DPU_ERR_INTERNAL;
//     }
//     *Py_buffer_size = buffer_size;

//     return status;
// }

// __API_SYMBOL__ PyObject *
// Py_dpu_get_symbol_names(struct dpu_program_t *program)
// {
//     uint32_t nr_symbols = program->symbols->nr_symbols;
//     PyObject *list = PyList_New(nr_symbols);
//     for (uint32_t each_symbol = 0; each_symbol < nr_symbols; ++each_symbol) {
//         PyList_SetItem(list, each_symbol, PyUnicode_FromString(program->symbols->map[each_symbol].name));
//     }
//     return list;
// }
