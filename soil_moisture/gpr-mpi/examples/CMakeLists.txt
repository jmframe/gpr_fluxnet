include_directories(${CMAKE_SOURCE_DIR}/framework ${GSL_HOME}/include ${PROJECT_BINARY_DIR}/framework)

add_executable(make_predictions_sample make_predictions_sample.cpp)
target_link_libraries(make_predictions_sample ${BASELIBS})

add_executable(v1_train_gpr_sample v1_train_gpr_sample.cpp)
target_link_libraries(v1_train_gpr_sample ${BASELIBS})

add_executable(train_gpr_sample train_gpr_sample.cpp)
target_link_libraries(train_gpr_sample ${BASELIBS})

add_executable(train_gpr_NIGP train_gpr_NIGP.cpp)
target_link_libraries(train_gpr_NIGP ${BASELIBS})

add_executable(train_gpr_kfold train_gpr_kfold.cpp)
target_link_libraries(train_gpr_kfold ${BASELIBS})

add_executable(sub_sample_statistics sub_sample_statistics.cpp)
target_link_libraries(sub_sample_statistics ${BASELIBS})

add_executable(gpr_fortran_sample gpr_fortran_sample.F90)
target_link_libraries(gpr_fortran_sample ${BASELIBS})
set_target_properties(gpr_fortran_sample PROPERTIES LINKER_LANGUAGE Fortran)

add_executable(train_gpr_sample2 train_gpr_sample2.cpp)
target_link_libraries(train_gpr_sample2 ${BASELIBS})
add_custom_command(TARGET train_gpr_sample2 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/examples/*.bin $<TARGET_FILE_DIR:train_gpr_sample2>)
