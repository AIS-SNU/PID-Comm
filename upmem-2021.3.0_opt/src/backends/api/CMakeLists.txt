# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0048 NEW)

find_package(JNI REQUIRED)
find_package(Threads REQUIRED)
find_package(LibElf REQUIRED)
find_package(Python3 COMPONENTS Development REQUIRED)
find_package(LibNuma REQUIRED)

set( JAVA_HEADERS "${JAVA_INCLUDE_PATH}" "${JAVA_INCLUDE_PATH2}" )

set(VC_INCLUDE_DIRECTORIES ../verbose/src)
set(UFI_INCLUDE_DIRECTORIES ../ufi/src ../ufi/include)
set(COMMONS_INCLUDE_DIRECTORIES ../commons/include ../commons/src ../commons/src/properties)
set(INCLUDE_DIRECTORIES include/lowlevel include/api src/include src/api/include ${UFI_INCLUDE_DIRECTORIES} ${COMMONS_INCLUDE_DIRECTORIES} ${VC_INCLUDE_DIRECTORIES} ${LIBELF_INCLUDE_DIR} ${Python3_INCLUDE_DIRS} ${LIBNUMA_INCLUDE_DIR} ${GEN_INCLUDE_DIRECTORY})

set(PROPERTIES_SOURCES
        ../commons/src/properties/dpu_properties.c
        ../commons/src/properties/dpu_properties_loader.c
        )

set(DESCRIPTION_SOURCES
        ../commons/src/types/dpu_description.c
        )

set(COMMONS_SOURCES
        ${PROPERTIES_SOURCES}
        ${DESCRIPTION_SOURCES}
        ../commons/src/dpu_predef_programs.c
        ../commons/src/dpu_package.c
        )

set(SOURCES
        src/api/dpu_thread_job.c
        src/api/dpu_polling.c
        src/api/dpu_loader.c
        src/api/dpu_runner.c
        src/api/dpu_set.c
        src/api/dpu_memory.c
        src/api/dpu_checkpoint.c
        src/api/dpu_python_wrappers.c
        src/dpu_custom.c
        src/dpu_log.c
        src/dpu_elf.c
        src/dpu_error.c
        src/dpu_config.c
        src/dpu_debug.c
        src/dpu_internals.c
        src/dpu_loader.c
        src/dpu_profiler.c
        src/dpu_program.c
        src/dpu_management.c
        src/dpu_memory.c
        src/dpu_rank_handler_allocator.c
        src/dpu_runner.c

        src/runtime_info.c

        ../ufi/src/ufi.c
        ../ufi/src/ufi_bit_config.c
        ../ufi/src/ufi_dma_wavegen_config.c
        ../ufi/src/ufi_ci.c
        ../ufi/src/ufi_config.c
        ../ufi/src/ufi_debug.c
        ../ufi/src/ufi_memory.c
        ../ufi/src/ufi_runner.c
        )

set(ALL_SOURCES ${SOURCES} ${COMMONS_SOURCES})

set(JNI_SOURCES
        src/api/dpu_jni.c
        )

add_library( dpu SHARED ${ALL_SOURCES} )
target_include_directories( dpu PUBLIC ${INCLUDE_DIRECTORIES} )
target_link_libraries( dpu m ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${LIBELF_LIBRARIES} ${Python3_LIBRARIES} ${LIBNUMA_LIBRARIES} dpuverbose )
target_compile_definitions(dpu PUBLIC DPU_TOOLS_VERSION=${UPMEM_VERSION})
set_target_properties(dpu PROPERTIES VERSION ${UPMEM_VERSION})
add_dependencies(dpu gen_files)

add_library(dpujni SHARED ${JNI_SOURCES})
target_include_directories(dpujni PUBLIC ${INCLUDE_DIRECTORIES} ${JAVA_HEADERS})
target_link_libraries(dpujni dpu dpuverbose )
set_target_properties(dpujni PROPERTIES VERSION ${UPMEM_VERSION})

install(
    TARGETS dpu dpujni
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lowlevel/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dpu
    )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/api/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dpu
    )
