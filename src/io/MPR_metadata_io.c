/*
 * metadata_io.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */


#include "../MPR_inc.h"
#include <errno.h>
#include <sys/types.h>

MPR_return_code MPR_create_folder_structure(MPR_file file, int start_var_index, int end_var_index)
{
	char* file_name = file->mpr->filename;

	char *directory_path;
	char *data_set_path;

	directory_path = malloc(sizeof(*directory_path) * PATH_MAX);
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file_name, strlen(file_name) - 4);

	data_set_path = malloc(sizeof(*data_set_path) * PATH_MAX);
	memset(data_set_path, 0, sizeof(*data_set_path) * PATH_MAX);

	char time_template[512];
	sprintf(time_template, "%%s/%s", file->mpr->filename_time_template);
	sprintf(data_set_path, time_template, directory_path, file->mpr->current_time_step);
	free(directory_path);

	char last_path[PATH_MAX] = {0};
	char this_path[PATH_MAX] = {0};
	char tmp_path[PATH_MAX] = {0};
	char* pos;
	if (file->comm->simulation_rank == 0)
	{
		strcpy(this_path, data_set_path);
		if ((pos = strrchr(this_path, '/')))
		{
			pos[1] = '\0';
			if (strcmp(this_path, last_path) != 0)
			{
				/* make sure if this directory exists */
				strcpy(last_path, this_path);
				memset(tmp_path, 0, PATH_MAX * sizeof (char));
				/* walk up path and mkdir each segment */
				for (int j = 0; j < (int)strlen(this_path); j++)
				{
					if (j > 0 && this_path[j] == '/')
					{
						int ret = mkdir(tmp_path, S_IRWXU | S_IRWXG | S_IRWXO);
						if (ret != 0 && errno != EEXIST)
						{
							fprintf(stderr, "Error: failed to mkdir %s\n", tmp_path);
							return MPR_err_file;
						}
					}
					tmp_path[j] = this_path[j];
				}
			}
		}
	}
	MPI_Barrier(file->comm->simulation_comm);
	free(data_set_path);
	return MPR_success;
}

