# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0063 NEW)

set( CMAKE_C_VISIBILITY_PRESET hidden )

set( SOURCE_DIRECTORY src/ include/ )
set( COMMONS_INCLUDE_DIRECTORIES ../commons/include/ ../commons/src/properties ../commons/src/pcb ../verbose/src
        ../commons/src ../api/include/lowlevel ../api/include/api ../api/src/include)

set( UFI_SOURCES
        src/ufi_bit_config.c
        src/ufi_dma_wavegen_config.c
        src/ufi.c
        src/ufi_ci.c

        src/ufi_config.c
        src/ufi_debug.c
        src/ufi_memory.c
        src/ufi_runner.c
        )

add_library( dpu-ufi SHARED ${UFI_SOURCES} )
target_include_directories( dpu-ufi PUBLIC ${COMMONS_INCLUDE_DIRECTORIES} ${SOURCE_DIRECTORY} ${GEN_INCLUDE_DIRECTORY})
target_link_libraries( dpu-ufi dpuverbose )
set_target_properties(dpu-ufi PROPERTIES VERSION ${UPMEM_VERSION})
add_dependencies(dpu-ufi gen_files)
# We want to be able to compile this with the Linux kernel rules
target_compile_options(dpu-ufi PRIVATE
        -std=gnu89
        -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Werror-implicit-function-declaration -Wno-format-security
        -Wno-sign-compare -Wno-unused-const-variable
        -Wdeclaration-after-statement -Wno-pointer-sign
        -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time -Werror=incompatible-pointer-types)

install(
    TARGETS dpu-ufi
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dpu
    )

file(GLOB_RECURSE ALL_SOURCE_FILES *.c *.h)
list(SORT ALL_SOURCE_FILES)

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format-pattern "")
set(FIRST_SOURCE_FILE 1)

foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
    string(REGEX MATCHALL "${CMAKE_CURRENT_SOURCE_DIR}(/.*)?/(target|build|CMakeFiles|(cmake-build-.*))" EXCLUDED_FILE_FOUND ${SOURCE_FILE})
    if (NOT ${EXCLUDED_FILE_FOUND} EQUAL -1)
        list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
    else()
        if (FIRST_SOURCE_FILE)
            set(FIRST_SOURCE_FILE 0)
        else()
            file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format-pattern "\n")
        endif()

        STRING(REGEX REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/" "" SRC_WITHOUT_PREFIX ${SOURCE_FILE})
        if (NOT ${SRC_WITHOUT_PREFIX} EQUAL -1)
            file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format-pattern "${SRC_WITHOUT_PREFIX}")
        else()
            file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format-pattern "${SOURCE_FILE}")
        endif ()
    endif ()
endforeach ()

add_custom_target(
        ufi-format
        COMMAND ${CLANG_FORMAT_EXE}
        -style=file
        -i
        ${ALL_SOURCE_FILES}
)
