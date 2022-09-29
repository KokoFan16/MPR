cmake_minimum_required (VERSION 2.8)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/src/comm/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-std=c++11")

add_compile_options(--std=c++11 -Wno-strict-aliasing -Werror=class-varargs -fpermissive)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Werror=class-varargs -fpermissive")

aux_source_directory(. MPR_COMM_SOURCES)

add_library(mpr_comm ${MPR_COMM_SOURCES})