cmake_minimum_required (VERSION 2.8)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )

add_compile_options(--std=c++11 -Wno-strict-aliasing -Werror=class-varargs -fpermissive)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Werror=class-varargs -fpermissive")

include_directories(./utils)
include_directories(./comm)
include_directories(./data_handle)
include_directories(./data_structs)
include_directories(./io)
include_directories(./core)

aux_source_directory(. MPR_SOURCES)
add_library(mpr ${MPR_SOURCES})

add_subdirectory(data_handle)
add_subdirectory(utils)
add_subdirectory(comm)
add_subdirectory(io)
add_subdirectory(core)

target_link_libraries(mpr ${MPI_C_LIBRARIES} mpr_data_handle mpr_utils mpr_comm mpr_io mpr_core)