# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATH_SUFFIXES "include" "libusb" "libusb-1.0"
    )

find_library(LIBUSB_LIBRARIES NAMES usb-1.0 usb)

include(FindPackageHandleStandardArgs)

#Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(LibUsb DEFAULT_MSG
    LIBUSB_LIBRARIES
    LIBUSB_INCLUDE_DIR
)
