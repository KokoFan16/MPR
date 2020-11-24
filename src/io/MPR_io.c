/*
 * MPR_io.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

MPR_return_code MPR_write(MPR_file file, int svi, int evi)
{
	int MODE = file->mpr->io_type;
	/* Set the default restructuring box (32x32x32) */
	if (MPR_set_patch_box_size(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	/* Create metadata layout (folder) */
	if (MPR_create_folder_structure(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	/* Perform restructure phase */
	file->time->rst_start = MPI_Wtime();
	if (MPR_restructure_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->rst_end = MPI_Wtime();

	/* Write Mode: write data out */
	int ret = 0;
	if (MODE == MPR_RAW_IO)
		ret = MPR_raw_write(file, svi, evi);
	else if (MODE == MPR_MUL_RES_IO)
		ret = MPR_multi_res_write(file, svi, evi);
	else if (MODE == MPR_MUL_PRE_IO)
		ret = MPR_multi_pre_write(file, svi, evi);
	else if (MODE == MPR_MUL_RES_PRE_IO)
		ret = MPR_multi_pre_res_write(file, svi, evi);
	else
		fprintf(stderr, "Unsupported MPR Mode.\n");

	if (ret != MPR_success)
	{
		fprintf(stderr,"File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* buffers cleanup */
	if (MPR_variable_buffer_cleanup(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}


MPR_return_code MPR_read(MPR_file file, int svi)
{
	int MODE = file->mpr->io_type;

	/* check which files need to be opened */
	file->time->parse_bound_start = MPI_Wtime();
	if (MPR_check_bouding_box(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->parse_bound_end = MPI_Wtime();

	int ret = 0;
	if (MODE == MPR_RAW_IO)
		ret = MPR_raw_read(file, svi);
	else if (MODE == MPR_MUL_RES_IO)
		ret = MPR_multi_res_read(file, svi);
	else if (MODE == MPR_MUL_PRE_IO)
		ret = MPR_multi_pre_read(file, svi);
	else if (MODE == MPR_MUL_RES_PRE_IO)
		ret = MPR_multi_pre_res_read(file, svi);
	else
		fprintf(stderr, "Unsupported MPR Mode.\n");

	/* get local box for each process */
	file->time->rst_start =  MPI_Wtime();
	if (MPR_get_local_read_box(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->rst_end =  MPI_Wtime();
//	if (file->comm->simulation_rank == 0)
//		printf("MPR_get_local_read_box\n");

	/* buffers cleanup */
	if (MPR_variable_cleanup(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}


MPR_return_code MPR_check_bouding_box(MPR_file file)
{
	char bounding_meta_path[PATH_MAX]; /* the patch bounding box metadata */
	memset(bounding_meta_path, 0, sizeof(*bounding_meta_path) * PATH_MAX);
	sprintf(bounding_meta_path, "%s_bounding_box", file->mpr->filename);

	/* Parse the bounding box meta-data */
	if (MPR_bounding_box_metatda_parse(bounding_meta_path, file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}
