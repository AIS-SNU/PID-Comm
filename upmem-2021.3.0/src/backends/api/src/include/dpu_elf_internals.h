/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_ELF_INTERNALS_H
#define DPU_ELF_INTERNALS_H

#include <libelf.h>
#include <dpu_elf.h>

typedef struct _elf_fd {
    int fd;
    Elf *elf;
    Elf_Kind ek;
    size_t shnum;
    size_t shstrndx;
    size_t phnum;
    unsigned int symtab_index;
    unsigned int strtab_index;
    /* Preload the symbol maps, so that we do not need to browse all the symbols each time. The table is indexed
     * by section number. */
    dpu_elf_symbols_t *symbol_maps;
    char *filename;
} * elf_fd;

#endif // DPU_ELF_INTERNALS_H
