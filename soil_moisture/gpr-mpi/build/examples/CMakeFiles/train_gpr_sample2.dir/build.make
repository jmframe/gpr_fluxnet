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
CMAKE_COMMAND = /gpfsm/dulocal/sles12/other/cmake/3.17.0/bin/cmake

# The command to remove a file.
RM = /gpfsm/dulocal/sles12/other/cmake/3.17.0/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build

# Include any dependencies generated for this target.
include examples/CMakeFiles/train_gpr_sample2.dir/depend.make

# Include the progress variables for this target.
include examples/CMakeFiles/train_gpr_sample2.dir/progress.make

# Include the compile flags for this target's objects.
include examples/CMakeFiles/train_gpr_sample2.dir/flags.make

examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o: examples/CMakeFiles/train_gpr_sample2.dir/flags.make
examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o: ../examples/train_gpr_sample2.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o"
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && /usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/bin/mpiicpc  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o -c /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/examples/train_gpr_sample2.cpp

examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.i"
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && /usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/bin/mpiicpc $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/examples/train_gpr_sample2.cpp > CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.i

examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.s"
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && /usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/bin/mpiicpc $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/examples/train_gpr_sample2.cpp -o CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.s

# Object files for target train_gpr_sample2
train_gpr_sample2_OBJECTS = \
"CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o"

# External object files for target train_gpr_sample2
train_gpr_sample2_EXTERNAL_OBJECTS =

examples/train_gpr_sample2: examples/CMakeFiles/train_gpr_sample2.dir/train_gpr_sample2.cpp.o
examples/train_gpr_sample2: examples/CMakeFiles/train_gpr_sample2.dir/build.make
examples/train_gpr_sample2: framework/libmatrix_utils.a
examples/train_gpr_sample2: /usr/local/other/gsl/2.6/lib/libgsl.a
examples/train_gpr_sample2: /usr/local/other/gsl/2.6/lib/libgslcblas.a
examples/train_gpr_sample2: examples/CMakeFiles/train_gpr_sample2.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable train_gpr_sample2"
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/train_gpr_sample2.dir/link.txt --verbose=$(VERBOSE)
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && /gpfsm/dulocal/sles12/other/cmake/3.17.0/bin/cmake -E copy /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/examples/*.bin /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples

# Rule to build all files generated by this target.
examples/CMakeFiles/train_gpr_sample2.dir/build: examples/train_gpr_sample2

.PHONY : examples/CMakeFiles/train_gpr_sample2.dir/build

examples/CMakeFiles/train_gpr_sample2.dir/clean:
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples && $(CMAKE_COMMAND) -P CMakeFiles/train_gpr_sample2.dir/cmake_clean.cmake
.PHONY : examples/CMakeFiles/train_gpr_sample2.dir/clean

examples/CMakeFiles/train_gpr_sample2.dir/depend:
	cd /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/examples /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/build/examples/CMakeFiles/train_gpr_sample2.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/CMakeFiles/train_gpr_sample2.dir/depend

