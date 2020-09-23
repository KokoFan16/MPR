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
//	printf("%d: %f\n", file->comm->simulation_rank, file->time->rst_end - file->time->rst_start);

	/* Write Mode: write data out */
	MPR_return_code ret = 0;
	if (MODE == MPR_RAW_IO)
		ret =  MPR_raw_write(file, svi, evi);

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

