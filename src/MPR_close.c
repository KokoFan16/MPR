/*
 * MPR_close.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

static void MPR_timing_output(MPR_file file, int svi, int evi);

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
		if (MPR_write(file, lvi, (lvi + lvc)) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_io;
		}
	}
	file->time->total_end = MPI_Wtime(); /* the end time for the program */

	MPR_timing_output(file, lvi, (lvi + lvc));

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

static void MPR_timing_output(MPR_file file, int svi, int evi)
{
	int MODE = file->mpr->io_type;
	int rank = file->comm->simulation_rank;
	double total_time = file->time->total_end - file->time->total_start;
	double rst_time = file->time->rst_end - file->time->rst_start;
	double agg_time = file->time->agg_end - file->time->agg_start;
	double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;
	double wrt_metadata_time = file->time->wrt_metadata_end - file->time->wrt_metadata_start;

	double max_total_time = 0;
	MPI_Allreduce(&total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, file->comm->simulation_comm);

	if (rank == 0)
	{
		char mode[100];
		if (file->mpr->aggregation_mode == 0)
			sprintf(mode, "%s", "Fixed-size");
		else if (file->mpr->aggregation_mode == 1)
			sprintf(mode, "%s", "Fixed-patch");
		else
			sprintf(mode, "%s", "Default");
		printf("Current aggregation mode is %s mode\n", mode);
	}

	if (MODE == MPR_RAW_IO)
	{
		if (file->mpr->is_aggregator == 1)
			fprintf(stderr,"AGG_%d: [%f] >= [rst %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, agg_time, wrt_data_time, wrt_metadata_time);
		if (total_time == max_total_time)
			fprintf(stderr, "MAX_%d: [%f] >= [rst %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, agg_time, wrt_data_time, wrt_metadata_time);
	}
}
