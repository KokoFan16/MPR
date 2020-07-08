cmake_minimum_required (VERSION 2.8)

add_executable(raw_write ./raw_write.c)

target_link_libraries(raw_write mpr)
