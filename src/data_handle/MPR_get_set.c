/*
 * MPR_get_set.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

MPR_return_code MPR_set_variable_count(MPR_file file, int variable_count)
{
  if (!file)
    return MPR_err_file;

  if (variable_count <= 0)
    return MPR_err_count;

  file->mpr->variable_count = variable_count;

  return MPR_success;
}


MPR_return_code MPR_get_variable_count(MPR_file file, int* variable_count)
{
  if (!file)
    return MPR_err_file;

  *variable_count = file->mpr->variable_count;

  return MPR_success;
}

MPR_return_code MPR_set_current_time_step(MPR_file file, const int current_time_step)
{
  if (!file)
    return MPR_err_file;

  if (current_time_step < 0)
    return MPR_err_time;

  file->mpr->current_time_step = current_time_step;

  return MPR_success;
}

MPR_return_code MPR_set_last_time_step(MPR_file file, const int last_time_step)
{
  if (!file)
    return MPR_err_file;

  if (last_time_step < 0)
    return MPR_err_time;

  file->mpr->last_tstep = last_time_step;

  return MPR_success;
}

MPR_return_code MPR_get_current_time_step(MPR_file file, int* current_time_step)
{
  if (!file)
    return MPR_err_file;

  *current_time_step = file->mpr->current_time_step;

  return MPR_success;
}

MPR_return_code MPR_set_out_file_num(MPR_file file, int num)
{
	if (!file)
      return MPR_err_file;

    file->mpr->out_file_num = num;

    return MPR_success;
}


MPR_return_code MPR_set_compression_mode(MPR_file file, int mode)
{
	if (!file)
      return MPR_err_file;

	file->mpr->compression_type = mode;

    return MPR_success;
}


MPR_return_code MPR_set_compression_parameter(MPR_file file, float param)
{
	if (!file)
      return MPR_err_file;

	file->mpr->compression_param = param;

    return MPR_success;
}


MPR_return_code MPR_set_io_mode(MPR_file file, enum MPR_io_type io_type)
{
  if (!file)
    return MPR_err_file;

  file->mpr->io_type = io_type;

  return MPR_success;
}

MPR_return_code MPR_get_io_mode(MPR_file file, enum MPR_io_type* io_type)
{
  if (!file)
    return MPR_err_file;

  *io_type = file->mpr->io_type;

  return MPR_success;
}


MPR_return_code MPR_set_global_offset(MPR_file file, int* offset)
{
	if (!file)
		return MPR_err_file;

	if (offset != NULL)
		memcpy(file->mpr->global_offset, offset, MPR_MAX_DIMENSIONS * sizeof(int));

	return MPR_success;
}


MPR_return_code MPR_set_read_level(MPR_file file, int read_level)
{
  if (!file)
    return MPR_err_file;

  file->mpr->read_level = read_level;

  return MPR_success;
}


MPR_return_code MPR_set_is_write(MPR_file file, int is_write)
{
  if (!file)
    return MPR_err_file;

  file->mpr->is_write = is_write;

  return MPR_success;
}


MPR_return_code MPR_set_aggregation_mode(MPR_file file, float mode)
{
	if (!file)
      return MPR_err_file;

	file->mpr->is_fixed_file_size = mode;

    return MPR_success;
}


MPR_return_code MPR_set_logs(MPR_file file, int log)
{
	if (!file)
      return MPR_err_file;

	file->mpr->is_logs = log;

    return MPR_success;
}



