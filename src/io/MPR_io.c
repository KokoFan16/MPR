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

//	file->time->rst_start = MPI_Wtime();
	if (MPR_is_partition(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
//	file->time->rst_end = MPI_Wtime();

	/* Write Mode: write data out */
	int ret = 0;
	if (MODE != MPR_RAW_IO)
	{
		if (MODE == MPR_MUL_RES_IO)
			ret = MPR_multi_res_write(file, svi, evi);
		else if (MODE == MPR_MUL_PRE_IO)
			ret = MPR_multi_pre_write(file, svi, evi);
		else if (MODE == MPR_MUL_RES_PRE_IO)
			ret = MPR_multi_pre_res_write(file, svi, evi);
		else
			fprintf(stderr, "Unsupported MPR Mode.\n");
	}

	if (ret != MPR_success)
	{
		fprintf(stderr,"File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}


	/* Aggregation phase */
	if (file->mpr->out_file_num == file->comm->simulation_nprocs)
	{
		if (MPR_no_aggregation(file, svi, evi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}
	else
	{
		Events e("AGG", 1);
//		file->time->agg_start = MPI_Wtime();
		if (MPR_aggregation_perform(file, svi, evi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
//		file->time->agg_end = MPI_Wtime();
	}

	/* Write metadata out */
//	file->time->wrt_data_start = MPI_Wtime();
	{
		Events e("wrt", 1);
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* write data out */
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	}
//	file->time->wrt_data_end = MPI_Wtime();

	return MPR_success;
}


MPR_return_code MPR_read(MPR_file file, int svi)
{
	int MODE = file->mpr->io_type;

	if (MPR_check_local_box(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* check which files need to be opened */
	file->time->read_start = MPI_Wtime();
	if (MPR_check_bouding_box(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* read data */
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->read_end = MPI_Wtime();


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

	/* For check the read result */
	if (file->mpr->is_write == 1)
	{
		if (write_data_out(file, svi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}

	return MPR_success;
}


MPR_return_code write_data_out(MPR_file file, int svi)
{
	char file_name[512] = "./out_data.raw";
	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */

	int div = 1;
	int tmp_global_box[MPR_MAX_DIMENSIONS];
	int tmp_local_box[MPR_MAX_DIMENSIONS];
	int tmp_local_offset[MPR_MAX_DIMENSIONS];

	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		tmp_global_box[d] = file->mpr->global_box[d] / div;
		tmp_local_box[d] = file->mpr->local_box[d] / div;
		tmp_local_offset[d] = file->mpr->local_offset[d] / div;

		if (file->mpr->read_level > 0)
		{
			div = pow(2, file->mpr->read_level);
			tmp_global_box[d] = (file->mpr->global_box[d] + 1) / div;
			tmp_local_box[d] = (file->mpr->local_box[d] + 1) / div;
			tmp_local_offset[d] = (file->mpr->local_offset[d] + 1) / div;
		}

		if ((tmp_local_offset[d] + tmp_local_box[d]) >  tmp_global_box[d])
			tmp_local_box[d] = tmp_global_box[d] - tmp_local_offset[d];
	}
	if (file->comm->simulation_rank == 0)
		printf("Dimensions of output data at level %d is %dx%dx%d.\n",
				file->mpr->read_level, tmp_global_box[0], tmp_global_box[1], tmp_global_box[2]);

	tmp_global_box[0] *= bytes;
	tmp_local_box[0] *= bytes;
	tmp_local_offset[0] *= bytes;

	int size = tmp_local_box[0] * tmp_local_box[1] * tmp_local_box[2];


	MPI_Datatype out_type;
	MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, tmp_global_box, tmp_local_box, tmp_local_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &out_type);
	MPI_Type_commit(&out_type);

	MPI_File fh;
	MPI_Status status;
	MPI_File_open(file->comm->simulation_comm, file_name, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	MPI_File_set_view(fh, 0, MPI_CHAR, out_type, "native", MPI_INFO_NULL);
	MPI_File_write(fh, file->variable[svi]->local_patch->buffer, size, MPI_BYTE, &status);
	MPI_File_close(&fh);

	MPI_Type_free(&out_type);

	return MPR_success;
}


MPR_return_code MPR_check_local_box(MPR_file file)
{
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
	{
		if (file->mpr->local_offset[i] + file->mpr->local_box[i] > file->mpr->global_box[i])
			file->mpr->local_box[i] = (file->mpr->global_box[i] - file->mpr->local_offset[i]);
	}
	return MPR_success;
}


MPR_return_code MPR_check_bouding_box(MPR_file file)
{
	char bounding_meta_path[PATH_MAX]; /* the patch bounding box metadata */
	memset(bounding_meta_path, 0, sizeof(*bounding_meta_path) * PATH_MAX);
	sprintf(bounding_meta_path, "%s/time%09d/bounding_box.mpr", file->mpr->filename, file->mpr->current_time_step);

	/* Parse the bounding box meta-data */
	if (MPR_bounding_box_metatda_parse(bounding_meta_path, file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}
