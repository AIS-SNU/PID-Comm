# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0048 NEW)

set( COMMONS_INCLUDE_DIRECTORIES ../commons/include ../api/include/api ../api/include/lowlevel)
set (INCLUDES_DIRECTORIES src/ ${COMMONS_INCLUDE_DIRECTORIES})

set ( DPUVERBOSE_LOADER_SOURCES
        src/static_verbose.c
        src/verbose_config.c
        src/verbose_control.c
        src/verbose_profile.c
        )

add_library( dpuverbose SHARED ${DPUVERBOSE_LOADER_SOURCES} )
target_include_directories( dpuverbose PUBLIC ${INCLUDES_DIRECTORIES} )
set_target_properties(dpuverbose PROPERTIES VERSION ${UPMEM_VERSION})

install(
    TARGETS dpuverbose
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
