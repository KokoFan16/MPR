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
include src/core/CMakeFiles/mpr_core.dir/depend.make

# Include the progress variables for this target.
include src/core/CMakeFiles/mpr_core.dir/progress.make

# Include the compile flags for this target's objects.
include src/core/CMakeFiles/mpr_core.dir/flags.make

src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.o: src/core/CMakeFiles/mpr_core.dir/flags.make
src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.o: ../src/core/MPR_aggregation.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_core.dir/MPR_aggregation.c.o   -c /Users/kokofan/Documents/project/MPR/src/core/MPR_aggregation.c

src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_core.dir/MPR_aggregation.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/core/MPR_aggregation.c > CMakeFiles/mpr_core.dir/MPR_aggregation.c.i

src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_core.dir/MPR_aggregation.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/core/MPR_aggregation.c -o CMakeFiles/mpr_core.dir/MPR_aggregation.c.s

src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.o: src/core/CMakeFiles/mpr_core.dir/flags.make
src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.o: ../src/core/MPR_compression.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_core.dir/MPR_compression.c.o   -c /Users/kokofan/Documents/project/MPR/src/core/MPR_compression.c

src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_core.dir/MPR_compression.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/core/MPR_compression.c > CMakeFiles/mpr_core.dir/MPR_compression.c.i

src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_core.dir/MPR_compression.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/core/MPR_compression.c -o CMakeFiles/mpr_core.dir/MPR_compression.c.s

src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.o: src/core/CMakeFiles/mpr_core.dir/flags.make
src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.o: ../src/core/MPR_restructure.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_core.dir/MPR_restructure.c.o   -c /Users/kokofan/Documents/project/MPR/src/core/MPR_restructure.c

src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_core.dir/MPR_restructure.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/core/MPR_restructure.c > CMakeFiles/mpr_core.dir/MPR_restructure.c.i

src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_core.dir/MPR_restructure.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/core/MPR_restructure.c -o CMakeFiles/mpr_core.dir/MPR_restructure.c.s

src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.o: src/core/CMakeFiles/mpr_core.dir/flags.make
src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.o: ../src/core/MPR_wavelet.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.o"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mpr_core.dir/MPR_wavelet.c.o   -c /Users/kokofan/Documents/project/MPR/src/core/MPR_wavelet.c

src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mpr_core.dir/MPR_wavelet.c.i"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/kokofan/Documents/project/MPR/src/core/MPR_wavelet.c > CMakeFiles/mpr_core.dir/MPR_wavelet.c.i

src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mpr_core.dir/MPR_wavelet.c.s"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && /Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/kokofan/Documents/project/MPR/src/core/MPR_wavelet.c -o CMakeFiles/mpr_core.dir/MPR_wavelet.c.s

# Object files for target mpr_core
mpr_core_OBJECTS = \
"CMakeFiles/mpr_core.dir/MPR_aggregation.c.o" \
"CMakeFiles/mpr_core.dir/MPR_compression.c.o" \
"CMakeFiles/mpr_core.dir/MPR_restructure.c.o" \
"CMakeFiles/mpr_core.dir/MPR_wavelet.c.o"

# External object files for target mpr_core
mpr_core_EXTERNAL_OBJECTS =

src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/MPR_aggregation.c.o
src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/MPR_compression.c.o
src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/MPR_restructure.c.o
src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/MPR_wavelet.c.o
src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/build.make
src/core/libmpr_core.a: src/core/CMakeFiles/mpr_core.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/kokofan/Documents/project/MPR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking C static library libmpr_core.a"
	cd /Users/kokofan/Documents/project/MPR/build/src/core && $(CMAKE_COMMAND) -P CMakeFiles/mpr_core.dir/cmake_clean_target.cmake
	cd /Users/kokofan/Documents/project/MPR/build/src/core && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mpr_core.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/core/CMakeFiles/mpr_core.dir/build: src/core/libmpr_core.a

.PHONY : src/core/CMakeFiles/mpr_core.dir/build

src/core/CMakeFiles/mpr_core.dir/clean:
	cd /Users/kokofan/Documents/project/MPR/build/src/core && $(CMAKE_COMMAND) -P CMakeFiles/mpr_core.dir/cmake_clean.cmake
.PHONY : src/core/CMakeFiles/mpr_core.dir/clean

src/core/CMakeFiles/mpr_core.dir/depend:
	cd /Users/kokofan/Documents/project/MPR/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/kokofan/Documents/project/MPR /Users/kokofan/Documents/project/MPR/src/core /Users/kokofan/Documents/project/MPR/build /Users/kokofan/Documents/project/MPR/build/src/core /Users/kokofan/Documents/project/MPR/build/src/core/CMakeFiles/mpr_core.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/core/CMakeFiles/mpr_core.dir/depend

