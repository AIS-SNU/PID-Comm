# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0048 NEW)

enable_testing()

set( COMMONS_INCLUDE_DIRECTORIES ../api/include/lowlevel ../commons/include ../api/include/api)
set (INCLUDES_DIRECTORIES src/ ${COMMONS_INCLUDE_DIRECTORIES})

set ( DPUVPD_SOURCES
        src/dpu_vpd.c
        src/dpu_vpd_container.c
        src/dpu_vpd_decode.c
        src/dpu_vpd_encode.c
        )

add_library( dpuvpd SHARED ${DPUVPD_SOURCES} )
target_link_libraries( dpuvpd dpuverbose )
target_include_directories( dpuvpd PUBLIC ${INCLUDES_DIRECTORIES} )
set_target_properties(dpuvpd PROPERTIES VERSION ${UPMEM_VERSION})

install(
    TARGETS dpuvpd
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

# Hook not to build the test when creating source package
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_executable(VpdDatabaseTest ${DPUVPD_SOURCES} tests/VpdDatabaseTest.c)
    target_link_libraries( VpdDatabaseTest dpuverbose )
    target_include_directories(VpdDatabaseTest PUBLIC ${INCLUDES_DIRECTORIES})
    add_test(NAME VpdDatabaseTest COMMAND VpdDatabaseTest)
endif()