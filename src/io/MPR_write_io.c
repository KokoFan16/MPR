/*
 * MPR_write_io.c
 *
 *  Created on: Oct 2, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

/* Write data out with raw write mode */
MPR_return_code MPR_raw_write(MPR_file file, int svi, int evi)
{
	/* Aggregation phase */
	file->time->agg_start = MPI_Wtime();
	if (MPR_aggregation_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->agg_end = MPI_Wtime();

	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();

	return MPR_success;
}


MPR_return_code MPR_multi_res_write(MPR_file file, int svi, int evi)
{
	/* Wavelet transform */
	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_transform_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wave_end = MPI_Wtime();

	/* Aggregation phase */
	file->time->agg_start = MPI_Wtime();
	if (MPR_aggregation_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->agg_end = MPI_Wtime();

	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();

	return MPR_success;
}


/* Write data out with multiple precision mode */
MPR_return_code MPR_multi_pre_write(MPR_file file, int svi, int evi)
{
	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_compression_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->zfp_end = MPI_Wtime();

	/* Aggregation phase */
	file->time->agg_start = MPI_Wtime();
	if (MPR_aggregation_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->agg_end = MPI_Wtime();

	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();

	return MPR_success;
}


MPR_return_code MPR_multi_pre_res_write(MPR_file file, int svi, int evi)
{
	/* Wavelet transform */
	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_transform_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wave_end = MPI_Wtime();

	/* compressed each sub-bands after wavelet transform*/
	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_multi_res_compression_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->zfp_end = MPI_Wtime();

	/* Aggregation phase */
	file->time->agg_start = MPI_Wtime();
	if (MPR_aggregation_perform(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->agg_end = MPI_Wtime();

	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();

	return MPR_success;
}

/* Write data out */
MPR_return_code MPR_write_data_out(MPR_file file, int svi, int evi)
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

	/* Write file */
	if (file->mpr->is_aggregator == 1)
	{
		for (int v = svi; v < evi; v++)
		{
			MPR_local_patch local_patch = file->variable[v]->local_patch;
			int fp = open(file_name, O_CREAT | O_WRONLY | O_APPEND, 0664);
			int write_count = write(fp, local_patch->buffer, local_patch->out_file_size);
			if (write_count != local_patch->out_file_size)
			{
			  fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
			  return MPR_err_io;
			}
			close(fp);
		}
	}
	free(file_name);
	free(directory_path);
	return MPR_success;
}


/* Write meta-data out */
MPR_return_code MPR_metadata_write_out(MPR_file file, int svi, int evi)
{
	/* Write basic information out */
	if (MPR_basic_info_metadata_write_out(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	if (MPR_bounding_box_metadata_write_out(file, svi, evi) != MPR_success)
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
