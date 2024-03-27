# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13)

set(CMAKE_BUILD_TYPE Debug)
set(HOST_LLDB_BINDIR share/upmem/lldb)

add_executable(host-lldb-attach-dpu lldb-attach-dpu.c)
target_include_directories(host-lldb-attach-dpu PUBLIC ../api/include/lowlevel ../api/include/api)
target_link_libraries(host-lldb-attach-dpu dpu)

install(
	TARGETS host-lldb-attach-dpu
	RUNTIME
	DESTINATION ${HOST_LLDB_BINDIR}
	)
