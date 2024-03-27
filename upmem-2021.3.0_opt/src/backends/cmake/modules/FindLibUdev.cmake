# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_path(LIBUDEV_INCLUDE_DIR libudev.h)

find_library(LIBUDEV_LIBRARIES udev)

include(FindPackageHandleStandardArgs)

#Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(LibUdev DEFAULT_MSG
    LIBUDEV_LIBRARIES
    LIBUDEV_INCLUDE_DIR
)
