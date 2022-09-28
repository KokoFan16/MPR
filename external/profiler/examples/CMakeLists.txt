cmake_minimum_required (VERSION 3.9)

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/examples/*.cpp")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-std=c++17")

add_executable(lifegame ./lifegame_mpi.cpp)

target_link_libraries(lifegame HVIS_src)