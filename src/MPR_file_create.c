/*
 * MPR_file_create.c
 *
 *  Created on: Jul 6, 2020
 *      Author: kokofan
 */

#ifndef SRC_MPR_FILE_CREATE_C_
#define SRC_MPR_FILE_CREATE_C_

#include "../MPR_inc.h"

MPR_return_code MPR_file_create(const char* filename, int flags, MPR_access access_type, MPR_point global, MPR_point local, MPR_point offset, MPR_file* file)
{

	if (flags != MPR_MODE_CREATE && flags != MPR_MODE_EXCL)
	{
		fprintf(stderr,"[%s] [%d]\n", __FILE__, __LINE__);
		return MPR_err_unsupported_flags;
	}

	if (flags == MPR_MODE_EXCL)   /* Error creating a file that already exists */
	{
		struct stat buffer;
		if (stat(filename, &buffer) != 0)
		{
			fprintf(stderr,"[%s] [%d]\n", __FILE__, __LINE__);
			return MPR_err_file_exists;
		}
	}

	char file_name_skeleton[MPR_FILE_PATH_LENGTH];

	if (strncmp(".mpr", &filename[strlen(filename) - 4], 4) != 0 && !filename){
		fprintf(stderr,"[%s] [%d]\n", __FILE__, __LINE__);
		return MPR_err_name;
	}

	*file = malloc(sizeof (*(*file)) );
	memset(*file, 0, sizeof (*(*file)) );

	(*file)->flags = flags;

	(*file)->mpr = malloc(sizeof (*((*file)->mpr)));
	memset((*file)->mpr, 0, sizeof (*((*file)->mpr)));

	(*file)->comm = malloc(sizeof (*((*file)->comm)));
	memset((*file)->comm, 0, sizeof (*((*file)->comm)));

	(*file)->time = malloc(sizeof (*((*file)->time)));
	memset((*file)->time, 0, sizeof (*((*file)->time)));

	(*file)->restructured_patch = malloc(sizeof(*(*file)->restructured_patch ));
	memset((*file)->restructured_patch , 0, sizeof(*(*file)->restructured_patch));

	if (global != NULL)
		memcpy((*file)->mpr->global_box, global, MPR_MAX_DIMENSIONS * sizeof(int));

	if (local != NULL)
		memcpy((*file)->mpr->local_box, local, MPR_MAX_DIMENSIONS * sizeof(int));

	if (local != NULL)
		memcpy((*file)->mpr->local_offset, offset, MPR_MAX_DIMENSIONS * sizeof(int));

	(*file)->comm->simulation_comm = access_type->comm;

	MPI_Comm_rank((*file)->comm->simulation_comm, &((*file)->comm->simulation_rank));
	MPI_Comm_size((*file)->comm->simulation_comm, &((*file)->comm->simulation_nprocs));

	(*file)->mpr->io_type = MPR_RAW_IO;

	(*file)->mpr->current_time_step = 0;
	(*file)->mpr->first_tstep = 0;
	(*file)->mpr->last_tstep = 0;

	(*file)->mpr->variable_count = -1;

	strncpy(file_name_skeleton, filename, strlen(filename) - 4);
	file_name_skeleton[strlen(filename) - 4] = '\0';
	sprintf((*file)->mpr->filename, "%s.mpr", file_name_skeleton);
	sprintf((*file)->mpr->filename_time_template, "time%%09d/");

	(*file)->mpr->compression_type = MPR_NO_COMPRESSION;
	(*file)->mpr->compression_bit_rate = 64;
	(*file)->mpr->compression_param = 1;

	(*file)->mpr->out_file_num = 0;
	(*file)->mpr->file_size = 0;
//	(*file)->mpr->patch_count_per_out_file = 0;
	(*file)->mpr->is_aggregator = 0;

	(*file)->mpr->aggregation_mode = -1;

	(*file)->mpr->restructuring_factor[0] = 1;
	(*file)->mpr->restructuring_factor[1] = 1;
	(*file)->mpr->restructuring_factor[2] = 1;

	(*file)->restructured_patch->patch_size[0] = -1;
	(*file)->restructured_patch->patch_size[1] = -1;
	(*file)->restructured_patch->patch_size[2] = -1;

	(*file)->local_variable_index = 0;
	(*file)->local_variable_count = 0;
	(*file)->variable_index_tracker = 0;
	(*file)->max_wavelet_level = 0;
	(*file)->fs_block_size = 0;

	if ((*file)->comm->simulation_rank == 0)

	{
		struct stat stat_buf;
		FILE *dummy = fopen(".dummy.txt", "w");
		fclose(dummy);
		int ret = stat(".dummy.txt", &stat_buf);
		if (ret != 0)
		{
			fprintf(stderr, "[%s] [%d] Unable to identify File-System block size\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
		(*file)->fs_block_size = stat_buf.st_blksize;
	}

	MPI_Bcast(&((*file)->fs_block_size), 1, MPI_INT, 0, (*file)->comm->simulation_comm);

	return MPR_success;
}

#endif /* SRC_MPR_FILE_CREATE_C_ */
