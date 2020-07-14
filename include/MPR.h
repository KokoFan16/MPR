/*
 * mpr.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef INCLUDE_MPR_H_
#define INCLUDE_MPR_H_

#include "../src/MPR_inc.h"

MPR_return_code MPR_file_create(const char* filename, int flags, MPR_access access_type, MPR_point global, MPR_point local, MPR_point offset, MPR_file* file);

MPR_return_code MPR_close(MPR_file file);

MPR_return_code MPR_flush(MPR_file file);

MPR_return_code MPR_default_bits_per_datatype(MPR_data_type type, int* bits);

MPR_return_code MPR_values_per_datatype(MPR_data_type type, int* values, int* bits);

MPR_return_code MPR_set_variable_count(MPR_file file, int  variable_count);

MPR_return_code MPR_get_variable_count(MPR_file file, int* variable_count);

MPR_return_code MPR_set_current_time_step(MPR_file file, const int current_time_step);

MPR_return_code MPR_get_current_time_step(MPR_file file, int* current_time_step);

MPR_return_code MPR_set_restructuing_factor(MPR_file file, float rst_factor_x, float rst_factor_y, float rst_factor_z);

MPR_return_code MPR_get_restructuing_factor(MPR_file file, float* rst_factor_x, float* rst_factor_y, float* rst_factor_z);

MPR_return_code MPR_set_required_file_param(MPR_file file, unsigned long long rfp);

MPR_return_code MPR_get_required_file_param(MPR_file file, unsigned long long* rfp);

MPR_return_code MPR_set_io_mode(MPR_file file, enum MPR_io_type io_type);

MPR_return_code MPR_get_io_mode(MPR_file file, enum MPR_io_type* io_type);

MPR_return_code MPR_set_restructuring_box(MPR_file file, MPR_point reg_patch_size);

MPR_return_code MPR_get_restructuring_box(MPR_file file, MPR_point reg_patch_size);

MPR_return_code MPR_variable_create(char* variable_name, unsigned int bits_per_sample, MPR_data_type type_name, MPR_variable* variable);

MPR_return_code MPR_append_and_write_variable(MPR_file file, MPR_variable variable);

MPR_return_code MPR_variable_write_data(MPR_variable variable, const void* buffer);

#endif /* INCLUDE_MPR_H_ */
