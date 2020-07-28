include_directories(${GSL_HOME}/include)

if(MPI_DISABLED)
add_library(matrix_utils comm_serial.cpp comm_mod.F90 timer_extern_C_routines.cpp timer_wrapper_module.F90 scalapack_matrix.cpp options.cpp option_extern_C_routines.cpp option_wrapper_module.F90 gpr_extern_C_routines.cpp gpr_wrapper_module.F90)
else()
add_library(matrix_utils comm_mpi.cpp comm_mod.F90 timer_extern_C_routines.cpp timer_wrapper_module.F90 scalapack_matrix.cpp options.cpp option_extern_C_routines.cpp option_wrapper_module.F90 gpr_extern_C_routines.cpp gpr_wrapper_module.F90)
endif()
