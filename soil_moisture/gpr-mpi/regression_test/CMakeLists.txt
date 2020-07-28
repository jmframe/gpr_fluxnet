include_directories(${CMAKE_SOURCE_DIR}/framework ${GSL_HOME}/include ${PROJECT_BINARY_DIR}/framework)

add_executable(regression_test regression_test.cpp)
add_custom_command(TARGET regression_test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/regression_test/*.bin $<TARGET_FILE_DIR:regression_test>)
target_link_libraries(regression_test ${BASELIBS})

#add_executable(regression_test_f regression_test.F90)
#target_link_libraries(regression_test_f ${BASELIBS})
