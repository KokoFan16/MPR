cmake_minimum_required (VERSION 2.8)

project (MPR)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set (CMAKE_CXX_STANDARD 11)

message("Configuring MPR with MPI support")
find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
include_directories(SYSTEM ${ZFP_INCLUDE_DIR})

include_directories("${PROJECT_SOURCE_DIR}/external")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/examples")

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(examples)

message("Configuring caliper")
find_package(caliper REQUIRED PATHS ${CALIPER_DIR})
include_directories(SYSTEM ${caliper_INCLUDE_DIR})
