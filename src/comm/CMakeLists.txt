cmake_minimum_required (VERSION 2.8)

aux_source_directory(. MPR_COMM_SOURCES)

add_library(mpr_comm ${MPR_COMM_SOURCES})