cmake_minimum_required (VERSION 2.8)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/src/core/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )

add_compile_options(--std=c++11 -Wno-strict-aliasing -Werror=class-varargs -fpermissive)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Werror=class-varargs -fpermissive")

include_directories(${ZFP_INCLUDE_DIR})

aux_source_directory(. MPR_CORE_SOURCES)

add_library(mpr_core ${MPR_CORE_SOURCES})

target_link_libraries(mpr_core zfp)