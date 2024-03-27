# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_path(LIBFTDI_INCLUDE_DIR
    NAMES ftdi.h
    PATHS   /usr/local/include
            /usr/include
            /opt/local/include
    PATH_SUFFIXES libftdi libftdi1
    )

find_library(LIBFTDI_LIBRARIES
    NAMES ftdi ftdi1
    PATHS   /usr/lib
            /usr/local/lib
            /opt/local/lib
    )

include(FindPackageHandleStandardArgs)

#Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(LibFtdi DEFAULT_MSG
    LIBFTDI_LIBRARIES
    LIBFTDI_INCLUDE_DIR
)
