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

# Include any dependencies generated for this target.
include api/CMakeFiles/dpujni.dir/depend.make

# Include the progress variables for this target.
include api/CMakeFiles/dpujni.dir/progress.make

# Include the compile flags for this target's objects.
include api/CMakeFiles/dpujni.dir/flags.make

api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o: api/CMakeFiles/dpujni.dir/flags.make
api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o: ../api/src/api/dpu_jni.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/api/src/api/dpu_jni.c

api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpujni.dir/src/api/dpu_jni.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/api/src/api/dpu_jni.c > CMakeFiles/dpujni.dir/src/api/dpu_jni.c.i

api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpujni.dir/src/api/dpu_jni.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/api/src/api/dpu_jni.c -o CMakeFiles/dpujni.dir/src/api/dpu_jni.c.s

# Object files for target dpujni
dpujni_OBJECTS = \
"CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o"

# External object files for target dpujni
dpujni_EXTERNAL_OBJECTS =

api/libdpujni.so.0.0: api/CMakeFiles/dpujni.dir/src/api/dpu_jni.c.o
api/libdpujni.so.0.0: api/CMakeFiles/dpujni.dir/build.make
api/libdpujni.so.0.0: api/libdpu.so.0.0
api/libdpujni.so.0.0: verbose/libdpuverbose.so.0.0
api/libdpujni.so.0.0: /usr/lib/x86_64-linux-gnu/libelf.so
api/libdpujni.so.0.0: /usr/lib/x86_64-linux-gnu/libpython3.8.so
api/libdpujni.so.0.0: /usr/lib/x86_64-linux-gnu/libnuma.so
api/libdpujni.so.0.0: api/CMakeFiles/dpujni.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C shared library libdpujni.so"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dpujni.dir/link.txt --verbose=$(VERBOSE)
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && $(CMAKE_COMMAND) -E cmake_symlink_library libdpujni.so.0.0 libdpujni.so.0.0 libdpujni.so

api/libdpujni.so: api/libdpujni.so.0.0
	@$(CMAKE_COMMAND) -E touch_nocreate api/libdpujni.so

# Rule to build all files generated by this target.
api/CMakeFiles/dpujni.dir/build: api/libdpujni.so

.PHONY : api/CMakeFiles/dpujni.dir/build

api/CMakeFiles/dpujni.dir/clean:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api && $(CMAKE_COMMAND) -P CMakeFiles/dpujni.dir/cmake_clean.cmake
.PHONY : api/CMakeFiles/dpujni.dir/clean

api/CMakeFiles/dpujni.dir/depend:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/api /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/api/CMakeFiles/dpujni.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : api/CMakeFiles/dpujni.dir/depend

