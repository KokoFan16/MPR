cmake_minimum_required (VERSION 2.8)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/src/utils/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )

aux_source_directory(. MPR_UTILS_SOURCES)

add_library(mpr_utils ${MPR_UTILS_SOURCES})