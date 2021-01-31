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

	if (file->flags == MPR_MODE_CREATE)
	{
		double agg_time = file->time->agg_end - file->time->agg_start;
		double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;

		time_buffer[file->mpr->current_time_step*8] = file->mpr->current_time_step;
		time_buffer[file->mpr->current_time_step*8+1] = rank;
		time_buffer[file->mpr->current_time_step*8+2] = total_time;
		time_buffer[file->mpr->current_time_step*8+3] = rst_time;
		time_buffer[file->mpr->current_time_step*8+4] = wave_time;
		time_buffer[file->mpr->current_time_step*8+5] = comp_time;
		time_buffer[file->mpr->current_time_step*8+6] = agg_time;
		time_buffer[file->mpr->current_time_step*8+7] = wrt_data_time;

		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
		{
			int count = file->mpr->last_tstep * 8;
			float* total_time_buffer = NULL;
			if (rank == 0)
				total_time_buffer = malloc(count*sizeof(float)*file->comm->simulation_nprocs);

			MPI_Gather(time_buffer, count, MPI_FLOAT, total_time_buffer, count, MPI_FLOAT, 0, file->comm->simulation_comm);

			if (rank == 0)
			{
				char* directory_path = malloc(512);
				memset(directory_path, 0, sizeof(*directory_path) * 512);
				strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

				char time_log[512];
				sprintf(time_log, "time_write_%s_log", directory_path);
				free(directory_path);

				FILE* fp = fopen(time_log, "w"); /* open file */
				if (!fp) /* Check file handle */
					fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);

				int loop_count = file->comm->simulation_nprocs * file->mpr->last_tstep;
				for (int i = 0; i < loop_count; i++)
					fprintf(fp, "%d %d: [%f] >= [part %f wave %f comp %f agg %f io %f]\n", (int)total_time_buffer[i*8+0], (int)total_time_buffer[i*8+1], total_time_buffer[i*8+2], total_time_buffer[i*8+3], total_time_buffer[i*8+4], total_time_buffer[i*8+5], total_time_buffer[i*8+6], total_time_buffer[i*8+7]);

				fclose(fp);
			}
			free(total_time_buffer);
		}
	}


	if (file->flags == MPR_MODE_RDONLY)
	{
		time_buffer = malloc(6*sizeof(float));

		double read_time = file->time->read_end - file->time->read_start;

		time_buffer[0] = rank;
		time_buffer[1] = total_time;
		time_buffer[2] = read_time;
		time_buffer[3] = comp_time;
		time_buffer[4] = wave_time;
		time_buffer[5] = rst_time;

		int count = 6;
		float* total_time_buffer = NULL;
		if (file->comm->simulation_rank == 0)
			total_time_buffer = malloc(count*sizeof(float)*file->comm->simulation_nprocs);

		MPI_Gather(time_buffer, count, MPI_FLOAT, total_time_buffer, count, MPI_FLOAT, 0, file->comm->simulation_comm);

		if (rank == 0)
		{
			char time_log[512];
			sprintf(time_log, "time_read_%s_log", file->mpr->filename);

			FILE* fp = fopen(time_log, "w"); /* open file */
			if (!fp) /* Check file handle */
				fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);

			int loop_count = file->comm->simulation_nprocs;
			for (int i = 0; i < loop_count; i++)
				fprintf(fp,"%d: [%f] >= [read %f comp %f wave %f rst %f]\n", (int)total_time_buffer[i*6+0], total_time_buffer[i*6+1], total_time_buffer[i*6+2], total_time_buffer[i*6+3], total_time_buffer[i*6+4], total_time_buffer[i*6+5]);

			fclose(fp);
		}

		free(total_time_buffer);
		free(time_buffer);
	}

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

//				for (int i = 0; i < local_patch->agg_patch_count; i++)
//				{
//					fprintf(fp, "%d, P %d, size %d\n", file->mpr->current_time_step, local_patch->agg_patch_id_array[i], local_patch->agg_patch_size[i]);
//				}
			}
			fprintf(fp, "%d, R %d, count %d, size %lld\n", file->mpr->current_time_step, rank, local_patch->patch_count, local_patch->proc_size);
		}
		fclose(fp);
	}

	MPI_Barrier(file->comm->simulation_comm);

	return MPR_success;
}
