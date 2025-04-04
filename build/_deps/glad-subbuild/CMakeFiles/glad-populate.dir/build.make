# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.31

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
CMAKE_COMMAND = /opt/homebrew/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild

# Utility rule file for glad-populate.

# Include any custom commands dependencies for this target.
include CMakeFiles/glad-populate.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/glad-populate.dir/progress.make

CMakeFiles/glad-populate: CMakeFiles/glad-populate-complete

CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-install
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-mkdir
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-download
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-update
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-build
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-install
CMakeFiles/glad-populate-complete: glad-populate-prefix/src/glad-populate-stamp/glad-populate-test
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Completed 'glad-populate'"
	/opt/homebrew/bin/cmake -E make_directory /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles
	/opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles/glad-populate-complete
	/opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-done

glad-populate-prefix/src/glad-populate-stamp/glad-populate-update:
.PHONY : glad-populate-prefix/src/glad-populate-stamp/glad-populate-update

glad-populate-prefix/src/glad-populate-stamp/glad-populate-build: glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "No build step for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-build

glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure: glad-populate-prefix/tmp/glad-populate-cfgcmd.txt
glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure: glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "No configure step for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure

glad-populate-prefix/src/glad-populate-stamp/glad-populate-download: glad-populate-prefix/src/glad-populate-stamp/glad-populate-gitinfo.txt
glad-populate-prefix/src/glad-populate-stamp/glad-populate-download: glad-populate-prefix/src/glad-populate-stamp/glad-populate-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Performing download step (git clone) for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps && /opt/homebrew/bin/cmake -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/tmp/glad-populate-gitclone.cmake
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps && /opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-download

glad-populate-prefix/src/glad-populate-stamp/glad-populate-install: glad-populate-prefix/src/glad-populate-stamp/glad-populate-build
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "No install step for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-install

glad-populate-prefix/src/glad-populate-stamp/glad-populate-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Creating directories for 'glad-populate'"
	/opt/homebrew/bin/cmake -Dcfgdir= -P /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/tmp/glad-populate-mkdirs.cmake
	/opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-mkdir

glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch: glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch-info.txt
glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch: glad-populate-prefix/src/glad-populate-stamp/glad-populate-update
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "No patch step for 'glad-populate'"
	/opt/homebrew/bin/cmake -E echo_append
	/opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch

glad-populate-prefix/src/glad-populate-stamp/glad-populate-update:
.PHONY : glad-populate-prefix/src/glad-populate-stamp/glad-populate-update

glad-populate-prefix/src/glad-populate-stamp/glad-populate-test: glad-populate-prefix/src/glad-populate-stamp/glad-populate-install
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "No test step for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build && /opt/homebrew/bin/cmake -E touch /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/glad-populate-test

glad-populate-prefix/src/glad-populate-stamp/glad-populate-update: glad-populate-prefix/tmp/glad-populate-gitupdate.cmake
glad-populate-prefix/src/glad-populate-stamp/glad-populate-update: glad-populate-prefix/src/glad-populate-stamp/glad-populate-update-info.txt
glad-populate-prefix/src/glad-populate-stamp/glad-populate-update: glad-populate-prefix/src/glad-populate-stamp/glad-populate-download
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Performing update step for 'glad-populate'"
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-src && /opt/homebrew/bin/cmake -Dcan_fetch=YES -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/tmp/glad-populate-gitupdate.cmake

CMakeFiles/glad-populate.dir/codegen:
.PHONY : CMakeFiles/glad-populate.dir/codegen

glad-populate: CMakeFiles/glad-populate
glad-populate: CMakeFiles/glad-populate-complete
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-build
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-configure
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-download
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-install
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-mkdir
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-patch
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-test
glad-populate: glad-populate-prefix/src/glad-populate-stamp/glad-populate-update
glad-populate: CMakeFiles/glad-populate.dir/build.make
.PHONY : glad-populate

# Rule to build all files generated by this target.
CMakeFiles/glad-populate.dir/build: glad-populate
.PHONY : CMakeFiles/glad-populate.dir/build

CMakeFiles/glad-populate.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/glad-populate.dir/cmake_clean.cmake
.PHONY : CMakeFiles/glad-populate.dir/clean

CMakeFiles/glad-populate.dir/depend:
	cd /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild /Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/CMakeFiles/glad-populate.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/glad-populate.dir/depend

