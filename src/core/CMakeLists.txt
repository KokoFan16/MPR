cmake_minimum_required (VERSION 2.8)

include_directories(${ZFP_INCLUDE_DIR})

aux_source_directory(. MPR_CORE_SOURCES)

add_library(mpr_core ${MPR_CORE_SOURCES})

target_link_libraries(mpr_core zfp)