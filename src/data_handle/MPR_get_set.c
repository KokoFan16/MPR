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

MPR_return_code MPR_get_current_time_step(MPR_file file, int* current_time_step)
{
  if (!file)
    return MPR_err_file;

  *current_time_step = file->mpr->current_time_step;

  return MPR_success;
}


MPR_return_code MPR_set_restructuing_factor(MPR_file file, float rst_factor_x, float rst_factor_y, float rst_factor_z)
{
  if (!file)
    return MPR_err_file;

  file->mpr->restructuring_factor[0] = rst_factor_x;
  file->mpr->restructuring_factor[1] = rst_factor_y;
  file->mpr->restructuring_factor[2] = rst_factor_z;

  return MPR_success;
}

MPR_return_code MPR_get_restructuing_factor(MPR_file file, float* rst_factor_x, float* rst_factor_y, float* rst_factor_z)
{
  if (!file)
    return MPR_err_file;

  *rst_factor_x = file->mpr->restructuring_factor[0];
  *rst_factor_y = file->mpr->restructuring_factor[1];
  *rst_factor_z = file->mpr->restructuring_factor[2];

  return MPR_success;
}

MPR_return_code MPR_set_required_file_param(MPR_file file, unsigned long long rfp)
{
	if (!file)
      return MPR_err_file;

    file->mpr->required_file_param = rfp;

    return MPR_success;
}

MPR_return_code MPR_get_required_file_param(MPR_file file, unsigned long long* rfp)
{
	if (!file)
      return MPR_err_file;

    *rfp = file->mpr->required_file_param;

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

MPR_return_code MPR_set_restructuring_box(MPR_file file, MPR_point reg_patch_size)
{
  if (!file)
    return MPR_err_file;

  memcpy(file->restructured_patch->patch_size, reg_patch_size, MPR_MAX_DIMENSIONS * sizeof(int));

  return MPR_success;
}

MPR_return_code MPR_get_restructuring_box(MPR_file file, MPR_point reg_patch_size)
{
  if (!file)
    return MPR_err_file;

  memcpy(reg_patch_size, file->restructured_patch->patch_size, MPR_MAX_DIMENSIONS * sizeof(int));

  return MPR_success;
}




