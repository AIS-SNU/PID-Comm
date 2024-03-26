# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_path(LIBELF_INCLUDE_DIR libelf.h)

find_library(LIBELF_LIBRARIES elf)

include(FindPackageHandleStandardArgs)

#Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(LibElf DEFAULT_MSG
	LIBELF_LIBRARIES
	LIBELF_INCLUDE_DIR
)
