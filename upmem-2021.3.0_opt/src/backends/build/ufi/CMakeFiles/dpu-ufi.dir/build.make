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
include ufi/CMakeFiles/dpu-ufi.dir/depend.make

# Include the progress variables for this target.
include ufi/CMakeFiles/dpu-ufi.dir/progress.make

# Include the compile flags for this target's objects.
include ufi/CMakeFiles/dpu-ufi.dir/flags.make

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o: ../ufi/src/ufi_bit_config.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_bit_config.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_bit_config.c > CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_bit_config.c -o CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o: ../ufi/src/ufi_dma_wavegen_config.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_dma_wavegen_config.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_dma_wavegen_config.c > CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_dma_wavegen_config.c -o CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.o: ../ufi/src/ufi.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi.c > CMakeFiles/dpu-ufi.dir/src/ufi.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi.c -o CMakeFiles/dpu-ufi.dir/src/ufi.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o: ../ufi/src/ufi_ci.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_ci.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_ci.c > CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_ci.c -o CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o: ../ufi/src/ufi_config.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_config.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_config.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_config.c > CMakeFiles/dpu-ufi.dir/src/ufi_config.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_config.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_config.c -o CMakeFiles/dpu-ufi.dir/src/ufi_config.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o: ../ufi/src/ufi_debug.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_debug.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_debug.c > CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_debug.c -o CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o: ../ufi/src/ufi_memory.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_memory.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_memory.c > CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_memory.c -o CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.s

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o: ufi/CMakeFiles/dpu-ufi.dir/flags.make
ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o: ../ufi/src/ufi_runner.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o   -c /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_runner.c

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.i"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_runner.c > CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.i

ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.s"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi/src/ufi_runner.c -o CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.s

# Object files for target dpu-ufi
dpu__ufi_OBJECTS = \
"CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o" \
"CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o"

# External object files for target dpu-ufi
dpu__ufi_EXTERNAL_OBJECTS =

ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_bit_config.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_dma_wavegen_config.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_ci.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_config.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_debug.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_memory.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/src/ufi_runner.c.o
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/build.make
ufi/libdpu-ufi.so.0.0: verbose/libdpuverbose.so.0.0
ufi/libdpu-ufi.so.0.0: ufi/CMakeFiles/dpu-ufi.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking C shared library libdpu-ufi.so"
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dpu-ufi.dir/link.txt --verbose=$(VERBOSE)
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && $(CMAKE_COMMAND) -E cmake_symlink_library libdpu-ufi.so.0.0 libdpu-ufi.so.0.0 libdpu-ufi.so

ufi/libdpu-ufi.so: ufi/libdpu-ufi.so.0.0
	@$(CMAKE_COMMAND) -E touch_nocreate ufi/libdpu-ufi.so

# Rule to build all files generated by this target.
ufi/CMakeFiles/dpu-ufi.dir/build: ufi/libdpu-ufi.so

.PHONY : ufi/CMakeFiles/dpu-ufi.dir/build

ufi/CMakeFiles/dpu-ufi.dir/clean:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi && $(CMAKE_COMMAND) -P CMakeFiles/dpu-ufi.dir/cmake_clean.cmake
.PHONY : ufi/CMakeFiles/dpu-ufi.dir/clean

ufi/CMakeFiles/dpu-ufi.dir/depend:
	cd /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/ufi /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi /home/siung/reduce_scatter/temp/PID-Comm/upmem-2021.3.0_opt/src/backends/build/ufi/CMakeFiles/dpu-ufi.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ufi/CMakeFiles/dpu-ufi.dir/depend

