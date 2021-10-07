/*
 * MPR_write_io.c
 *
 *  Created on: Oct 2, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_multi_res_write(MPR_file file, int svi, int evi)
{
	/* Wavelet transform */
//	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_transform_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
//	file->time->wave_end = MPI_Wtime();

	return MPR_success;
}


/* Write data out with multiple precision mode */
MPR_return_code MPR_multi_pre_write(MPR_file file, int svi, int evi)
{
	/* Perform zfp compression */
//	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_compression_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
//	file->time->zfp_end = MPI_Wtime();

	return MPR_success;
}


MPR_return_code MPR_multi_pre_res_write(MPR_file file, int svi, int evi)
{
	/* Wavelet transform */
//	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_transform_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
//	file->time->wave_end = MPI_Wtime();

	/* compressed each sub-bands after wavelet transform*/
//	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_multi_res_compression_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
//	file->time->zfp_end = MPI_Wtime();

	return MPR_success;
}

/* Write data out */
MPR_return_code MPR_write_data_out(MPR_file file, int svi, int evi)
{
	Events e("wrtData", "io");

	/* the directory patch for out files */
	char *directory_path;
	directory_path = (char*)malloc(sizeof(*directory_path) * PATH_MAX);
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

	/* The file name for out files */
	char file_name[512];
	memset(file_name, 0, 512 * sizeof(*file_name));
	sprintf(file_name, "%s/time%09d/%d", directory_path, file->mpr->current_time_step, file->comm->simulation_rank);
	free(directory_path);

	/* Write file */
	if (file->mpr->is_aggregator == 1)
	{
		int fp = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0664);
		if (fp == -1)
		{
			fprintf(stderr, "File %s is existed, please delete it.\n", file_name);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		int write_meta_size = write(fp, file->mpr->file_meta_buffer, file->mpr->file_meta_size);
		if (write_meta_size != file->mpr->file_meta_size)
		{
			fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		free(file->mpr->file_meta_buffer);

		for (int v = svi; v < evi; v++)
		{
			MPR_local_patch local_patch = file->variable[v]->local_patch;
			long long int write_file_size = write(fp, local_patch->buffer, local_patch->out_file_size);
			if (write_file_size != local_patch->out_file_size)
			{
			  fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
			  return MPR_err_io;
			}
		}
		close(fp);
	}
	return MPR_success;
}

