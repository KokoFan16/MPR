cmake_minimum_required (VERSION 2.8)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/src/data_handle/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )

add_compile_options(--std=c++11 -Wno-strict-aliasing -Werror=class-varargs -fpermissive)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Werror=class-varargs -fpermissive")

aux_source_directory(. MPR_DATA_HANDLE_SOURCES)

add_library(mpr_data_handle ${MPR_DATA_HANDLE_SOURCES})