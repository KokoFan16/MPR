/*
 * MPR_variable.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

MPR_return_code MPR_variable_create(char* variable_name, unsigned int bits_per_sample, MPR_data_type type_name, MPR_variable* variable)
{
  if (!variable_name)
    return MPR_err_name;

  if (bits_per_sample <= 0)
    return MPR_err_size;

  if (!type_name)
    return MPR_err_type;

  *variable = malloc(sizeof *(*variable));
  memset(*variable, 0, sizeof *(*variable));

  (*variable)->local_patch = malloc(sizeof(*((*variable)->local_patch)));
  memset((*variable)->local_patch, 0, sizeof (*((*variable)->local_patch)));

  int bits = 0;
  MPR_default_bits_per_datatype(type_name, &bits);
  if (bits !=0 && bits != bits_per_sample)
    return MPR_err_type;

  (*variable)->vps = 1;
  (*variable)->bpv = (bits_per_sample/1);

  strcpy((*variable)->type_name, type_name);
  strcpy((*variable)->var_name, variable_name);

  return MPR_success;
}

MPR_return_code MPR_variable_write_data(MPR_variable variable, const void* buffer)
{
	if (!variable)
		return MPR_err_variable;

	const void *temp_buffer = buffer;

//	variable->local_patch->buffer = (unsigned char*)temp_buffer;

	return MPR_success;
}

MPR_return_code MPR_append_and_write_variable(MPR_file file, MPR_variable variable)
{

  if (!file)
    return MPR_err_file;

  if (!variable)
    return MPR_err_variable;

  if (file->variable_index_tracker >= file->mpr->variable_count)
    return MPR_err_variable;

  file->variable[file->variable_index_tracker] = variable;

  file->variable_index_tracker++;
  file->local_variable_count++;

  return MPR_success;
}
