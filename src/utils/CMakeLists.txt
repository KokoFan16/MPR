cmake_minimum_required (VERSION 2.8)

aux_source_directory(. MPR_UTILS_SOURCES)

add_library(mpr_utils ${MPR_UTILS_SOURCES})