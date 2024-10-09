# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /src/build_js

# Utility rule file for opencv_js_loader.

# Include any custom commands dependencies for this target.
include modules/js/CMakeFiles/opencv_js_loader.dir/compiler_depend.make

# Include the progress variables for this target.
include modules/js/CMakeFiles/opencv_js_loader.dir/progress.make

modules/js/CMakeFiles/opencv_js_loader: bin/opencv.js
modules/js/CMakeFiles/opencv_js_loader: ../modules/js/src/loader.js
modules/js/CMakeFiles/opencv_js_loader: bin/loader.js

bin/opencv.js: bin/opencv_js.js
bin/opencv.js: ../modules/js/src/make_umd.py
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/src/build_js/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating ../../bin/opencv.js"
	cd /src/build_js/modules/js && /usr/bin/python3 /src/modules/js/src/make_umd.py /src/build_js/bin/opencv_js.js /src/build_js/bin/opencv.js

opencv_js_loader: bin/opencv.js
opencv_js_loader: modules/js/CMakeFiles/opencv_js_loader
opencv_js_loader: modules/js/CMakeFiles/opencv_js_loader.dir/build.make
.PHONY : opencv_js_loader

# Rule to build all files generated by this target.
modules/js/CMakeFiles/opencv_js_loader.dir/build: opencv_js_loader
.PHONY : modules/js/CMakeFiles/opencv_js_loader.dir/build

modules/js/CMakeFiles/opencv_js_loader.dir/clean:
	cd /src/build_js/modules/js && $(CMAKE_COMMAND) -P CMakeFiles/opencv_js_loader.dir/cmake_clean.cmake
.PHONY : modules/js/CMakeFiles/opencv_js_loader.dir/clean

modules/js/CMakeFiles/opencv_js_loader.dir/depend:
	cd /src/build_js && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /src /src/modules/js /src/build_js /src/build_js/modules/js /src/build_js/modules/js/CMakeFiles/opencv_js_loader.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : modules/js/CMakeFiles/opencv_js_loader.dir/depend

