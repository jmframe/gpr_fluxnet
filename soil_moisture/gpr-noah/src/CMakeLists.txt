include_directories(${GSL_HOME}/include)

file(GLOB FILES *.F90)
add_library(noah ${FILES})
