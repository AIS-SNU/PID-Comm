# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

project(librt C)

set(OBJCOPY llvm-objcopy)
set(CLANGFORMAT clang-format)

if (DEFINED DPU_TOOLCHAIN_DIRECTORY)
    set(OBJCOPY ${DPU_TOOLCHAIN_DIRECTORY}/llvm-objcopy)
    set(CLANGFORMAT ${DPU_TOOLCHAIN_DIRECTORY}/clang-format)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(TIMESTAMP ${CMAKE_C_COMPILER} COMPILER_TIMESTAMP)
set(COMPILER_TIMESTAMP_DEF -DCOMPILER_TIMESTAMP=${COMPILER_TIMESTAMP})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_C_COMPILER})

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_C_STANDARD 11)
set(STRICT_FLAGS -Wall -Wextra -Werror)
set(NOSTDLIB_FLAGS -nostdlib -nostdinc -Wno-incompatible-library-redeclaration)
set(COMPILE_LTO_FLAGS -flto)
set(COMPILE_LTO_THIN_FLAGS -flto=thin)
set(PROFILING_FLAGS -pg)

set(STDLIB_DIR src/stdlib)
set(SYSLIB_DIR src/syslib)
set(UTILS_DIR src/misc)

set(STDLIB_C_SOURCES
        ${STDLIB_DIR}/abort.c
        ${STDLIB_DIR}/assert.h
        ${STDLIB_DIR}/atoi.c
        ${STDLIB_DIR}/atol.c
        ${STDLIB_DIR}/ctype.h
        ${STDLIB_DIR}/errno.c
        ${STDLIB_DIR}/errno.h
        ${STDLIB_DIR}/exit.c
        ${STDLIB_DIR}/inttypes.h
        ${STDLIB_DIR}/iso646.h
        ${STDLIB_DIR}/limits.h
        ${STDLIB_DIR}/memchr.c
        ${STDLIB_DIR}/memcmp.c
        ${STDLIB_DIR}/memcpy.c
        ${STDLIB_DIR}/memmove.c
        ${STDLIB_DIR}/memset.c
        ${STDLIB_DIR}/stdalign.h
        ${STDLIB_DIR}/stdarg.h
        ${STDLIB_DIR}/stdbool.h
        ${STDLIB_DIR}/stddef.h
        ${STDLIB_DIR}/stdint.h
        ${STDLIB_DIR}/stdio.c
        ${STDLIB_DIR}/stdio.h
        ${STDLIB_DIR}/stdlib.h
        ${STDLIB_DIR}/stdnoreturn.h
        ${STDLIB_DIR}/stpcpy.c
        ${STDLIB_DIR}/stpncpy.c
        ${STDLIB_DIR}/strcat.c
        ${STDLIB_DIR}/strchr.c
        ${STDLIB_DIR}/strcmp.c
        ${STDLIB_DIR}/strcpy.c
        ${STDLIB_DIR}/strcspn.c
        ${STDLIB_DIR}/strdup.c
        ${STDLIB_DIR}/strerror.c
        ${STDLIB_DIR}/string.h
        ${STDLIB_DIR}/strlen.c
        ${STDLIB_DIR}/strlwr.c
        ${STDLIB_DIR}/strncat.c
        ${STDLIB_DIR}/strncmp.c
        ${STDLIB_DIR}/strncpy.c
        ${STDLIB_DIR}/strndup.c
        ${STDLIB_DIR}/strnlen.c
        ${STDLIB_DIR}/strpbrk.c
        ${STDLIB_DIR}/strrchr.c
        ${STDLIB_DIR}/strrev.c
        ${STDLIB_DIR}/strsep.c
        ${STDLIB_DIR}/strspn.c
        ${STDLIB_DIR}/strstr.c
        ${STDLIB_DIR}/strtok_r.c
        ${STDLIB_DIR}/strtol.c
        ${STDLIB_DIR}/strupr.c
    )

set(SYSLIB_C_SOURCES
        ${SYSLIB_DIR}/absvdi2.c
        ${SYSLIB_DIR}/absvsi2.c
        ${SYSLIB_DIR}/adddf3.c
        ${SYSLIB_DIR}/addsf3.c
        ${SYSLIB_DIR}/addvdi3.c
        ${SYSLIB_DIR}/addvsi3.c
        ${SYSLIB_DIR}/alloc.c
        ${SYSLIB_DIR}/alloc.h
        ${SYSLIB_DIR}/ashldi3.c
        ${SYSLIB_DIR}/ashrdi3.c
        ${SYSLIB_DIR}/atomic_bit.h
        ${SYSLIB_DIR}/atomics.c
        ${SYSLIB_DIR}/attributes.h
        ${SYSLIB_DIR}/barrier.c
        ${SYSLIB_DIR}/barrier.h
        ${SYSLIB_DIR}/bswapdi2.c
        ${SYSLIB_DIR}/bswapsi2.c
        ${SYSLIB_DIR}/buddy_alloc.c
        ${SYSLIB_DIR}/buddy_alloc.h
        ${SYSLIB_DIR}/buddy_realloc.c
        ${SYSLIB_DIR}/built_ins.h
        ${SYSLIB_DIR}/clzdi2.c
        ${SYSLIB_DIR}/clzsi2.c
        ${SYSLIB_DIR}/cmpdi2.c
        ${SYSLIB_DIR}/comparedf2.c
        ${SYSLIB_DIR}/comparesf2.c
        ${SYSLIB_DIR}/ctzdi2.c
        ${SYSLIB_DIR}/ctzsi2.c
        ${SYSLIB_DIR}/defs.c
        ${SYSLIB_DIR}/defs.h
        ${SYSLIB_DIR}/devprivate.h
        ${SYSLIB_DIR}/div32.c
        ${SYSLIB_DIR}/divdf3.c
        ${SYSLIB_DIR}/divdi3.c
        ${SYSLIB_DIR}/divmodsi4.c
        ${SYSLIB_DIR}/divsf3.c
        ${SYSLIB_DIR}/divsi3.c
        ${SYSLIB_DIR}/dpuconst.h
        ${SYSLIB_DIR}/dpufault.h
        ${SYSLIB_DIR}/dpuruntime.h
        ${SYSLIB_DIR}/extendhfsf2.c
        ${SYSLIB_DIR}/extendsfdf2.c
        ${SYSLIB_DIR}/ffsdi2.c
        ${SYSLIB_DIR}/ffssi2.c
        ${SYSLIB_DIR}/ffsti2.c
        ${SYSLIB_DIR}/fixdfdi.c
        ${SYSLIB_DIR}/fixdfsi.c
        ${SYSLIB_DIR}/fixsfdi.c
        ${SYSLIB_DIR}/fixsfsi.c
        ${SYSLIB_DIR}/fixunsdfdi.c
        ${SYSLIB_DIR}/fixunsdfsi.c
        ${SYSLIB_DIR}/fixunssfdi.c
        ${SYSLIB_DIR}/fixunssfsi.c
        ${SYSLIB_DIR}/floatdidf.c
        ${SYSLIB_DIR}/floatdisf.c
        ${SYSLIB_DIR}/float.h
        ${SYSLIB_DIR}/floatsidf.c
        ${SYSLIB_DIR}/floatsisf.c
        ${SYSLIB_DIR}/floatundidf.c
        ${SYSLIB_DIR}/floatundisf.c
        ${SYSLIB_DIR}/floatunsidf.c
        ${SYSLIB_DIR}/floatunsisf.c
        ${SYSLIB_DIR}/fp_extend.h
        ${SYSLIB_DIR}/fp_lib.h
        ${SYSLIB_DIR}/fp_trunc.h
        ${SYSLIB_DIR}/fsb_allocator.c
        ${SYSLIB_DIR}/fsb_allocator.h
        ${SYSLIB_DIR}/handshake.c
        ${SYSLIB_DIR}/handshake.h
        ${SYSLIB_DIR}/int_endianness.h
        ${SYSLIB_DIR}/int_lib.h
        ${SYSLIB_DIR}/int_math.h
        ${SYSLIB_DIR}/int_types.h
        ${SYSLIB_DIR}/int_util.c
        ${SYSLIB_DIR}/int_util.h
        ${SYSLIB_DIR}/listener.c
        ${SYSLIB_DIR}/lshrdi3.c
        ${SYSLIB_DIR}/macro_utils.h
        ${SYSLIB_DIR}/moddi3.c
        ${SYSLIB_DIR}/modsi3.c
        ${SYSLIB_DIR}/mram.h
        ${SYSLIB_DIR}/mul32.c
        ${SYSLIB_DIR}/mul64.c
        ${SYSLIB_DIR}/muldc3.c
        ${SYSLIB_DIR}/muldf3.c
        ${SYSLIB_DIR}/mulodi4.c
        ${SYSLIB_DIR}/mulosi4.c
        ${SYSLIB_DIR}/mulsf3.c
        ${SYSLIB_DIR}/mulvdi3.c
        ${SYSLIB_DIR}/mulvsi3.c
        ${SYSLIB_DIR}/mutex.h
        ${SYSLIB_DIR}/negdf2.c
        ${SYSLIB_DIR}/negdi2.c
        ${SYSLIB_DIR}/negsf2.c
        ${SYSLIB_DIR}/negvdi2.c
        ${SYSLIB_DIR}/negvsi2.c
        ${SYSLIB_DIR}/paritydi2.c
        ${SYSLIB_DIR}/paritysi2.c
        ${SYSLIB_DIR}/perfcounter.c
        ${SYSLIB_DIR}/perfcounter.h
        ${SYSLIB_DIR}/popcountdi2.c
        ${SYSLIB_DIR}/popcountsi2.c
        ${SYSLIB_DIR}/powidf2.c
        ${SYSLIB_DIR}/powisf2.c
        ${SYSLIB_DIR}/profiling.c
        ${SYSLIB_DIR}/profiling.h
        ${SYSLIB_DIR}/profiling_internals.h
        ${SYSLIB_DIR}/sem.c
        ${SYSLIB_DIR}/sem.h
        ${SYSLIB_DIR}/seqread32.c
        ${SYSLIB_DIR}/seqread64.c
        ${SYSLIB_DIR}/seqread128.c
        ${SYSLIB_DIR}/seqread256.c
        ${SYSLIB_DIR}/seqread512.c
        ${SYSLIB_DIR}/seqread1024.c
        ${SYSLIB_DIR}/seqread.h
        ${SYSLIB_DIR}/soft_cache.c
        ${SYSLIB_DIR}/soft_cache.h
        ${SYSLIB_DIR}/subdf3.c
        ${SYSLIB_DIR}/subsf3.c
        ${SYSLIB_DIR}/subvdi3.c
        ${SYSLIB_DIR}/subvsi3.c
        ${SYSLIB_DIR}/sysdef.h
        ${SYSLIB_DIR}/truncdfhf2.c
        ${SYSLIB_DIR}/truncdfsf2.c
        ${SYSLIB_DIR}/truncsfhf2.c
        ${SYSLIB_DIR}/ucmpdi2.c
        ${SYSLIB_DIR}/udiv64.c
        ${SYSLIB_DIR}/udivdi3.c
        ${SYSLIB_DIR}/udivmodsi4.c
        ${SYSLIB_DIR}/udivsi3.c
        ${SYSLIB_DIR}/umoddi3.c
        ${SYSLIB_DIR}/umodsi3.c
        ${SYSLIB_DIR}/waitqueue.c
   )

set(UTILS_SOURCES
        ${UTILS_DIR}/accessMramFromDpu.c
        ${UTILS_DIR}/coreDump.c
        ${UTILS_DIR}/internalStateReset.c
        ${UTILS_DIR}/restoreRegisters.c
    )

set(ALL_SOURCES ${STDLIB_C_SOURCES} ${SYSLIB_C_SOURCES})

set(INCLUDE_DIRECTORIES ${STDLIB_DIR} ${SYSLIB_DIR})

function(add_dpu_library)
    cmake_parse_arguments(DPU_LIB "PROFILING;LTO;LTO_THIN" "TARGET" "SOURCES" ${ARGN})
    set(OTHER_FLAGS "")

    if (DPU_LIB_PROFILING)
        set(OTHER_FLAGS ${OTHER_FLAGS} ${PROFILING_FLAGS})
    endif()

    if (DPU_LIB_LTO)
        set(OTHER_FLAGS ${OTHER_FLAGS} ${COMPILE_LTO_FLAGS})
    endif()

    if (DPU_LIB_LTO_THIN)
        set(OTHER_FLAGS ${OTHER_FLAGS} ${COMPILE_LTO_THIN_FLAGS})
    endif()

    add_library(${DPU_LIB_TARGET} STATIC "${DPU_LIB_SOURCES}")
    target_include_directories(${DPU_LIB_TARGET} PRIVATE ${INCLUDE_DIRECTORIES})

    target_compile_options(${DPU_LIB_TARGET} PRIVATE ${NOSTDLIB_FLAGS} ${STRICT_FLAGS} ${COMPILER_TIMESTAMP_DEF} ${OTHER_FLAGS})

    install(
        TARGETS ${DPU_LIB_TARGET}
        ARCHIVE
        DESTINATION share/upmem/include/built-in
        )
endfunction()

add_dpu_library(TARGET rt                             SOURCES ${ALL_SOURCES})
add_dpu_library(TARGET rt_p                 PROFILING SOURCES ${ALL_SOURCES})
add_dpu_library(TARGET rtlto       LTO                SOURCES ${ALL_SOURCES})
add_dpu_library(TARGET rtlto_p     LTO      PROFILING SOURCES ${ALL_SOURCES})
add_dpu_library(TARGET rtltothin   LTO_THIN           SOURCES ${ALL_SOURCES})
add_dpu_library(TARGET rtltothin_p LTO_THIN PROFILING SOURCES ${ALL_SOURCES})

add_library(rtmcount STATIC ${SYSLIB_DIR}/mcount.c)
target_compile_options(rtmcount PRIVATE ${NOSTDLIB_FLAGS} ${STRICT_FLAGS} ${COMPILER_TIMESTAMP_DEF})
install(
    TARGETS rtmcount
    ARCHIVE
    DESTINATION share/upmem/include/built-in
    )

install(
    DIRECTORY ${STDLIB_DIR} ${SYSLIB_DIR}
    DESTINATION share/upmem/include
    FILES_MATCHING PATTERN "*.h"
    )

install(
    FILES ${UTILS_DIR}/dpu.lds
    DESTINATION share/upmem/include/link
    )

install(
    FILES ${UTILS_DIR}/swreset.S
    DESTINATION share/upmem/include/misc
    )

function(add_bootstrap)
    cmake_parse_arguments(BOOTSTRAP "PROFILING" "TARGET" "SOURCES" ${ARGN})
    set(OTHER_FLAGS "")

    if (BOOTSTRAP_PROFILING)
        set(OTHER_FLAGS ${OTHER_FLAGS} ${PROFILING_FLAGS})
    endif()

    add_library(${BOOTSTRAP_TARGET} OBJECT ${BOOTSTRAP_SOURCES})
    target_include_directories(${BOOTSTRAP_TARGET} PRIVATE ${INCLUDE_DIRECTORIES})

    target_compile_options(${BOOTSTRAP_TARGET} PRIVATE ${STRICT_FLAGS} ${COMPILER_TIMESTAMP_DEF} ${OTHER_FLAGS})

    install(
        FILES $<TARGET_OBJECTS:${BOOTSTRAP_TARGET}>
        DESTINATION share/upmem/include/misc/
        RENAME ${BOOTSTRAP_TARGET}.o
        )
endfunction()

add_bootstrap(TARGET crt0             SOURCES ${UTILS_DIR}/crt0.c)
add_bootstrap(TARGET crt0_p PROFILING SOURCES ${UTILS_DIR}/crt0.c)

foreach(UTIL_FILE ${UTILS_SOURCES})
    get_filename_component(UTIL_TARGET ${UTIL_FILE} NAME_WE)
    add_executable(${UTIL_TARGET} ${UTIL_FILE})
    target_compile_options(${UTIL_TARGET} PRIVATE ${STRICT_FLAGS} ${NOSTDLIB_FLAGS} ${COMPILER_TIMESTAMP_DEF})
    set_target_properties(${UTIL_TARGET} PROPERTIES LINK_FLAGS "-nostartfiles -nostdlib -Wno-incompatible-library-redeclaration -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/${UTILS_DIR}/linkerScript.lds")
    target_include_directories(${UTIL_TARGET} PRIVATE ${INCLUDE_DIRECTORIES})
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${UTIL_TARGET}.bin
            COMMAND ${OBJCOPY} --dump-section=.text.__bootstrap=${CMAKE_CURRENT_BINARY_DIR}/${UTIL_TARGET}.bin ${UTIL_TARGET}
            DEPENDS ${UTIL_TARGET}
    )
    add_custom_target(
            ${UTIL_TARGET}_bin
            ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${UTIL_TARGET}.bin
    )
    add_dependencies(${UTIL_TARGET}_bin ${UTIL_TARGET})

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${UTIL_TARGET}.bin
        DESTINATION share/upmem/include/misc
        )
endforeach()

file(GLOB_RECURSE ALL_FORMAT_FILES *.c *.h)
list(SORT ALL_FORMAT_FILES)
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format-pattern "")
set(FIRST_SOURCE_FILE 1)

foreach (SOURCE_FILE ${ALL_FORMAT_FILES})
    string(REGEX MATCHALL "${CMAKE_CURRENT_SOURCE_DIR}/(target|build|CMakeFiles|(cmake-build-.*))" EXCLUDED_FILE_FOUND ${SOURCE_FILE})
    if (NOT ${EXCLUDED_FILE_FOUND} EQUAL -1)
        list(REMOVE_ITEM ALL_FORMAT_FILES ${SOURCE_FILE})
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
        format
        COMMAND ${CLANGFORMAT}
        -style=file
        -i
        ${ALL_FORMAT_FILES}
)

if (DPU_INSTALL_SOURCES)
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_LLVM.TXT
        DESTINATION src/dpu-rt
        )
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Toolchain-dpu.default.cmake
        RENAME Toolchain-dpu.cmake
        DESTINATION src/dpu-rt
        )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/misc
        DESTINATION src/dpu-rt/src
        )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib
        DESTINATION src/dpu-rt/src
        )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/syslib
        DESTINATION src/dpu-rt/src
        )
endif()
