# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build

# Utility rule file for ufi-format.

# Include the progress variables for this target.
include ufi/CMakeFiles/ufi-format.dir/progress.make

ufi/CMakeFiles/ufi-format:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && clang-format -style=file -i /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_bit_config.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_ci.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_ci_commands.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_ci_types.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_config.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_debug.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_dma_wavegen_config.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_memory.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/include/ufi/ufi_runner.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_bit_config.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_ci.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_config.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_debug.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_dma_wavegen_config.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_memory.c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_rank_utils.h /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_runner.c

ufi-format: ufi/CMakeFiles/ufi-format
ufi-format: ufi/CMakeFiles/ufi-format.dir/build.make

.PHONY : ufi-format

# Rule to build all files generated by this target.
ufi/CMakeFiles/ufi-format.dir/build: ufi-format

.PHONY : ufi/CMakeFiles/ufi-format.dir/build

ufi/CMakeFiles/ufi-format.dir/clean:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && $(CMAKE_COMMAND) -P CMakeFiles/ufi-format.dir/cmake_clean.cmake
.PHONY : ufi/CMakeFiles/ufi-format.dir/clean

ufi/CMakeFiles/ufi-format.dir/depend:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi/CMakeFiles/ufi-format.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ufi/CMakeFiles/ufi-format.dir/depend

