cmake_minimum_required (VERSION 2.8)

aux_source_directory(. MPR_CORE_SOURCES)

add_library(mpr_core ${MPR_CORE_SOURCES})