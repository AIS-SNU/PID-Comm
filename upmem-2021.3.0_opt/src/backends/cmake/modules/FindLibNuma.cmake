# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_path(LIBNUMA_INCLUDE_DIR numa.h)

find_library(LIBNUMA_LIBRARIES numa)

include(FindPackageHandleStandardArgs)

#Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(LibNuma DEFAULT_MSG
    LIBNUMA_LIBRARIES
    LIBNUMA_INCLUDE_DIR
)
