# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/roys/svn3d/imgv/other/oscpack_1_1_0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/roys/svn3d/imgv/other/oscpack_1_1_0/build

# Include any dependencies generated for this target.
include CMakeFiles/OscDump.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/OscDump.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/OscDump.dir/flags.make

CMakeFiles/OscDump.dir/examples/OscDump.cpp.o: CMakeFiles/OscDump.dir/flags.make
CMakeFiles/OscDump.dir/examples/OscDump.cpp.o: ../examples/OscDump.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/roys/svn3d/imgv/other/oscpack_1_1_0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/OscDump.dir/examples/OscDump.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/OscDump.dir/examples/OscDump.cpp.o -c /home/roys/svn3d/imgv/other/oscpack_1_1_0/examples/OscDump.cpp

CMakeFiles/OscDump.dir/examples/OscDump.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/OscDump.dir/examples/OscDump.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/roys/svn3d/imgv/other/oscpack_1_1_0/examples/OscDump.cpp > CMakeFiles/OscDump.dir/examples/OscDump.cpp.i

CMakeFiles/OscDump.dir/examples/OscDump.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/OscDump.dir/examples/OscDump.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/roys/svn3d/imgv/other/oscpack_1_1_0/examples/OscDump.cpp -o CMakeFiles/OscDump.dir/examples/OscDump.cpp.s

CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.requires:

.PHONY : CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.requires

CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.provides: CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.requires
	$(MAKE) -f CMakeFiles/OscDump.dir/build.make CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.provides.build
.PHONY : CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.provides

CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.provides.build: CMakeFiles/OscDump.dir/examples/OscDump.cpp.o


# Object files for target OscDump
OscDump_OBJECTS = \
"CMakeFiles/OscDump.dir/examples/OscDump.cpp.o"

# External object files for target OscDump
OscDump_EXTERNAL_OBJECTS =

OscDump: CMakeFiles/OscDump.dir/examples/OscDump.cpp.o
OscDump: CMakeFiles/OscDump.dir/build.make
OscDump: liboscpack.so
OscDump: CMakeFiles/OscDump.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/roys/svn3d/imgv/other/oscpack_1_1_0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable OscDump"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/OscDump.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/OscDump.dir/build: OscDump

.PHONY : CMakeFiles/OscDump.dir/build

CMakeFiles/OscDump.dir/requires: CMakeFiles/OscDump.dir/examples/OscDump.cpp.o.requires

.PHONY : CMakeFiles/OscDump.dir/requires

CMakeFiles/OscDump.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/OscDump.dir/cmake_clean.cmake
.PHONY : CMakeFiles/OscDump.dir/clean

CMakeFiles/OscDump.dir/depend:
	cd /home/roys/svn3d/imgv/other/oscpack_1_1_0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/roys/svn3d/imgv/other/oscpack_1_1_0 /home/roys/svn3d/imgv/other/oscpack_1_1_0 /home/roys/svn3d/imgv/other/oscpack_1_1_0/build /home/roys/svn3d/imgv/other/oscpack_1_1_0/build /home/roys/svn3d/imgv/other/oscpack_1_1_0/build/CMakeFiles/OscDump.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/OscDump.dir/depend

