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
include src/data_handle/CMakeFiles/mpr_data_handle.dir/depend.make

# Include the progress variables for this target.
include src/data_handle/CMakeFiles/mpr_data_handle.dir/progress.make

# Include the compile flags for this target's objects.
include src/data_handle/CMakeFiles/mpr_data_handle.dir/flags.make

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o: src/data_handle/CMakeFiles/mpr_data_handle.dir/flags.make
src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o: ../src/data_handle/MPR_data_types.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o   -c /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_data_types.c

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_data_types.c > CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.i

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_data_types.c -o CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.s

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o: src/data_handle/CMakeFiles/mpr_data_handle.dir/flags.make
src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o: ../src/data_handle/MPR_get_set.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o   -c /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_get_set.c

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_get_set.c > CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.i

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_get_set.c -o CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.s

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o: src/data_handle/CMakeFiles/mpr_data_handle.dir/flags.make
src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o: ../src/data_handle/MPR_variable.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o   -c /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_variable.c

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_data_handle.dir/MPR_variable.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_variable.c > CMakeFiles/mpr_data_handle.dir/MPR_variable.c.i

src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_data_handle.dir/MPR_variable.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/data_handle/MPR_variable.c -o CMakeFiles/mpr_data_handle.dir/MPR_variable.c.s

# Object files for target mpr_data_handle
mpr_data_handle_OBJECTS = \
"CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o" \
"CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o" \
"CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o"

# External object files for target mpr_data_handle
mpr_data_handle_EXTERNAL_OBJECTS =

src/data_handle/libmpr_data_handle.a: src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_data_types.c.o
src/data_handle/libmpr_data_handle.a: src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_get_set.c.o
src/data_handle/libmpr_data_handle.a: src/data_handle/CMakeFiles/mpr_data_handle.dir/MPR_variable.c.o
src/data_handle/libmpr_data_handle.a: src/data_handle/CMakeFiles/mpr_data_handle.dir/build.make
src/data_handle/libmpr_data_handle.a: src/data_handle/CMakeFiles/mpr_data_handle.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C static library libmpr_data_handle.a"
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && $(CMAKE_COMMAND) -P CMakeFiles/mpr_data_handle.dir/cmake_clean_target.cmake
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mpr_data_handle.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/data_handle/CMakeFiles/mpr_data_handle.dir/build: src/data_handle/libmpr_data_handle.a

.PHONY : src/data_handle/CMakeFiles/mpr_data_handle.dir/build

src/data_handle/CMakeFiles/mpr_data_handle.dir/clean:
	cd /Users/kokofan/Documents/project/MPR/build/src/data_handle && $(CMAKE_COMMAND) -P CMakeFiles/mpr_data_handle.dir/cmake_clean.cmake
.PHONY : src/data_handle/CMakeFiles/mpr_data_handle.dir/clean

src/data_handle/CMakeFiles/mpr_data_handle.dir/depend:
	cd /Users/kokofan/Documents/project/MPR/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/kokofan/Documents/project/MPR /Users/kokofan/Documents/project/MPR/src/data_handle /Users/kokofan/Documents/project/MPR/build /Users/kokofan/Documents/project/MPR/build/src/data_handle /Users/kokofan/Documents/project/MPR/build/src/data_handle/CMakeFiles/mpr_data_handle.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/data_handle/CMakeFiles/mpr_data_handle.dir/depend

