cmake_minimum_required (VERSION 2.8)

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