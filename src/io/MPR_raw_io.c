/*
 * raw_io.c
 *
 *  Created on: Aug 6, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_raw_write(MPR_file file, int svi, int evi)
{
	/* the directory patch for out files */
	char *directory_path;
	directory_path = malloc(sizeof(*directory_path) * PATH_MAX);
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

	/* The file name for out files */
	char *file_name;
	file_name = malloc(PATH_MAX * sizeof(*file_name));
	memset(file_name, 0, PATH_MAX * sizeof(*file_name));
	sprintf(file_name, "%s/time%09d/%d", directory_path, file->mpr->current_time_step, file->comm->simulation_rank);

	int bits = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */
	/* The size of each patch */
	int buffer_size =  file->restructured_patch->patch_size[0] * file->restructured_patch->patch_size[1] * file->restructured_patch->patch_size[2] * bits;
	int patch_count = file->variable[svi]->local_patch->patch_count; /* patch count per process */
	int out_offset = 0; /* the offset for each patch */

	/* If the aggregation mode isn't set */
	if (file->mpr->aggregation_mode == -1)
	{
		for (int i = 0; i < patch_count; i++)
		{
			for (int v = svi; v < evi; v++)
			{
				/* Write file */
				int fp = open(file_name, O_CREAT | O_WRONLY | O_APPEND, 0664);
				int write_count = pwrite(fp, file->variable[v]->local_patch->patch[i]->buffer, buffer_size, out_offset);
				if (write_count != buffer_size)
				{
				  fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
				  return MPR_err_io;
				}
				close(fp);
				out_offset += buffer_size;
			}
		}
	}
	else   /* Aggregation */
	{
		int ret = MPR_aggregation(file, svi, evi);

		if (ret != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}

	free(file_name);
	free(directory_path);
	return MPR_success;
}

