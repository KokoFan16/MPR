/*
 * MPR_close.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

MPR_return_code MPR_flush(MPR_file file)
{
	/* making sure that variables are added to the dataset */
	if (file->mpr->variable_count <= 0)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_variable;
	}

	// index range of variables within a flush
	int lvi = file->local_variable_index;
	int lvc = file->local_variable_count;

	if (file->flags == MPR_MODE_CREATE)
	{
		if (MPR_write(file, lvi, (lvi + lvc), file->mpr->io_type) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_io;
		}
	}

	return MPR_success;
}

MPR_return_code MPR_close(MPR_file file)
{
	if (MPR_flush(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_flush;
	}
	/* Clean Up */
	for (int j = 0; j < file->mpr->variable_count; j++)
	{
		free(file->variable[j]);
		file->variable[j] = 0;
	}
	file->mpr->variable_count = 0;

	free(file->mpr);
	free(file->time);
	free(file->comm);
	free(file);

	return MPR_success;
}
