cmake_minimum_required(VERSION 3.8)
project (GaussianProcessRegression)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
  message("C++ compiler recognized as Intel ... setting compiler specific options with -gcc-name=$ENV{GXX_NAME}.")
  set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wall -gxx-name=$ENV{GXX_NAME}")
  set(FORTRAN_LIB "ifcore")
else(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  message("C++ compiler recognized as GNU ... setting compiler specific options.")
  set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wall")
  set(FORTRAN_LIB "gfortran")
endif()

#enable/disable mpi
if(MPI_DISABLED)
  message("MPI_DISABLED=yes: GPR will be built without MPI.")
  add_definitions(-DMPI_DISABLED)
else()
  message("MPI_ENABLED: GPR will be built with MPI.")
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_MPI_FLAGS})
endif()
enable_language (CXX C Fortran)


#external libraries
set(BLAS_HOME $ENV{BLAS_HOME})
set(LAPACK_HOME $ENV{LAPACK_HOME})
set(SCALAPACK_HOME $ENV{SCALAPACK_HOME})
set(GSL_HOME $ENV{GSL_HOME})
set(CPPUNIT_HOME $ENV{CPPUNIT_HOME})

#set scalapack/lapack/blas depending on compiler. For intel we use MKL.
if(MPI_DISABLED)
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(LIN_ALG -lmkl_intel_lp64 -lmkl_intel_thread -liomp5 -lpthread -lm -lmkl_core -lmkl_lapack95_lp64)
  else()
    set(LIN_ALG ${LAPACK_HOME}/liblapack.a ${BLAS_HOME}/blas_LINUX.a)
  endif()
else()
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(LIN_ALG -lmkl_scalapack_lp64 -lmkl_blacs_intelmpi_lp64 -lmkl_intel_lp64 -lmkl_intel_thread -liomp5 -lpthread -lm -lmkl_core -lmkl_lapack95_lp64)
  else()
    set(LIN_ALG ${SCALAPACK_HOME}/libscalapack.a ${LAPACK_HOME}/liblapack.a ${BLAS_HOME}/blas_LINUX.a)
  endif()
endif()

#set global common libs to reduce redundancy
if(MPI_DISABLED)
  set(LIBS noah matrix_utils ${LIN_ALG} ${GSL_HOME}/lib/libgsl.a ${GSL_HOME}/lib/libgslcblas.a)
else()
  set(LIBS ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} matrix_utils ${LIN_ALG} ${GSL_HOME}/lib/libgsl.a ${GSL_HOME}/lib/libgslcblas.a)
endif()
set(BASELIBS ${LIBS} CACHE STRING "common libraries for building")


add_subdirectory (framework)
add_subdirectory (src)
add_subdirectory (examples)
add_subdirectory (regression_test)
