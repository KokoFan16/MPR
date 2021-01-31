/*
 * MPR_log_io.c
 *
 *  Created on: Jan 20, 2021
 *      Author: kokofan
 */

#include "../MPR_inc.h"


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
		int time_count = 9;

		double agg_time = file->time->agg_end - file->time->agg_start;
		double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;

		int offset = file->mpr->current_time_step * time_count;

		if (file->mpr->is_aggregator == 1)
		{
			time_buffer[offset] = file->mpr->is_aggregator;
			time_buffer[offset + 1] = file->mpr->current_time_step;
			time_buffer[offset + 2] = rank;
			time_buffer[offset + 3] = total_time;
			time_buffer[offset + 4] = rst_time;
			time_buffer[offset + 5] = wave_time;
			time_buffer[offset + 6] = comp_time;
			time_buffer[offset + 7] = agg_time;
			time_buffer[offset + 8] = wrt_data_time;
		}

		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
		{
			int count = file->mpr->last_tstep * time_count;
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
				{
					if (total_time_buffer[i*time_count] == 1)
						fprintf(fp, "%d %d: [%f] >= [p %f w %f c %f a %f o %f]\n", (int)total_time_buffer[i*time_count+1], (int)total_time_buffer[i*time_count+2], total_time_buffer[i*time_count+3], total_time_buffer[i*time_count+4], total_time_buffer[i*time_count+5], total_time_buffer[i*time_count+6], total_time_buffer[i*time_count+7], total_time_buffer[i*time_count+8]);
				}

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
		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
		{
			int log_count = 5;
			MPR_local_patch local_patch = file->variable[0]->local_patch;

			int offset = file->mpr->current_time_step * log_count;

			if (file->mpr->is_aggregator == 1)
			{
				size_buffer[offset] = file->mpr->is_aggregator;
				size_buffer[offset + 1] = file->mpr->current_time_step;
				size_buffer[offset + 2] = rank;
				size_buffer[offset + 3] = local_patch->agg_patch_count;
				size_buffer[offset + 4] = local_patch->out_file_size;
			}

			int count = file->mpr->last_tstep * log_count;
			long long int* total_size_buffer = NULL;
			if (rank == 0)
				total_size_buffer = malloc(count*sizeof(long long int)*file->comm->simulation_nprocs);

			MPI_Gather(size_buffer, count, MPI_LONG_LONG_INT, total_size_buffer, count, MPI_LONG_LONG_INT, 0, file->comm->simulation_comm);

			if (rank == 0)
			{
				char* directory_path = malloc(512);
				memset(directory_path, 0, sizeof(*directory_path) * 512);
				strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

				char size_log[512];
				sprintf(size_log, "size_%s_log", directory_path);
				free(directory_path);

				FILE* fp = fopen(size_log, "w"); /* open file */
				if (!fp) /* Check file handle */
					fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);

				int loop_count = file->comm->simulation_nprocs * file->mpr->last_tstep;
				for (int i = 0; i < loop_count; i++)
				{
					if (total_size_buffer[i*log_count] == 1)
						fprintf(fp, "%lld %lld %lld %lld\n", total_size_buffer[i*log_count+1], total_size_buffer[i*log_count+2], total_size_buffer[i*log_count+3], total_size_buffer[i*log_count+4]);
				}

				fclose(fp);
			}
			free(total_size_buffer);
		}
	}

	return MPR_success;
}
