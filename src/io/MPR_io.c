/*
 * MPR_io.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "MPR.h"

MPR_return_code MPR_write(MPR_file file, int svi, int evi)
{
//	write_data_out(file, svi);

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
	else if (MODE == MPR_Benchmark)
		ret = MPR_benchmark_write(file, svi, evi);
	else
		fprintf(stderr, "Unsupported MPR Mode.\n");

	if (ret != MPR_success)
	{
		fprintf(stderr,"File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* Write metadata out */
	file->time->wrt_metadata_start = MPI_Wtime();
	if (MPR_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_metadata_end = MPI_Wtime();

	/* write data out */
	file->time->wrt_data_start = MPI_Wtime();
	if (MPR_write_data_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wrt_data_end = MPI_Wtime();

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

	if (MPR_check_local_box(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

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

	/* For check the read result */
//	if (write_data_out(file, svi) != MPR_success)
//	{
//		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
//		return MPR_err_file;
//	}

	/* buffers cleanup */
	if (MPR_variable_cleanup(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}


MPR_return_code write_data_out(MPR_file file, int svi)
{
//	if (file->comm->simulation_rank == 0)
//	{
//		for (int i = 0; i < 4096; i++)
//		{
//			float a;
//			memcpy(&a, &file->variable[svi]->local_patch->buffer[i*4], 4);
//			printf("%f\n", a);
//		}
//	}

	char file_name[512] = "./out_data.raw";
	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */

	int div = pow(2, file->mpr->read_level);

	int tmp_global_box[MPR_MAX_DIMENSIONS] = {file->mpr->global_box[0]/div * bytes, file->mpr->global_box[1]/div, file->mpr->global_box[2]/div};
	int tmp_local_box[MPR_MAX_DIMENSIONS] = {file->mpr->local_box[0]/div * bytes, file->mpr->local_box[1]/div, file->mpr->local_box[2]/div};
	int tmp_local_offset[MPR_MAX_DIMENSIONS] = {file->mpr->local_offset[0]/div * bytes, file->mpr->local_offset[1]/div, file->mpr->local_offset[2]/div};

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
		if (file->mpr->local_offset[i] + file->mpr->local_box[i] > file->mpr->origin_global_box[i])
			file->mpr->local_box[i] = (file->mpr->origin_global_box[i] - file->mpr->local_offset[i]);
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
