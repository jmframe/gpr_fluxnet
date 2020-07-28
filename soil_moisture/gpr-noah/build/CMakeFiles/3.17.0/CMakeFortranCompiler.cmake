set(CMAKE_Fortran_COMPILER "/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/bin/intel64/ifort")
set(CMAKE_Fortran_COMPILER_ARG1 "")
set(CMAKE_Fortran_COMPILER_ID "Intel")
set(CMAKE_Fortran_COMPILER_VERSION "19.1.0.20191121")
set(CMAKE_Fortran_COMPILER_WRAPPER "")
set(CMAKE_Fortran_PLATFORM_ID "Linux")
set(CMAKE_Fortran_SIMULATE_ID "")
set(CMAKE_Fortran_SIMULATE_VERSION "")



set(CMAKE_AR "/usr/bin/ar")
set(CMAKE_Fortran_COMPILER_AR "")
set(CMAKE_RANLIB "/usr/bin/ranlib")
set(CMAKE_Fortran_COMPILER_RANLIB "")
set(CMAKE_COMPILER_IS_GNUG77 )
set(CMAKE_Fortran_COMPILER_LOADED 1)
set(CMAKE_Fortran_COMPILER_WORKS TRUE)
set(CMAKE_Fortran_ABI_COMPILED TRUE)
set(CMAKE_COMPILER_IS_MINGW )
set(CMAKE_COMPILER_IS_CYGWIN )
if(CMAKE_COMPILER_IS_CYGWIN)
  set(CYGWIN 1)
  set(UNIX 1)
endif()

set(CMAKE_Fortran_COMPILER_ENV_VAR "FC")

set(CMAKE_Fortran_COMPILER_SUPPORTS_F90 1)

if(CMAKE_COMPILER_IS_MINGW)
  set(MINGW 1)
endif()
set(CMAKE_Fortran_COMPILER_ID_RUN 1)
set(CMAKE_Fortran_SOURCE_FILE_EXTENSIONS f;F;fpp;FPP;f77;F77;f90;F90;for;For;FOR;f95;F95)
set(CMAKE_Fortran_IGNORE_EXTENSIONS h;H;o;O;obj;OBJ;def;DEF;rc;RC)
set(CMAKE_Fortran_LINKER_PREFERENCE 20)
if(UNIX)
  set(CMAKE_Fortran_OUTPUT_EXTENSION .o)
else()
  set(CMAKE_Fortran_OUTPUT_EXTENSION .obj)
endif()

# Save compiler ABI information.
set(CMAKE_Fortran_SIZEOF_DATA_PTR "8")
set(CMAKE_Fortran_COMPILER_ABI "ELF")
set(CMAKE_Fortran_LIBRARY_ARCHITECTURE "")

if(CMAKE_Fortran_SIZEOF_DATA_PTR AND NOT CMAKE_SIZEOF_VOID_P)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_Fortran_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_Fortran_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_Fortran_COMPILER_ABI}")
endif()

if(CMAKE_Fortran_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()





set(CMAKE_Fortran_IMPLICIT_INCLUDE_DIRECTORIES "/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/include;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/ipp/include;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mkl/include;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/tbb/include;/gpfsm/dulocal/sles12/intel/2020/compilers_and_libraries_2020.0.166/linux/compiler/include/intel64;/gpfsm/dulocal/sles12/intel/2020/compilers_and_libraries_2020.0.166/linux/compiler/include/icc;/gpfsm/dulocal/sles12/intel/2020/compilers_and_libraries_2020.0.166/linux/compiler/include;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0/include;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0/include-fixed;/gpfsm/dulocal/sles12/other/gcc/8.3.0/include;/usr/include")
set(CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES "ifport;ifcoremt;imf;svml;m;ipgo;irc;pthread;svml;c;gcc;gcc_s;irc_s;dl;c")
set(CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES "/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib/release;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/compiler/lib/intel64;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/ipp/lib/intel64;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/mkl/lib/intel64;/usr/local/intel/2020/compilers_and_libraries_2020.0.166/linux/tbb/lib/intel64/gcc4.1;/usr/local/other/gcc/8.3.0/lib64;/gpfsm/dulocal/sles12/intel/2020/compilers_and_libraries_2020.0.166/linux/compiler/lib/intel64_lin;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib/gcc;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib64;/lib64;/usr/lib64;/gpfsm/dulocal/sles12/other/gcc/8.3.0/lib;/lib;/usr/lib")
set(CMAKE_Fortran_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")
