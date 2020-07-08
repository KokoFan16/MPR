cmake_minimum_required (VERSION 2.8)

aux_source_directory(. MPR_IO_SOURCES)

add_library(mpr_io ${MPR_IO_SOURCES})