/*
 * MPR_log_io.c
 *
 *  Created on: Jan 20, 2021
 *      Author: kokofan
 */

#include "../MPR_inc.h"
#include <errno.h>


MPR_return_code MPR_timing_logs(MPR_file file, int svi, int evi)
{
	int rank = file->comm->simulation_rank;
	int MODE = file->mpr->io_type;

	double total_time = file->time->total_end - file->time->total_start;
	double rst_time = file->time->rst_end - file->time->rst_start;
	double wave_time = file->time->wave_end - file->time->wave_start;
	double comp_time = file->time->zfp_end - file->time->zfp_start;

	double max_total_time = 0;
	MPI_Allreduce(&total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, file->comm->simulation_comm);

	char time_folder[512];

	if (file->flags == MPR_MODE_CREATE)
	{
		char* directory_path = malloc(512);
		memset(directory_path, 0, sizeof(*directory_path) * 512);
		strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

		sprintf(time_folder, "time_write_%s", directory_path);
		free(directory_path);
	}

	if (file->flags == MPR_MODE_RDONLY)
		sprintf(time_folder, "time_read_%s", file->mpr->filename);

	long len = strlen(time_folder);
	time_folder[len] = '\0';

	if (file->mpr->current_time_step == 0)
	{
		if (rank == 0)
		{
			int ret = mkdir(time_folder, S_IRWXU | S_IRWXG | S_IRWXO);
			if (ret != 0 && errno != EEXIST)
				fprintf(stderr, "Error: failed to mkdir %s\n", time_folder);
		}
	}

	MPI_Barrier(file->comm->simulation_comm);


	if (file->flags == MPR_MODE_CREATE)
	{
		if (file->mpr->current_time_step == 0)
		{
			int file_size = 0;
			for (int v = svi; v < evi; v++)
			{
				MPR_local_patch local_patch = file->variable[v]->local_patch;
				file_size += local_patch->out_file_size;

				if (rank == 0 && file->mpr->current_time_step == 0)
					printf("The compression ratio for variable %d is %f.\n", v, file->variable[v]->local_patch->compression_ratio);
			}
		}

		double agg_time = file->time->agg_end - file->time->agg_start;
		double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;
		double wrt_metadata_time = file->time->wrt_metadata_end - file->time->wrt_metadata_start;

		char time_log[512];
		sprintf(time_log, "%s/time_%d", time_folder, rank);

		if (file->mpr->is_aggregator == 1)
		{
			FILE* fp = fopen(time_log, "a"); /* open file */
		    if (!fp) /* Check file handle */
				fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);
		    fprintf(fp,"%d %d: [%f] >= [rst %f wave %f comp %f agg %f w_dd %f w_meda %f]\n", file->mpr->current_time_step, rank, total_time, rst_time, wave_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);
		    fclose(fp);
		}
	}

	if (file->flags == MPR_MODE_RDONLY)
	{
		double parse_bound = file->time->parse_bound_end - file->time->parse_bound_start;
		double read_time = file->time->read_end - file->time->read_start;

		char time_log[512];
		sprintf(time_log, "%s/time_%d", time_folder, rank);

		FILE* fp = fopen(time_log, "a"); /* open file */
	    if (!fp) /* Check file handle */
			fprintf(stderr, " [%s] [%d] time_dir is corrupt.\n", __FILE__, __LINE__);
	    fprintf(fp,"%d: [%f] >= [meta %f read %f comp %f wave %f rst %f]\n", rank, total_time, parse_bound, read_time, comp_time, wave_time, rst_time);
	    fclose(fp);
	}

	MPI_Barrier(file->comm->simulation_comm);

	return MPR_success;
}


MPR_return_code MPR_logs(MPR_file file, int svi, int evi)
{
	int rank = file->comm->simulation_rank;

	if (file->flags == MPR_MODE_CREATE)
	{
		char log_folder[512];

		char* directory_path = malloc(512);
		memset(directory_path, 0, sizeof(*directory_path) * 512);
		strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

		sprintf(log_folder, "log_%s", directory_path);
		free(directory_path);

		long len = strlen(log_folder);
		log_folder[len] = '\0';

		if (file->mpr->current_time_step == 0)
		{
			if (rank == 0)
			{
				int ret = mkdir(log_folder, S_IRWXU | S_IRWXG | S_IRWXO);
				if (ret != 0 && errno != EEXIST)
					fprintf(stderr, "Error: failed to mkdir %s\n", log_folder);

			}
		}
		MPI_Barrier(file->comm->simulation_comm);

		char log_file[512];
		sprintf(log_file, "%s/log_%d", log_folder, rank);

		FILE* fp = fopen(log_file, "a"); /* open file */
	    if (!fp) /* Check file handle */
	    {
			fprintf(stderr, " [%s] [%d] log_dir is corrupt.\n", __FILE__, __LINE__);
			return -1;
	    }
		for (int v = svi; v < evi; v++)
		{
			MPR_local_patch local_patch = file->variable[v]->local_patch;
			if (file->mpr->is_aggregator == 1)
			{
				fprintf(fp, "%d, A %d, count %d, size %lld, msize %d\n", file->mpr->current_time_step, rank, local_patch->agg_patch_count, local_patch->out_file_size, file->mpr->file_meta_size);

				for (int i = 0; i < local_patch->agg_patch_count; i++)
				{
					fprintf(fp, "%d, P %d, size %d\n", file->mpr->current_time_step, local_patch->agg_patch_id_array[i], local_patch->agg_patch_size[i]);
				}
			}
			fprintf(fp, "%d, R %d, count %d, size %lld\n", file->mpr->current_time_step, rank, local_patch->patch_count, local_patch->proc_size);
		}
		fclose(fp);
	}

	MPI_Barrier(file->comm->simulation_comm);

	return MPR_success;
}
