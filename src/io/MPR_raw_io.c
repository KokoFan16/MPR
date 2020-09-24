/*
 * raw_io.c
 *
 *  Created on: Aug 6, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_raw_write(MPR_file file, int svi, int evi)
{
	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_raw_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_raw_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();
	return MPR_success;
}

MPR_return_code MPR_raw_write_data_out(MPR_file file, int svi, int evi)
{
	/* The size of each patch */
	int patch_size =  file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2];

	/* If the aggregation mode isn't set */
	if (file->mpr->aggregation_mode == -1)
	{
		file->time->agg_start = 0; /* no aggregation */
		file->time->agg_end = 0;
		file->mpr->is_aggregator = 1; /* all the processes are aggregators */
		for (int v = svi; v < evi; v++)
		{
			unsigned long long offset = 0;
			MPR_local_patch local_patch = file->variable[v]->local_patch;

			int patch_count = local_patch->patch_count; /* patch count per process */
			int bits = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */
			unsigned long long out_file_size = patch_count * patch_size * bits;
			local_patch->agg_patch_count = patch_count;

			local_patch->patch_id_array = malloc(patch_count*sizeof(int));
			local_patch->agg_patch_disps = malloc(patch_count*sizeof(unsigned long long));

			local_patch->out_file_size = out_file_size;
			local_patch->buffer = malloc(out_file_size);
			for (int i = 0; i < patch_count; i++)
			{
				local_patch->patch_id_array[i] = local_patch->patch[i]->global_id;
				local_patch->agg_patch_disps[i] = offset;
				memcpy(&local_patch->buffer[offset], local_patch->patch[i]->buffer, patch_size * bits);
				offset += patch_size;
			}
		}
	}
	else   /* Aggregation */
	{
		file->time->agg_start = MPI_Wtime();
		int ret = MPR_aggregation(file, svi, evi);
		if (ret != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
		file->time->agg_end = MPI_Wtime();
	}

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

	/* Write file */
	if (file->mpr->is_aggregator == 1)
	{
		unsigned long long out_offset = 0; /* the offset for each patch */
		for (int v = svi; v < evi; v++)
		{
			MPR_local_patch local_patch = file->variable[v]->local_patch;
			int fp = open(file_name, O_CREAT | O_WRONLY | O_APPEND, 0664);
			int write_count = pwrite(fp, local_patch->buffer, local_patch->out_file_size, out_offset);
			if (write_count != local_patch->out_file_size)
			{
			  fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
			  return MPR_err_io;
			}
			for (int i = 0; i < local_patch->agg_patch_count; i++)
				local_patch->agg_patch_disps[i] += out_offset;
			out_offset += local_patch->out_file_size;
			close(fp);
		}
	}
	free(file_name);
	free(directory_path);
	return MPR_success;
}

MPR_return_code MPR_metadata_raw_write_out(MPR_file file, int svi, int evi)
{
	/* Write basic information out */
	if (MPR_basic_info_metadata_write_out(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	if (MPR_out_file_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	return MPR_success;
}

