cmake_minimum_required (VERSION 2.8)

#set_source_files_properties(multiple_resolution_precision_write.c PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-std=c++11")

file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/examples/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-std=c++11")


#add_executable(raw_write ./raw_write.c)
#add_executable(multi_pre_write ./multiple_precision_write.c)
#add_executable(multi_res_write ./multiple_resolution_write.c)
add_executable(multi_res_pre_write ./multiple_resolution_precision_write.c)

#add_executable(raw_read ./raw_read.c)
#add_executable(multi_res_read ./multiple_resolution_read.c)
#add_executable(multi_pre_read ./multiple_precision_read.c)
#add_executable(multi_res_pre_read ./multiple_resolution_precision_read.c)

#add_executable(benchmark ./benchmark.c)

include_directories(${ZFP_INCLUDE_DIR})

#target_link_libraries(raw_write mpr zfp)
#target_link_libraries(multi_res_write mpr zfp)
#target_link_libraries(multi_pre_write mpr zfp)
target_link_libraries(multi_res_pre_write mpr zfp)

#target_link_libraries(raw_read mpr zfp)
#target_link_libraries(multi_res_read mpr zfp)
#target_link_libraries(multi_pre_read mpr zfp)
#target_link_libraries(multi_res_pre_read mpr zfp)

#target_link_libraries(benchmark mpr zfp)