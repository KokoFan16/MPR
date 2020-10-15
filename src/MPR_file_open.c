/*
 * MPR_file_open.c
 *
 *  Created on: Oct 8, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_file_open(const char* filename, MPR_access access_type, MPR_file* file)
{
	*file = malloc(sizeof (*(*file)) );
	memset(*file, 0, sizeof (*(*file)) );

	(*file)->mpr = malloc(sizeof (*((*file)->mpr)));
	memset((*file)->mpr, 0, sizeof (*((*file)->mpr)));

	(*file)->comm = malloc(sizeof (*((*file)->comm)));
	memset((*file)->comm, 0, sizeof (*((*file)->comm)));

	(*file)->time = malloc(sizeof (*((*file)->time)));
	memset((*file)->time, 0, sizeof (*((*file)->time)));

	(*file)->time->total_start = MPI_Wtime();  /* the start time for this program */

	sprintf((*file)->mpr->filename, "%s", filename);
	sprintf((*file)->mpr->filename_time_template, "time%%09d/");

	(*file)->mpr->io_type = MPR_RAW_IO;

	(*file)->mpr->current_time_step = 0;
	(*file)->mpr->first_tstep = 0;
	(*file)->mpr->last_tstep = 0;

	(*file)->mpr->variable_count = -1;

	(*file)->mpr->total_patches_num = 0;

	(*file)->mpr->proc_num_per_node = 0;
	(*file)->mpr->node_num = 0;
	(*file)->mpr->proc_num_last_node = 0;

	(*file)->mpr->compression_type = MPR_NO_COMPRESSION;
	(*file)->mpr->compression_param = 0;

	(*file)->mpr->out_file_num = 0;
	(*file)->mpr->file_size = 0;
	(*file)->mpr->is_aggregator = 0;

	(*file)->mpr->wavelet_trans_num = 0;

	(*file)->local_variable_index = 0;
	(*file)->local_variable_count = 0;
	(*file)->variable_index_tracker = 0;
	(*file)->fs_block_size = 0;

	(*file)->comm->simulation_comm = access_type->comm;
	MPI_Comm_rank((*file)->comm->simulation_comm, &((*file)->comm->simulation_rank));
	MPI_Comm_size((*file)->comm->simulation_comm, &((*file)->comm->simulation_nprocs));

	char mpr_meta_file[MPR_FILE_PATH_LENGTH];
	sprintf(mpr_meta_file, "%s.mpr", filename);
	mpr_meta_file[strlen(filename) + 4] = '\0';

	if (MPR_basic_metatda_parse(mpr_meta_file, file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	MPI_Bcast(&((*file)->mpr->io_type), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->global_box), MPR_MAX_DIMENSIONS, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->patch_box), MPR_MAX_DIMENSIONS, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->out_file_num), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->total_patches_num), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->variable_count), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->proc_num_per_node), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->proc_num_last_node), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->compression_type), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->compression_param), 1, MPI_FLOAT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->wavelet_trans_num), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->first_tstep), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->last_tstep), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
	MPI_Bcast(&((*file)->mpr->filename_time_template), MPR_FILE_PATH_LENGTH, MPI_CHAR, 0, (*file)->comm->simulation_comm);

	if ((*file)->comm->simulation_rank != 0)
	{
		for (int v = 0; v < (*file)->mpr->variable_count; v++)
		{
			(*file)->variable[v] = malloc(sizeof (*((*file)->variable[v])));
			memset((*file)->variable[v], 0, sizeof (*((*file)->variable[v])));
		}
	}

	for (int v = 0; v < (*file)->mpr->variable_count; v++)
	{
		MPI_Bcast(&((*file)->variable[v]->bpv), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
		MPI_Bcast(&((*file)->variable[v]->vps), 1, MPI_INT, 0, (*file)->comm->simulation_comm);
		MPI_Bcast(&((*file)->variable[v]->var_name), MPR_FILE_PATH_LENGTH, MPI_CHAR, 0, (*file)->comm->simulation_comm);
		MPI_Bcast(&((*file)->variable[v]->type_name), 512, MPI_CHAR, 0, (*file)->comm->simulation_comm);
	}

	return MPR_success;
}
