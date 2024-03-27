#
# Copyright (c) 2014-2019 - UPMEM
#

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR dpu)

if (NOT DEFINED UPMEM_HOME)
    if ("$ENV{UPMEM_HOME}" STREQUAL "")
        set (UPMEM_HOME "/usr")
    else()
        set(UPMEM_HOME $ENV{UPMEM_HOME})
    endif()
endif()

set(CMAKE_SYSROOT ${UPMEM_HOME})

set(tools ${UPMEM_HOME}/bin)

set(CMAKE_C_COMPILER ${tools}/dpu-upmem-dpurte-clang)
set(CMAKE_ASM_COMPILER ${tools}/dpu-upmem-dpurte-clang)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_COMPILER_FORCED 1)
