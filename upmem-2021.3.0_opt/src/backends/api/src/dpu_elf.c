/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <libelf.h>
#include <fcntl.h>
#include <unistd.h>
#include <gelf.h>
#include <memory.h>
#include <dpu_attributes.h>
#include <dpu_elf_internals.h>
#include <dpu_elf.h>
#include <runtime_info.h>

//#define DPU_ELF_VERBOSE
#ifdef DPU_ELF_VERBOSE

#include <stdarg.h>
#include <stdio.h>
#include <_libelf.h>

static void
report_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)vfprintf(stderr, fmt, args);
    fflush(stderr);
    va_end(args);
}

static void
report(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)vfprintf(stdout, fmt, args);
    fflush(stdout);
    va_end(args);
}

static void
report_runtime_info(dpu_elf_runtime_info_t *runtime_info)
{
    report("sys_heap_pointer_reset = 0x%08x (%d)\n",
        runtime_info->sys_heap_pointer_reset.value,
        runtime_info->sys_heap_pointer_reset.exists);
    report("sys_heap_pointer       = 0x%08x (%d)\n", runtime_info->sys_heap_pointer.value, runtime_info->sys_heap_pointer.exists);
    report("sys_wq_table           = 0x%08x (%d)\n", runtime_info->sys_wq_table.value, runtime_info->sys_wq_table.exists);
    report("sys_end                = 0x%08x (%d)\n", runtime_info->sys_end.value, runtime_info->sys_end.exists);
    report("mcount                 = 0x%08x (%d)\n", runtime_info->mcount.value, runtime_info->mcount.exists);
    report("ret_mcount             = 0x%08x (%d)\n", runtime_info->ret_mcount.value, runtime_info->ret_mcount.exists);
    report("thread_profiling       = 0x%08x (%d)\n", runtime_info->thread_profiling.value, runtime_info->thread_profiling.exists);
    report("nr_threads             = 0x%08x (%d)\n", runtime_info->nr_threads.value, runtime_info->nr_threads.exists);
    report("perfcounter_end_value  = 0x%08x (%d)\n",
        runtime_info->perfcounter_end_value.value,
        runtime_info->perfcounter_end_value.exists);
}

#else
#define report_error(...)
#define report(...)
#define report_runtime_info(...)
#endif

static bool
read_and_validate_header(elf_fd info)
{
    GElf_Ehdr ehdr;
    int class;
    char *ident;

    if (gelf_getehdr(info->elf, &ehdr) == NULL) {
        report_error("file '%s' does not contain a valid ELF header: no header found\n", path);
        return false;
    }

    class = gelf_getclass(info->elf);
    if (class != ELFCLASS32) {
        report_error("file '%s' does not contain a valid ELF header: class is not ELF32\n", path);
        return false;
    }

    ident = elf_getident(info->elf, NULL);
    if (ident == NULL) {
        report_error("file '%s' does not contain a valid ELF header: no identity found\n", path);
        return false;
    }

    // Let's verify that the machine description and other information comply with what we
    // expect...
    if (ehdr.e_type != ET_EXEC && ehdr.e_type != ET_CORE) {
        report_error("file '%s' is not an executable object file\n", path);
        return false;
    }

    if (ehdr.e_machine != EM_DPU) {
        report_error("file '%s' is not targeting DPUs\n", path);
        return false;
    }

    if (ehdr.e_version != EV_CURRENT) {
        report_error("file '%s' is not in the expected version\n", path);
        return false;
    }

    if (elf_getshdrnum(info->elf, &(info->shnum)) != 0) {
        report_error("file '%s' is corrupted: shnum not found\n", path);
        return false;
    }

    if (elf_getshdrstrndx(info->elf, &(info->shstrndx)) != 0) {
        report_error("file '%s' is corrupted: shstrndx not found\n", path);
        return false;
    }

    if (elf_getphdrnum(info->elf, &(info->phnum)) != 0) {
        report_error("file '%s' is corrupted: phnum not found\n", path);
        return false;
    }

    report("shnum    = %d\n", (int)(info->shnum));
    report("shstrndx = %d\n", (int)(info->shstrndx));
    report("phnum    = %d\n", (int)(info->phnum));
    return true;
}

static dpu_error_t
get_section_name(elf_fd info, Elf_Scn *scn, char **name)
{
    GElf_Shdr shdr;
    if (gelf_getshdr(scn, &shdr) != &shdr) {
        report_error("file corrupted: could not get section header\n");
        return DPU_ERR_ELF_INVALID_FILE;
    }

    if ((*name = elf_strptr(info->elf, info->shstrndx, shdr.sh_name)) == NULL) {
        report_error("file is corrupted: could not get section name\n");
        return DPU_ERR_ELF_INVALID_FILE;
    }

    return DPU_OK;
}

static dpu_error_t
locate_index_of_section_called(const char *name, elf_fd info, unsigned int *index)
{
    Elf_Scn *scn = NULL;
    unsigned int section_count = 0;

    while ((scn = elf_nextscn(info->elf, scn)) != NULL) {
        char *section_name;
        dpu_error_t err;

        err = get_section_name(info, scn, &section_name);
        if (err != DPU_OK)
            return err;

        section_count++;
        report("section #%u = '%s'\n", section_count, section_name);

        if (strcmp(section_name, name) == 0) {
            *index = section_count;
            return DPU_OK;
        }
    }

    // Browsed all the sections without matching the expected name.
    return DPU_ERR_ELF_NO_SUCH_SECTION;
}

static dpu_error_t
read_symbol_value(GElf_Sym symbol, uint32_t *value, uint32_t *size)
{
    *value = (uint32_t)symbol.st_value;
    *size = (uint32_t)symbol.st_size;
    return DPU_OK;
}

static dpu_error_t
setup_symbols_map(elf_fd info)
{
    dpu_error_t err = DPU_OK;

    Elf_Scn *symtab_scn = elf_getscn(info->elf, info->symtab_index);
    Elf_Data *sym_data;
    if ((sym_data = elf_getdata(symtab_scn, NULL)) == NULL) {
        report_error("file corrupted: could not read symbols\n");
        return DPU_ERR_ELF_INVALID_FILE;
    }

    info->symbol_maps = (dpu_elf_symbols_t *)calloc(info->shnum + 1, sizeof(dpu_elf_symbols_t)); // + 1 for ABS section
    if (info->symbol_maps == NULL) {
        report_error("could not allocate more memory!\n");
        err = DPU_ERR_SYSTEM;
        goto end;
    }

    unsigned int each_section;
    for (each_section = 0; each_section < info->shnum; each_section++) {
        info->symbol_maps[each_section].nr_symbols = 0;
        info->symbol_maps[each_section].map = NULL;
    }

    unsigned int each_symbol;
    GElf_Sym current_symbol;

    for (each_symbol = 0; gelf_getsym(sym_data, each_symbol, &current_symbol) == &current_symbol; each_symbol++) {
        unsigned int section_index = (unsigned int)current_symbol.st_shndx;
        Elf_Scn *section_scn = elf_getscn(info->elf, section_index);
        char *section_name = NULL;
        if (section_index == SHN_ABS) {
            section_index = info->shnum;
            section_name = "ABS";
        } else if (section_index >= info->shnum) {
            continue;
        } else {
            err = get_section_name(info, section_scn, &section_name);
            if (err != DPU_OK) {
                return err;
            }
        }
        char *symbol_name = elf_strptr(info->elf, (size_t)info->strtab_index, current_symbol.st_name);

        // May have irrelevant information... No need to record.
        if ((symbol_name == NULL) || (section_name == NULL) || (strlen(symbol_name) == 0) || (strlen(section_name) == 0))
            continue;

        report("symbol #%u in section #%u - symbol name='%s' - section name='%s'\n",
            each_symbol,
            section_index,
            symbol_name,
            section_name);
        uint32_t symbol_value, symbol_size;

        err = read_symbol_value(current_symbol, &symbol_value, &symbol_size);
        if (err != DPU_OK) {
            return err;
        }

        dpu_elf_symbols_t *map = &(info->symbol_maps[section_index]);
        map->nr_symbols++;
        map->map = (dpu_elf_symbol_t *)realloc(map->map, map->nr_symbols * sizeof(dpu_elf_symbol_t));
        if (map->map == NULL) {
            report_error("could not allocate more memory!\n");
            err = DPU_ERR_SYSTEM;
            goto end;
        }
        map->map[map->nr_symbols - 1].name = symbol_name;
        map->map[map->nr_symbols - 1].size = symbol_size;
        map->map[map->nr_symbols - 1].value = symbol_value;
    }

end:
    return err;
}

static void
clear_elf_fd(elf_fd info)
{
    info->elf = NULL;
    info->fd = -1;
    info->shnum = 0;
    info->symtab_index = (unsigned int)(-1);
    info->strtab_index = (unsigned int)(-1);
    info->symbol_maps = NULL;
    info->filename = NULL;
}

static dpu_error_t
setup_elf_info(const char *path, elf_fd info, bool is_based_on_file)
{
    dpu_error_t err;
    info->ek = elf_kind(info->elf);
    if (info->ek != ELF_K_ELF) {
        return DPU_ERR_ELF_INVALID_FILE;
    }

    if (!read_and_validate_header(info)) {
        return DPU_ERR_ELF_INVALID_FILE;
    }

    if (is_based_on_file) {
        info->filename = strdup(path);
    }

    report("locating '.symtab'\n");
    err = locate_index_of_section_called(".symtab", info, &(info->symtab_index));
    if (err != DPU_OK)
        return err;

    report("symtab index = %u\n", info->symtab_index);

    report("locating '.strtab'\n");
    err = locate_index_of_section_called(".strtab", info, &(info->strtab_index));
    if (err != DPU_OK)
        return err;

    report("strtab index = %u\n", info->strtab_index);

    report("loading symbol map\n");
    err = setup_symbols_map(info);
    if (err != DPU_OK)
        return err;

    return err;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_open(const char *path, dpu_elf_file_t *file)
{
    elf_fd info = (elf_fd)malloc(sizeof(struct _elf_fd));
    dpu_error_t err;

    report("dpu_elf_open '%s'\n", path);

    if (elf_version(EV_CURRENT) == EV_NONE) {
        err = DPU_ERR_INTERNAL;
        goto err_internal;
    }

    if (info == NULL) {
        err = DPU_ERR_SYSTEM;
        goto err_internal;
    }

    clear_elf_fd(info);

    report("preparing ELF structure\n");
    info->fd = open(path, O_RDONLY, 0);
    if (info->fd == -1) {
        err = DPU_ERR_ELF_NO_SUCH_FILE;
        goto err_invalid_elf;
    }

    info->elf = elf_begin(info->fd, ELF_C_READ, NULL);
    if (info->elf == NULL) {
        err = DPU_ERR_ELF_INVALID_FILE;
        goto err_invalid_elf;
    }

    // elf_rawfile will mmap the whole elf file.
    // This will simplify (ie. remove) the needed memory allocations when accessing different parts of the elf file.
    if (elf_rawfile(info->elf, NULL) == NULL) {
        err = DPU_ERR_ELF_INVALID_FILE;
        goto err_invalid_elf;
    }

    err = setup_elf_info(path, info, true);
    if (err != DPU_OK) {
        goto err_invalid_elf;
    }

    *file = (dpu_elf_file_t)info;
    return err;

err_invalid_elf:
    dpu_elf_close((dpu_elf_file_t)info);

err_internal:
    return err;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_map(uint8_t *memory, unsigned int size, dpu_elf_file_t *file, const char *filename_or_null)
{
    elf_fd info = (elf_fd)malloc(sizeof(struct _elf_fd));
    dpu_error_t err;

    report("dpu_elf_open '<memory>'\n");

    if (elf_version(EV_CURRENT) == EV_NONE) {
        err = DPU_ERR_INTERNAL;
        goto err_internal;
    }

    if (info == NULL) {
        err = DPU_ERR_SYSTEM;
        goto err_internal;
    }

    clear_elf_fd(info);

    report("preparing ELF structure\n");
    info->elf = elf_memory((char *)memory, (size_t)size);
    if (info->elf == NULL) {
        err = DPU_ERR_ELF_INVALID_FILE;
        goto err_invalid_elf;
    }

    if (filename_or_null == NULL) {
        err = setup_elf_info("<memory>", info, false);
    } else {
        err = setup_elf_info(filename_or_null, info, true);
    }
    if (err != DPU_OK) {
        goto err_invalid_elf;
    }

    *file = (dpu_elf_file_t)info;
    return err;

err_invalid_elf:
    dpu_elf_close((dpu_elf_file_t)info);

err_internal:
    return err;
}

__API_SYMBOL__ void
dpu_elf_free_symbols(dpu_elf_symbols_t *symbols)
{
    if (symbols->map != NULL) {
        // Note: the symbol map is only referencing strings. No need to free them.
        free(symbols->map);
    }
    symbols->map = NULL;
    symbols->nr_symbols = 0;
}

__API_SYMBOL__ void
dpu_elf_close(dpu_elf_file_t file)
{
    elf_fd info = (elf_fd)file;
    if (info->elf != NULL) {
        (void)elf_end(info->elf);
    }
    if (info->fd != -1) {
        (void)close(info->fd);
    }
    if (info->filename != NULL) {
        free(info->filename);
    }
    if (info->symbol_maps != NULL) {
        unsigned int each_map;
        for (each_map = 0; each_map < info->shnum + 1; each_map++) { // + 1 for ABS section
            if (info->symbol_maps[each_map].map != NULL) {
                dpu_elf_free_symbols(&(info->symbol_maps[each_map]));
            }
        }
        free(info->symbol_maps);
    }
    clear_elf_fd(info);
    free(info);
}

__API_SYMBOL__ dpu_error_t
dpu_elf_load_section(dpu_elf_file_t file, const char *name, unsigned int *size, uint8_t **buffer)
{
    elf_fd info = (elf_fd)file;
    unsigned int section_index;

    dpu_error_t err = locate_index_of_section_called(name, info, &section_index);
    if (err != DPU_OK) {
        return err;
    }

    Elf_Scn *section_scn = elf_getscn(info->elf, section_index);
    Elf_Data *section_data;
    if ((section_data = elf_getdata(section_scn, NULL)) == NULL) {
        report_error("file is corrupted: could not load section '%s' data\n", name);
        return DPU_ERR_ELF_INVALID_FILE;
    }

    report("\tdata loaded: at @%p size=%ld align=%ld off=%ld\n",
        section_data->d_buf,
        section_data->d_size,
        section_data->d_align,
        section_data->d_off);

    *size = (unsigned int)(section_data->d_size);
    *buffer = (uint8_t *)(section_data->d_buf) + section_data->d_off;

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_load_symbols(dpu_elf_file_t file, const char *name, dpu_elf_symbols_t **symbols)
{
    elf_fd info = (elf_fd)file;
    unsigned int section_index;
    dpu_error_t err;
    err = locate_index_of_section_called(name, info, &section_index);
    if (err == DPU_OK) {
        *symbols = &info->symbol_maps[section_index];
    }
    return err;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_get_sections(dpu_elf_file_t file, unsigned int *nr_sections, char ***sections)
{
    char **list = NULL;
    elf_fd info = (elf_fd)file;
    unsigned int each_section;
    dpu_error_t err;

    *nr_sections = 0;

    for (each_section = 0; each_section < info->shnum; each_section++) {
        Elf_Scn *section_scn = elf_getscn(info->elf, each_section);
        char *section_name;
        err = get_section_name(info, section_scn, &section_name);
        if (err != DPU_OK) {
            if (list != NULL)
                free(list);
            return err;
        }
        list = (char **)realloc(list, (each_section + 1) * sizeof(char *));
        if (list == NULL) {
            report_error("could not allocate memory to store section names\n");
            return DPU_ERR_SYSTEM;
        }
        list[each_section] = section_name;
    }

    *sections = list;
    *nr_sections = (unsigned int)info->shnum;
    return DPU_OK;
}

__API_SYMBOL__ void
dpu_elf_get_runtime_info(dpu_elf_file_t file, dpu_elf_runtime_info_t *runtime_info)
{
    elf_fd info = (elf_fd)file;

    reset_runtime_info(runtime_info);

    // For each section, for each symbol, update (or not) the runtime information with the symbol information.
    unsigned int each_section;
    for (each_section = 0; each_section < info->shnum + 1; each_section++) { // + 1 for ABS section
        dpu_elf_symbols_t *symbols = &info->symbol_maps[each_section];
        unsigned int each_symbol;
        for (each_symbol = 0; each_symbol < symbols->nr_symbols; each_symbol++) {
            dpu_elf_symbol_t *symbol = &symbols->map[each_symbol];
            register_runtime_info_if_needed_with(symbol->name, symbol->value, symbol->size, runtime_info);
        }
    }
}

__API_SYMBOL__ void
free_runtime_info(dpu_elf_runtime_info_t *runtime_info)
{
    if (runtime_info->mutex_info.map != NULL) {
        free(runtime_info->mutex_info.map);
    }
    if (runtime_info->semaphore_info.map != NULL) {
        free(runtime_info->semaphore_info.map);
    }
    if (runtime_info->barrier_info.map != NULL) {
        free(runtime_info->barrier_info.map);
    }
}

static uint32_t
add_section_in_string_table(const char *section_name, char **string_table, size_t *string_table_size)
{
    uint32_t section_name_size = strlen(section_name);
    uint32_t offset_in_string_table = *string_table_size;
    size_t new_size = *string_table_size + section_name_size + 1;
    char *new_table = realloc(*string_table, new_size + 1);

    strcpy(&new_table[offset_in_string_table], section_name);
    new_table[new_size - 1] = '\0';
    new_table[new_size] = '\0';

    *string_table_size = new_size;
    *string_table = new_table;

    return offset_in_string_table;
}

typedef enum {
    CORE_DUMP_DATA_SECTION,
    CORE_DUMP_TEXT_SECTION,
    CORE_DUMP_REGS_SECTION,
    CORE_DUMP_MRAM_SECTION,
    CORE_DUMP_NB_SECTION,
} core_dump_section_e;

static const char *core_dump_section_to_string[CORE_DUMP_NB_SECTION] = {
    [CORE_DUMP_DATA_SECTION] = ".data",
    [CORE_DUMP_TEXT_SECTION] = ".text",
    [CORE_DUMP_REGS_SECTION] = ".regs",
    [CORE_DUMP_MRAM_SECTION] = ".mram",
};
#define SHSTRTAB_SECTION_NAME ".shstrtab"
#define SYMTAB_SECTION_NAME ".symtab"
#define STRTAB_SECTION_NAME ".strtab"

static bool
section_can_be_copied(const char *section_name)
{
    if (!strncmp(section_name, SHSTRTAB_SECTION_NAME, strlen(SHSTRTAB_SECTION_NAME)))
        return false;
    for (unsigned int each_section = 0; each_section < CORE_DUMP_NB_SECTION; each_section++) {
        const char *core_dump_section_name = core_dump_section_to_string[each_section];
        if (!strncmp(section_name, core_dump_section_name, strlen(core_dump_section_name)))
            return false;
    }
    return true;
}

static dpu_error_t
dpu_elf_copy_section(Elf_Scn *scn_out,
    Elf_Scn *scn_in,
    const char *section_name,
    char **string_table,
    size_t *string_table_size,
    Elf32_Shdr **shdr_symtab,
    uint32_t *strtab_idx)
{
    Elf_Data *data_in, *data_out;
    GElf_Shdr shdr_in;
    Elf32_Shdr *shdr_out;
    size_t each_shdr;

    if (gelf_getshdr(scn_in, &shdr_in) != &shdr_in)
        return DPU_ERR_ELF_INVALID_FILE;

    data_in = NULL;
    each_shdr = 0;
    while (each_shdr < shdr_in.sh_size && (data_in = elf_getdata(scn_in, data_in)) != NULL) {
        if ((data_out = elf_newdata(scn_out)) == NULL)
            return DPU_ERR_ELF_INVALID_FILE;
        *data_out = *data_in;
        each_shdr++;
    }

    shdr_out = elf32_getshdr(scn_out);
    shdr_out->sh_name = add_section_in_string_table(section_name, string_table, string_table_size);
    shdr_out->sh_type = shdr_in.sh_type;
    shdr_out->sh_flags = shdr_in.sh_flags;
    shdr_out->sh_entsize = shdr_in.sh_entsize;

    if (!strncmp(section_name, SYMTAB_SECTION_NAME, strlen(SYMTAB_SECTION_NAME)))
        *shdr_symtab = shdr_out;
    if (!strncmp(section_name, STRTAB_SECTION_NAME, strlen(STRTAB_SECTION_NAME)))
        *strtab_idx = elf_ndxscn(scn_out);

    return DPU_OK;
}

__API_SYMBOL__ dpu_error_t
dpu_elf_create_core_dump(const char *exe_path,
    const char *core_file_path,
    uint8_t *wram,
    uint8_t *mram,
    uint8_t *iram,
    uint8_t *context,
    uint32_t wram_size,
    uint32_t mram_size,
    uint32_t iram_size,
    uint32_t context_size)
{
    dpu_error_t elf_status;
    // INITIALIZE STRING_TABLE STUFF
    size_t string_table_size = 1;
    char *string_table = (char *)malloc(string_table_size + 1);
    string_table[0] = '\0';

    // CHECK ELF_VERSION
    if (elf_version(EV_CURRENT) == EV_NONE) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_free_string_table;
    }

    // OPEN OUTPUT ELF FILE AND STRUCTURE
    int fd = open(core_file_path, O_WRONLY | O_CREAT, 444);
    if (fd < 0) {
        elf_status = DPU_ERR_ELF_NO_SUCH_FILE;
        goto core_dump_free_string_table;
    }

    Elf *elf_output_fd = elf_begin(fd, ELF_C_WRITE, NULL);
    if (elf_output_fd == NULL) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_close_fd;
    }

    // CREATE AND INIT EHDR
    Elf32_Ehdr *ehdr = elf32_newehdr(elf_output_fd);
    if (ehdr == NULL) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_end_elf_output_fd;
    }

    ehdr->e_ident[EI_DATA] = 0; // endianness = LITTLE ENDIAN
    ehdr->e_machine = EM_DPU;
    ehdr->e_type = ET_CORE;
    ehdr->e_version = EV_CURRENT;

    // CREATE PHDR
    Elf32_Phdr *phdr = elf32_newphdr(elf_output_fd, CORE_DUMP_NB_SECTION);
    if (phdr == NULL) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_end_elf_output_fd;
    }

    // CREATE CORE DUMP SPECIFIC SECTIONS
    Elf32_Shdr *shdr[CORE_DUMP_NB_SECTION];
    uint32_t section_addr[CORE_DUMP_NB_SECTION] = {
        [CORE_DUMP_TEXT_SECTION] = 0x80000000,
        [CORE_DUMP_DATA_SECTION] = 0x00000000,
        [CORE_DUMP_MRAM_SECTION] = 0x08000000,
        [CORE_DUMP_REGS_SECTION] = 0xa0000000,
    };
    uint8_t *buffer[CORE_DUMP_NB_SECTION] = {
        [CORE_DUMP_TEXT_SECTION] = iram,
        [CORE_DUMP_DATA_SECTION] = wram,
        [CORE_DUMP_MRAM_SECTION] = mram,
        [CORE_DUMP_REGS_SECTION] = context,
    };
    uint32_t buffer_size[CORE_DUMP_NB_SECTION] = {
        [CORE_DUMP_TEXT_SECTION] = iram_size,
        [CORE_DUMP_DATA_SECTION] = wram_size,
        [CORE_DUMP_MRAM_SECTION] = mram_size,
        [CORE_DUMP_REGS_SECTION] = context_size,
    };
    Elf32_Word flags[CORE_DUMP_NB_SECTION] = {
        [CORE_DUMP_TEXT_SECTION] = SHF_ALLOC | SHF_EXECINSTR,
        [CORE_DUMP_DATA_SECTION] = SHF_ALLOC | SHF_WRITE,
        [CORE_DUMP_MRAM_SECTION] = SHF_ALLOC | SHF_WRITE,
        [CORE_DUMP_REGS_SECTION] = SHF_ALLOC | SHF_WRITE,
    };
    for (unsigned int each_section = 0; each_section < CORE_DUMP_NB_SECTION; each_section++) {
        Elf_Scn *scn = elf_newscn(elf_output_fd);
        if (scn == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_end_elf_output_fd;
        }
        Elf_Data *data = elf_newdata(scn);
        if (data == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_end_elf_output_fd;
        }
        shdr[each_section] = elf32_getshdr(scn);
        if (shdr[each_section] == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_end_elf_output_fd;
        }

        data->d_buf = buffer[each_section];
        data->d_size = buffer_size[each_section];
        data->d_align = 4;
        data->d_type = ELF_T_WORD;
        data->d_off = 0LL;
        data->d_version = EV_CURRENT;

        shdr[each_section]->sh_name
            = add_section_in_string_table(core_dump_section_to_string[each_section], &string_table, &string_table_size);
        shdr[each_section]->sh_addr = section_addr[each_section];
        shdr[each_section]->sh_type = SHT_PROGBITS;
        shdr[each_section]->sh_flags = flags[each_section];

        phdr[each_section].p_type = PT_LOAD;
        phdr[each_section].p_vaddr = section_addr[each_section];
        phdr[each_section].p_paddr = section_addr[each_section];
        phdr[each_section].p_memsz = buffer_size[each_section];
        phdr[each_section].p_filesz = buffer_size[each_section];
    }

    // OPEN INITAL ELF
    dpu_elf_file_t exe_elf;
    elf_status = dpu_elf_open(exe_path, &exe_elf);
    if (elf_status != DPU_OK) {
        goto core_dump_end_elf_output_fd;
    }
    elf_fd elf_info = (elf_fd)exe_elf;

    // COPY NEEDED SECTION IN OUTPUT ELF
    Elf32_Shdr *shdr_symtab = NULL;
    uint32_t strtab_idx = 0;
    for (unsigned int each_section = 1; each_section < elf_info->shnum; each_section++) {
        Elf_Scn *scn_in = elf_getscn(elf_info->elf, each_section);
        char *section_name;
        elf_status = get_section_name(elf_info, scn_in, &section_name);
        if (elf_status != DPU_OK) {
            goto core_dump_close_exe_elf;
        }
        if (!section_can_be_copied(section_name)) {
            continue;
        }

        Elf_Scn *scn_out = elf_newscn(elf_output_fd);
        elf_status
            = dpu_elf_copy_section(scn_out, scn_in, section_name, &string_table, &string_table_size, &shdr_symtab, &strtab_idx);
        if (elf_status != DPU_OK) {
            goto core_dump_close_exe_elf;
        }
    }
    shdr_symtab->sh_link = strtab_idx;

    // ADD SHSTRTAB SECTION IN OUTPUT ELF
    {
        Elf_Scn *string_table_scn = elf_newscn(elf_output_fd);
        if (string_table_scn == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_close_exe_elf;
        }
        Elf_Data *data = elf_newdata(string_table_scn);
        if (data == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_close_exe_elf;
        }
        Elf32_Shdr *shdr = elf32_getshdr(string_table_scn);
        if (shdr == NULL) {
            elf_status = DPU_ERR_INTERNAL;
            goto core_dump_close_exe_elf;
        }

        shdr->sh_name = add_section_in_string_table(SHSTRTAB_SECTION_NAME, &string_table, &string_table_size);

        data->d_buf = string_table;
        data->d_size = string_table_size;

        ehdr->e_shstrndx = elf_ndxscn(string_table_scn);
        shdr->sh_type = SHT_STRTAB;
        shdr->sh_flags = SHF_STRINGS | SHF_ALLOC;
        shdr->sh_entsize = 0;
    }

    // UPDATE ELF TO UPDATE SHDR[]->SH_OFFSET
    if (elf_update(elf_output_fd, ELF_C_NULL) < 0) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_close_exe_elf;
    }

    // UPDATE PHDR WITH UPDATED SHDR
    for (unsigned int each_section = 0; each_section < CORE_DUMP_NB_SECTION; each_section++) {
        phdr[each_section].p_offset = shdr[each_section]->sh_offset;
    }

    // UPDATE AND WRITE IN FILE OUTPUT ELF
    if (elf_update(elf_output_fd, ELF_C_WRITE) < 0) {
        elf_status = DPU_ERR_INTERNAL;
        goto core_dump_close_exe_elf;
    }

core_dump_close_exe_elf:
    dpu_elf_close(exe_elf);
core_dump_end_elf_output_fd:
    elf_end(elf_output_fd);
core_dump_close_fd:
    close(fd);
core_dump_free_string_table:
    free(string_table);
    return elf_status;
}
