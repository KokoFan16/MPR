cmake_minimum_required (VERSION 2.8)

add_executable(raw_write ./raw_write.c)
add_executable(multiple_precision_write ./multiple_precision_write.c)
add_executable(multiple_resolution_write ./multiple_resolution_write.c)

include_directories(${ZFP_INCLUDE_DIR})

target_link_libraries(raw_write mpr zfp)
target_link_libraries(multiple_precision_write mpr zfp)
target_link_libraries(multiple_resolution_write mpr zfp)
