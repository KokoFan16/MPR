cmake_minimum_required (VERSION 2.8)

aux_source_directory(. MPR_DATA_HANDLE_SOURCES)

add_library(mpr_data_handle ${MPR_DATA_HANDLE_SOURCES})