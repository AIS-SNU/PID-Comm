# Copyright 2021 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GEN_INCLUDE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)
set(GEN_INCLUDE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include PARENT_SCOPE)
file(MAKE_DIRECTORY ${GEN_INCLUDE_DIRECTORY})

add_custom_target(gen_files)

function(add_gen_target name generator output options)
    add_custom_command(
        OUTPUT ${output}
        MAIN_DEPENDENCY ${generator}
        COMMAND ${Python3_EXECUTABLE} ${generator} ${options} ${output}
        COMMENT "Generating ${output} from ${generator}"
    )
    add_custom_target(${name} DEPENDS ${output})
    add_dependencies(gen_files ${name})
endfunction()

add_gen_target(gen_profile_header
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_profile_files.py
    ${GEN_INCLUDE_DIRECTORY}/dpu_profile_properties.h
    "defs"
)
