/*
 * MPR_close.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

//static void MPR_timing_output(MPR_file file, int svi, int evi);

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
		/* write 10 time-steps */
		if (MPR_write(file, lvi, (lvi + lvc)) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_io;
		}
	}

	if (file->flags == MPR_MODE_RDONLY)
	{
		if (MPR_read(file, lvi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_io;
		}
	}
	file->time->total_end = MPI_Wtime(); /* the end time for the program */


	if (MPR_timing_logs(file, lvi, (lvi + lvc)) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_io;
	}

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


void MPR_timing_output(MPR_file file, int svi, int evi)
{
	int MODE = file->mpr->io_type;
	int rank = file->comm->simulation_rank;

	double total_time = file->time->total_end - file->time->total_start;
	double rst_time = file->time->rst_end - file->time->rst_start;
	double wave_time = file->time->wave_end - file->time->wave_start;
	double comp_time = file->time->zfp_end - file->time->zfp_start;

	double max_total_time = 0;
	MPI_Allreduce(&total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, file->comm->simulation_comm);

	if (file->flags == MPR_MODE_CREATE)
	{
		double agg_time = file->time->agg_end - file->time->agg_start;
		double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;
		double wrt_metadata_time = file->time->wrt_metadata_end - file->time->wrt_metadata_start;

		if (MODE == MPR_RAW_IO)
		{
			if (file->mpr->is_aggregator == 1)
				fprintf(stderr,"AGG_%d: [%f] >= [rst %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, agg_time, wrt_data_time, wrt_metadata_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [rst %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, agg_time, wrt_data_time, wrt_metadata_time);
		}
		else if (MODE == MPR_MUL_RES_IO)
		{
			if (file->mpr->is_aggregator == 1)
				fprintf(stderr,"AGG_%d: [%f] >= [rst %f wave %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, wave_time, agg_time, wrt_data_time, wrt_metadata_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [rst %f wave %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, wave_time, agg_time, wrt_data_time, wrt_metadata_time);
		}
		else if (MODE == MPR_MUL_PRE_IO)
		{
			if (rank == 0)
			{
				for (int v = svi; v < evi; v++)
					printf("The compression ratio for variable %d is %f.\n", v, file->variable[v]->local_patch->compression_ratio);
			}
			if (file->mpr->is_aggregator == 1)
				fprintf(stderr,"AGG_%d: [%f] >= [rst %f comp %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [rst %f comp %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);
		}
		else if (MODE == MPR_MUL_RES_PRE_IO)
		{
			if (rank == 0)
			{
				for (int v = svi; v < evi; v++)
					printf("The compression ratio for variable %d is %f.\n", v, file->variable[v]->local_patch->compression_ratio);
			}
			if (file->mpr->is_aggregator == 1)
				fprintf(stderr,"AGG_%d: [%f] >= [rst %f wave %f comp %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, wave_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);
//			else
//				fprintf(stderr,"NOR_%d: [%f] >= [rst %f wave %f comp %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, wave_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);

			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [rst %f wave %f comp %f agg %f w_dd %f w_meda %f]\n", rank, total_time, rst_time, wave_time, comp_time, agg_time, wrt_data_time, wrt_metadata_time);
		}
		else
			fprintf(stderr,"Unsupported Mode!");
	}

	if (file->flags == MPR_MODE_RDONLY)
	{
		double parse_bound = file->time->parse_bound_end - file->time->parse_bound_start;
		double read_time = file->time->read_end - file->time->read_start;

		if (MODE == MPR_RAW_IO)
		{
			fprintf(stderr,"Rank_%d: [%f] >= [meta %f read %f rst %f]\n", rank, total_time, parse_bound, read_time, rst_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [meta %f read %f rst %f]\n", rank, total_time, parse_bound, read_time, rst_time);
		}
		else if (MODE == MPR_MUL_RES_IO)
		{
			fprintf(stderr,"Rank_%d: [%f] >= [meta %f read %f wave %f rst %f]\n", rank, total_time, parse_bound, read_time, wave_time, rst_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [meta %f read %f wave %f rst %f]\n", rank, total_time, parse_bound, read_time, rst_time, rst_time);
		}
		else if (MODE == MPR_MUL_PRE_IO)
		{
			fprintf(stderr,"Rank_%d: [%f] >= [meta %f read %f zfp %f rst %f]\n", rank, total_time, parse_bound, read_time, comp_time, rst_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [meta %f read %f zfp %f rst %f]\n", rank, total_time, parse_bound, read_time, comp_time, rst_time);
		}
		else if (MODE == MPR_MUL_RES_PRE_IO)
		{
			fprintf(stderr,"Rank_%d: [%f] >= [meta %f read %f zfp %f wave %f rst %f]\n", rank, total_time, parse_bound, read_time, comp_time, wave_time, rst_time);
			if (total_time == max_total_time)
				fprintf(stderr, "MAX_%d: [%f] >= [meta %f read %f zfp %f wave %f rst %f]\n", rank, total_time, parse_bound, read_time, comp_time, wave_time, rst_time);
		}
		else
			fprintf(stderr,"Unsupported Mode!\n");
	}
}
