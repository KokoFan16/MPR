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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.17.0/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.17.0/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/kokofan/Documents/project/MPR

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/kokofan/Documents/project/MPR/build

# Include any dependencies generated for this target.
include examples/CMakeFiles/multi_res_read.dir/depend.make

# Include the progress variables for this target.
include examples/CMakeFiles/multi_res_read.dir/progress.make

# Include the compile flags for this target's objects.
include examples/CMakeFiles/multi_res_read.dir/flags.make

examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o: examples/CMakeFiles/multi_res_read.dir/flags.make
examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o: ../examples/multiple_resolution_read.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/examples && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o   -c /Users/kokofan/Documents/project/MPR/examples/multiple_resolution_read.c

examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/examples && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/examples/multiple_resolution_read.c > CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.i

examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/examples && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/examples/multiple_resolution_read.c -o CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.s

# Object files for target multi_res_read
multi_res_read_OBJECTS = \
"CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o"

# External object files for target multi_res_read
multi_res_read_EXTERNAL_OBJECTS =

examples/multi_res_read: examples/CMakeFiles/multi_res_read.dir/multiple_resolution_read.c.o
examples/multi_res_read: examples/CMakeFiles/multi_res_read.dir/build.make
examples/multi_res_read: src/libmpr.a
examples/multi_res_read: external/libzfp.a
examples/multi_res_read: /usr/local/Cellar/mpich/3.3.2/lib/libmpi.dylib
examples/multi_res_read: /usr/local/Cellar/mpich/3.3.2/lib/libpmpi.dylib
examples/multi_res_read: src/data_handle/libmpr_data_handle.a
examples/multi_res_read: src/utils/libmpr_utils.a
examples/multi_res_read: src/comm/libmpr_comm.a
examples/multi_res_read: src/io/libmpr_io.a
examples/multi_res_read: src/core/libmpr_core.a
examples/multi_res_read: external/libzfp.a
examples/multi_res_read: examples/CMakeFiles/multi_res_read.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable multi_res_read"
	cd /Users/kokofan/Documents/project/MPR/build/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/multi_res_read.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/CMakeFiles/multi_res_read.dir/build: examples/multi_res_read

.PHONY : examples/CMakeFiles/multi_res_read.dir/build

examples/CMakeFiles/multi_res_read.dir/clean:
	cd /Users/kokofan/Documents/project/MPR/build/examples && $(CMAKE_COMMAND) -P CMakeFiles/multi_res_read.dir/cmake_clean.cmake
.PHONY : examples/CMakeFiles/multi_res_read.dir/clean

examples/CMakeFiles/multi_res_read.dir/depend:
	cd /Users/kokofan/Documents/project/MPR/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/kokofan/Documents/project/MPR /Users/kokofan/Documents/project/MPR/examples /Users/kokofan/Documents/project/MPR/build /Users/kokofan/Documents/project/MPR/build/examples /Users/kokofan/Documents/project/MPR/build/examples/CMakeFiles/multi_res_read.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/CMakeFiles/multi_res_read.dir/depend

