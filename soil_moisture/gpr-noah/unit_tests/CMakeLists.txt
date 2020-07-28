include_directories(${CMAKE_SOURCE_DIR}/framework ${GSL_HOME}/include ${CPPUNIT_HOME}/include)

add_executable(run_unit_tests run_unit_tests.cpp)
add_custom_command(TARGET run_unit_tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/unit_tests/*.bin $<TARGET_FILE_DIR:run_unit_tests>)
target_link_libraries(run_unit_tests ${BASELIBS} ${CPPUNIT_HOME}/lib/libcppunit.a)
