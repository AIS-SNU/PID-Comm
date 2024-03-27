# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

find_program(PYGEN ctypesgen)
find_program(PYFORMAT autopep8)
find_program(PYTHON3_EXEC python3)
execute_process(
    COMMAND ${PYTHON3_EXEC}
        -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_lib(True, False, ''))"
    OUTPUT_VARIABLE PYTHON_RELATIVE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

set(C_API_LIB libdpu.so)
set(C_VPD_LIB libdpuvpd.so)
get_filename_component(BACKENDS_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(C_API_DIR ${BACKENDS_DIR}/api/include/api)
set(C_LLAPI_DIR  ${BACKENDS_DIR}/api/include/lowlevel)
set(C_VPD_DIR  ${BACKENDS_DIR}/vpd/src/)
STRING(REPLACE "/" "\\/" BACKENDS_DIR_SED "${BACKENDS_DIR}")

set(C_API_HEADERS
    ${C_API_DIR}/dpu.h
    ${C_API_DIR}/dpu_checkpoint.h
    ${C_API_DIR}/dpu_error.h
    ${C_API_DIR}/dpu_types.h
    )

set(C_VPD_HEADERS
    ${C_LLAPI_DIR}/dpu_vpd.h
    ${C_LLAPI_DIR}/dpu_vpd_structures.h
    ${C_VPD_DIR}/dpu_flash_partition.h
    )

set(PYFORMAT_ARGS --in-place --aggressive --aggressive)

add_custom_target(
        python-generate
        COMMAND ${PYGEN}
            ${C_API_HEADERS}
            -o ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
            -I ${C_API_DIR}
            -I ${C_LLAPI_DIR}
            -l ${C_API_LIB}
            --strip-build-path=${C_API_DIR}
            --no-undefs
            --header-template=${CMAKE_CURRENT_SOURCE_DIR}/utils/ffi_header.txt
            --insert-file=${CMAKE_CURRENT_SOURCE_DIR}/utils/ffi_footer.txt
        COMMAND sed -i \"s/add_library_search_dirs\(\\[\\]\)/add_library_search_dirs([os.path.dirname(os.path.dirname(os.path.dirname((os.path.dirname(__file__)))))])/\" ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
        COMMAND sed -i \"s/${BACKENDS_DIR_SED}/<backends>/\" ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
        COMMAND sed -i \"s/set\(\)/[]/\" ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
        COMMAND sed -i \"s/cache_i.add/cache_i.append/\" ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
        COMMAND ${PYFORMAT}
            ${CMAKE_CURRENT_SOURCE_DIR}/dpu/ffi.py
            ${PYFORMAT_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )

add_custom_target(
        python-generate-vpd
        COMMAND ${PYGEN}
            ${C_VPD_HEADERS}
            -o ${CMAKE_CURRENT_SOURCE_DIR}/dpu/vpd/vpd.py
            -I ${C_API_DIR}
            -I ${C_LLAPI_DIR}
            -l ${C_VPD_LIB}
            --strip-build-path=${BACKENDS_DIR}
            --no-undefs
            --header-template=${CMAKE_CURRENT_SOURCE_DIR}/utils/ffi_header.txt
        COMMAND ${PYFORMAT}
            ${CMAKE_CURRENT_SOURCE_DIR}/dpu/vpd/vpd.py
            ${PYFORMAT_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )

add_custom_target(
        python-format
        COMMAND ${PYFORMAT}
            dpu/__init__.py
            dpu/compiler.py
            dpu/driver.py
            dpu/vpd/db.py
            dpu/vpd/dimm.py
            tests/*.py
            ${PYFORMAT_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dpu/
    DESTINATION ${PYTHON_RELATIVE_PATH}/dpu
    PATTERN "__pycache__" EXCLUDE
    )
