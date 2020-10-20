/*
 * MPR_read_io.c
 *
 *  Created on: Oct 19, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

/* Read data with raw I/O mode */
MPR_return_code MPR_raw_read(MPR_file file, int svi)
{
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}


/* Read data with multiple resolution I/O mode */
MPR_return_code MPR_multi_res_read(MPR_file file, int svi)
{
	return MPR_success;
}


/* Read data with multiple precision mode */
MPR_return_code MPR_multi_pre_read(MPR_file file, int svi)
{
	return MPR_success;
}


/* Read data with multiple resolution and precision mode */
MPR_return_code MPR_multi_pre_res_read(MPR_file file, int svi)
{
	return MPR_success;
}

/* Read data with meta-data */
MPR_return_code MPR_read_data(MPR_file file, int svi)
{
	/* The file name for out files */
	char file_name[PATH_MAX];
	memset(file_name, 0, PATH_MAX * sizeof(*file_name));

	int file_num = file->mpr->open_file_num; /* the number of files need to be opened for each process */

	for (int i = 0; i < file_num; i++)
	{
		sprintf(file_name, "%s/time%09d/%d", file->mpr->filename, file->mpr->current_time_step, file->mpr->open_file_ids[i]);
		MPR_file_related_metadata_parse(file_name, file, svi);  /* parse file related meta-data */
	}

	return MPR_success;
}
