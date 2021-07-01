/*
 * mpr.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef INCLUDE_MPR_H_
#define INCLUDE_MPR_H_

#include "../src/MPR_inc.h"

MPR_return_code MPR_file_create(const char* filename, int flags, MPR_access access_type, MPR_point global, MPR_point local, MPR_point offset, MPR_point patch, MPR_file* file);

MPR_return_code MPR_file_open(const char* filename, int flags, MPR_access access_type, MPR_point global, MPR_point local, MPR_point offset, MPR_file* file);

MPR_return_code MPR_close(MPR_file file);

MPR_return_code MPR_flush(MPR_file file);

MPR_return_code MPR_default_bits_per_datatype(MPR_data_type type, int* bits);

MPR_return_code MPR_values_per_datatype(MPR_data_type type, int* values, int* bits);

MPR_return_code MPR_set_variable_count(MPR_file file, int  variable_count);

MPR_return_code MPR_get_variable_count(MPR_file file, int* variable_count);

MPR_return_code MPR_set_current_time_step(MPR_file file, const int current_time_step);

MPR_return_code MPR_get_current_time_step(MPR_file file, int* current_time_step);

MPR_return_code MPR_set_last_time_step(MPR_file file, const int last_time_step);

MPR_return_code MPR_set_out_file_num(MPR_file file, int num);

MPR_return_code MPR_set_io_mode(MPR_file file, enum MPR_io_type io_type);

MPR_return_code MPR_get_io_mode(MPR_file file, enum MPR_io_type* io_type);

MPR_return_code MPR_variable_create(char* variable_name, unsigned int bits_per_sample, MPR_data_type type_name, MPR_variable* variable);

MPR_return_code MPR_append_and_write_variable(MPR_file file, MPR_variable variable);

MPR_return_code MPR_set_current_variable_index(MPR_file file, int variable_index);

MPR_return_code MPR_get_current_variable(MPR_file file, MPR_variable* variable);

MPR_return_code MPR_variable_write_data(MPR_variable variable, const void* buffer);

MPR_return_code MPR_variable_buffer_cleanup(MPR_file file, int svi, int evi);

MPR_return_code MPR_set_global_offset(MPR_file file, int* offset);

MPR_return_code MPR_set_compression_mode(MPR_file file, int mode);

MPR_return_code MPR_set_compression_parameter(MPR_file file, float param);

MPR_return_code MPR_set_read_level(MPR_file file, int read_level);

MPR_return_code MPR_set_is_write(MPR_file file, int is_write);

MPR_return_code MPR_set_aggregation_mode(MPR_file file, float mode);

MPR_return_code MPR_set_logs(MPR_file file, int log);

#endif /* INCLUDE_MPR_H_ */
