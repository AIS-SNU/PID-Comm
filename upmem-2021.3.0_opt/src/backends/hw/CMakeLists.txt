# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0048 NEW)

find_package(SSE)

if(C_SSE2_COMPILES)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SSE2")
endif()

if(C_AVX512F_COMPILES AND C_AVX512BW_COMPILES AND C_CLFLUSHOPT_COMPILES)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_AVX512F_FLAGS} ${C_AVX512BW_FLAGS} ${C_CLFLUSHOPT_FLAGS} -O3 -g")
else()
    if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^x86_64" )
        message(FATAL_ERROR "The host toolchain does not support avx512f/avx512bw or clflushopt: x86 mappings can't work without those extensions.")
    endif()
endif()

set( INCLUDE_DIRECTORIES
        ../api/include/lowlevel
        ../api/include/api
        ../api/src/include
        ../commons/src/properties
        ../commons/src/pcb
        ../commons/src
        ../commons/include
        ../verbose/src
        src/commons
        src
        )

set( COMMONS_SOURCES
        ../commons/src/properties/dpu_properties.c
        ../commons/src/dpu_chip_description.c
        )

set ( MDD_COMMONS_SOURCES
        src/commons/dpu_rank_ioctl.h
        src/commons/dpu_region_constants.h
        src/commons/dpu_region_address_translation.h
        src/commons/dpu_module_compatibility.h
        )

set ( HW_SOURCES ${COMMONS_SOURCES} ${MDD_COMMON_SOURCES}
        src/rank/hw_dpu_rank.c
        src/rank/hw_dpu_sysfs.c
        src/rank/hw_dpu_sysfs.h
        src/rank/fpga_ila.c
        src/rank/dpu_fpga_ila.h
        src/mappings/fpga_aws/fpga_aws_translation.c
        )

if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^ppc64le" )
list ( APPEND HW_SOURCES src/mappings/power9/power9_translation.c )
endif ()

if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^x86_64" )
list ( APPEND HW_SOURCES src/mappings/xeon_sp/xeon_sp_translation.c )
list ( APPEND HW_SOURCES src/mappings/xeon_sp/pid_comm.c )
endif ()

find_package(LibUdev REQUIRED)
find_package(LibNuma REQUIRED)

add_library( dpuhw SHARED ${HW_SOURCES} )
target_include_directories( dpuhw PUBLIC ${INCLUDE_DIRECTORIES} ${HW_DIR} ${LIBUDEV_INCLUDE_DIR} ${LIBNUMA_INCLUDE_DIR} ${GEN_INCLUDE_DIRECTORY})
target_link_libraries( dpuhw dpuverbose dpuvpd ${LIBUDEV_LIBRARIES} ${LIBNUMA_LIBRARIES} )
set_target_properties(dpuhw PROPERTIES VERSION ${UPMEM_VERSION})
add_dependencies(dpuhw gen_files)

install(
    TARGETS dpuhw
    LIBRARY
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
