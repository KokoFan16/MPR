/*
 * MPR_read_io.c
 *
 *  Created on: Oct 19, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

/* Read data with raw I/O mode */
MPR_return_code MPR_raw_read(MPR_file file, int svi)
{
	/* read data */
	file->time->read_start = MPI_Wtime();
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->read_end = MPI_Wtime();

	return MPR_success;
}


/* Read data with multiple resolution I/O mode */
MPR_return_code MPR_multi_res_read(MPR_file file, int svi)
{
	/* read data */
	file->time->read_start = MPI_Wtime();
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->read_end = MPI_Wtime();

	/* decode wavelet transform */
	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_decode_perform(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wave_end = MPI_Wtime();

	return MPR_success;
}


/* Read data with multiple precision mode */
MPR_return_code MPR_multi_pre_read(MPR_file file, int svi)
{
	/* read data */
	file->time->read_start = MPI_Wtime();
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->read_end = MPI_Wtime();

	/* decompression */
	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_decompression_perform(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->zfp_end = MPI_Wtime();

	return MPR_success;
}


/* Read data with multiple resolution and precision mode */
MPR_return_code MPR_multi_pre_res_read(MPR_file file, int svi)
{
	/* read data */
	file->time->read_start = MPI_Wtime();
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->read_end = MPI_Wtime();

	/* decompression */
	file->time->zfp_start = MPI_Wtime();
	if (MPR_ZFP_multi_res_decompression_perform(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->zfp_end = MPI_Wtime();

	/* decode wavelet transform */
	file->time->wave_start = MPI_Wtime();
	if (MPR_wavelet_decode_perform(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	file->time->wave_end = MPI_Wtime();

	return MPR_success;
}

/* Read data with meta-data */
MPR_return_code MPR_read_data(MPR_file file, int svi)
{
	/* The file name for out files */
	char file_name[PATH_MAX];
	memset(file_name, 0, PATH_MAX * sizeof(*file_name));

	int file_num = file->mpr->open_file_num; /* the number of files need to be opened for each process */
	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */

	int ret = 0;

	ret = MPR_check_required_patches(file, svi); /* convert read box to patch ids */
	if (ret != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */

	int subband_num = file->mpr->wavelet_trans_num * 7 + 1;

	int* patches_offset = malloc(file->mpr->total_patches_num * sizeof(int)); /* patch offset array */
	int* patches_size = malloc(file->mpr->total_patches_num * sizeof(int)); /* patch size array */
	int* patches_subbands = malloc(file->mpr->total_patches_num * subband_num * sizeof(int)); /* patch sub-bands size array */

	for (int i = 0; i < file_num; i++)
	{
		sprintf(file_name, "%s/time%09d/%d", file->mpr->filename, file->mpr->current_time_step, file->mpr->open_file_ids[i]);
		ret = MPR_file_related_metadata_parse(file_name, file, svi, patches_offset, patches_size, patches_subbands);  /* parse file related meta-data */
		if (ret != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}

		FILE * fp = fopen(file_name, "r"); /* Open bounding box meta-data file */
		if (fp == NULL)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}

		for (int p = 0; p < local_patch->patch_count; p++)
		{
			int global_id = local_patch->agg_patch_id_array[p];
			if (patches_offset[global_id] != -1)
			{
				if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				{
					local_patch->patch[p]->subbands_comp_size = malloc(subband_num * sizeof(int));
					memcpy(local_patch->patch[p]->subbands_comp_size, &patches_subbands[global_id * subband_num], subband_num * sizeof(int));
				}
				local_patch->patch[p]->patch_buffer_size = patches_size[global_id];
				local_patch->patch[p]->buffer = malloc(local_patch->patch[p]->patch_buffer_size);

				fseek(fp,  patches_offset[global_id], SEEK_SET);  /* set offset */
				int read_size = fread(local_patch->patch[p]->buffer, sizeof(char), local_patch->patch[p]->patch_buffer_size, fp);
				if (read_size != local_patch->patch[p]->patch_buffer_size)
				{
					printf("%d %d %d %d %s\n", file->comm->simulation_rank, read_size, local_patch->patch[p]->patch_buffer_size,
							patches_offset[global_id], file_name);
					fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
					return MPR_err_file;
				}
			}
		}
		fclose(fp); /* close file */
	}
	free(file->mpr->open_file_ids);

	free(patches_offset);
	free(patches_size);
	free(patches_subbands);
	return MPR_success;
}

MPR_return_code MPR_check_required_patches(MPR_file file, int svi)
{
	MPR_local_patch local_patch = file->variable[svi]->local_patch;
	local_patch->agg_patch_id_array = malloc(file->mpr->total_patches_num * sizeof(int));
	local_patch->patch = malloc(sizeof(MPR_patch*) * file->mpr->total_patches_num); /* Local patch array per variable */

	int local_offset_xyz[MPR_MAX_DIMENSIONS]; /* The real offset to read in origin dataset */
	int local_end_xyz[MPR_MAX_DIMENSIONS];    /* The local end coordinate */
	int patch_count_xyz[MPR_MAX_DIMENSIONS]; /* The patch count in each dimension */
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++) /* The current offset plus the global offset to read */
	{
		local_offset_xyz[i] = (file->mpr->local_offset[i] + file->mpr->global_offset[i]);
		local_end_xyz[i] = ceil((float)(local_offset_xyz[i] + file->mpr->local_box[i]) / file->mpr->patch_box[i]);
		local_offset_xyz[i] /= file->mpr->patch_box[i];
		patch_count_xyz[i] = ceil((float)file->mpr->origin_global_box[i] / file->mpr->patch_box[i]);
	}

	int required_patch_count = 0;
	/* Obtain the required patches' id for each process */
	for (int k = local_offset_xyz[2]; k < local_end_xyz[2]; k++)
	{
		for (int j = local_offset_xyz[1]; j < local_end_xyz[1]; j++)
		{
			for (int i = local_offset_xyz[0]; i < local_end_xyz[0]; i++)
			{
				int patch_id = k * patch_count_xyz[0] * patch_count_xyz[1] + j * patch_count_xyz[0] + i;
				local_patch->patch[required_patch_count] = (MPR_patch)malloc(sizeof(*local_patch->patch[required_patch_count]));/* Initialize patch pointer */
				local_patch->patch[required_patch_count]->global_id = patch_id;
				local_patch->patch[required_patch_count]->offset[0] = i * file->mpr->patch_box[0];
				local_patch->patch[required_patch_count]->offset[1] = j * file->mpr->patch_box[1];
				local_patch->patch[required_patch_count]->offset[2] = k * file->mpr->patch_box[2];
				memcpy(local_patch->patch[required_patch_count]->size, file->mpr->patch_box, MPR_MAX_DIMENSIONS*sizeof(int));
				local_patch->agg_patch_id_array[required_patch_count] = patch_id;
				required_patch_count++;
			}
		}
	}
	local_patch->patch_count = required_patch_count;
	local_patch->patch = realloc(local_patch->patch, sizeof(MPR_patch*) * required_patch_count);
	local_patch->agg_patch_id_array = realloc(local_patch->agg_patch_id_array, sizeof(int) * required_patch_count);

	return MPR_success;
}


MPR_return_code MPR_get_local_read_box(MPR_file file, int svi)
{
	int local_offset[MPR_MAX_DIMENSIONS] = {INT_MAX, INT_MAX, INT_MAX};  /* current local offset */
	int local_end[MPR_MAX_DIMENSIONS] = {0, 0, 0}; /* current local end coordinate */

	MPR_local_patch local_patch = file->variable[svi]->local_patch;
	for (int i = 0; i < local_patch->patch_count; i++)
	{
		if (local_patch->patch[i]->offset[0] < local_offset[0]) local_offset[0] = local_patch->patch[i]->offset[0];
		if (local_patch->patch[i]->offset[1] < local_offset[1]) local_offset[1] = local_patch->patch[i]->offset[1];
		if (local_patch->patch[i]->offset[2] < local_offset[2]) local_offset[2] = local_patch->patch[i]->offset[2];

		if (local_patch->patch[i]->offset[0] > local_end[0]) local_end[0] = local_patch->patch[i]->offset[0];
		if (local_patch->patch[i]->offset[1] > local_end[1]) local_end[1] = local_patch->patch[i]->offset[1];
		if (local_patch->patch[i]->offset[2] > local_end[2]) local_end[2] = local_patch->patch[i]->offset[2];
	}

	int local_box[MPR_MAX_DIMENSIONS]; /* current local box */
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
	{
		local_end[i] += file->mpr->patch_box[i];
		local_box[i] = local_end[i] - local_offset[i];
	}

	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */
	local_box[0] = local_box[0] * bytes;

	int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2] * bytes;
	int array_subsize[MPR_MAX_DIMENSIONS] = {file->mpr->patch_box[0] * bytes, file->mpr->patch_box[1], file->mpr->patch_box[2]};

	unsigned char* local_buffer =  malloc(patch_size * local_patch->patch_count);

	MPI_Request req[local_patch->patch_count*2];
	MPI_Status stat[local_patch->patch_count*2];
	int req_id = 0;
	int reltive_patch_offset[MPR_MAX_DIMENSIONS];
	for (int i = 0; i < local_patch->patch_count; i++)
	{
		for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
			reltive_patch_offset[d] = (local_patch->patch[i]->offset[d] - local_offset[d]);
		reltive_patch_offset[0] = reltive_patch_offset[0] * bytes;

		/* Creating patch receive data type */
		MPI_Datatype recv_type;
		MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, local_box, array_subsize, reltive_patch_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &recv_type);
		MPI_Type_commit(&recv_type);

		MPI_Irecv(local_buffer, 1, recv_type, file->comm->simulation_rank, i, file->comm->simulation_comm, &req[req_id]);
		req_id++;

		MPI_Isend(local_patch->patch[i]->buffer, patch_size, MPI_BYTE, file->comm->simulation_rank, i,
				file->comm->simulation_comm, &req[req_id]);
		req_id++;

		MPI_Type_free(&recv_type);
	}
	MPI_Waitall(req_id, req, stat);

	int local_size = file->mpr->local_box[0] * file->mpr->local_box[1] * file->mpr->local_box[2] * bytes;
	local_patch->buffer = malloc(local_size);

	int relative_local_offset[MPR_MAX_DIMENSIONS];
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
		relative_local_offset[i] = (file->mpr->local_offset[i] + file->mpr->global_offset[i]) - local_offset[i];
	relative_local_offset[0] = relative_local_offset[0] * bytes;

	/* Cut of data based on the required bounding box */
	MPI_Datatype local_div_type;
	int div_array_subsize[MPR_MAX_DIMENSIONS] = {file->mpr->local_box[0] * bytes, file->mpr->local_box[1], file->mpr->local_box[2]};

	MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, local_box, div_array_subsize, relative_local_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &local_div_type);
	MPI_Type_commit(&local_div_type);

	MPI_Request req2[2];
	MPI_Status stat2[2];
	MPI_Irecv(local_patch->buffer, local_size, MPI_BYTE, file->comm->simulation_rank, file->comm->simulation_rank, file->comm->simulation_comm, &req2[0]);
	MPI_Isend(local_buffer, 1, local_div_type, file->comm->simulation_rank, file->comm->simulation_rank, file->comm->simulation_comm, &req2[1]);
	MPI_Waitall(2, req2, stat2);
	MPI_Type_free(&local_div_type);

	free(local_buffer);

	if ((file->mpr->io_type == MPR_MUL_RES_PRE_IO || file->mpr->io_type == MPR_MUL_RES_IO) && file->mpr->read_level > 0)
	{
		if (MPR_read_level_samples(file, svi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}
	return MPR_success;
}

MPR_return_code MPR_read_level_samples(MPR_file file, int svi)
{
	MPR_local_patch local_patch = file->variable[svi]->local_patch;
	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */

	int sample_step = pow(2, file->mpr->read_level);
	int local_size = ((file->mpr->local_box[0])/sample_step) * ((file->mpr->local_box[1])/sample_step) * ((file->mpr->local_box[2])/sample_step) * bytes;
	unsigned char* local_level_buffer = malloc(local_size);

	int x = 0, y = 0, z = 0;
	if (file->mpr->global_offset[0] % sample_step != 0)
		x = sample_step - (file->mpr->global_offset[0] % sample_step);
	if (file->mpr->global_offset[1] % sample_step != 0)
		y = sample_step - (file->mpr->global_offset[1] % sample_step);
	if (file->mpr->global_offset[2] % sample_step != 0)
		z = sample_step - (file->mpr->global_offset[2] % sample_step);

	int temp = 0;
	for (int k = z; k < file->mpr->local_box[2]; k += sample_step)
	{
		for (int j = y; j < file->mpr->local_box[1]; j += sample_step)
		{
			for (int i = x; i < file->mpr->local_box[0]; i += sample_step)
			{
				int index = k * (file->mpr->local_box[1] * file->mpr->local_box[0]) + j * file->mpr->local_box[0] + i;
				memcpy(&local_level_buffer[temp * bytes], &local_patch->buffer[index * bytes], bytes);
				temp++;
			}
		}
	}

	local_patch->buffer = realloc(local_patch->buffer, local_size);
	memcpy(local_patch->buffer, local_level_buffer, local_size);

	free(local_level_buffer);
	return MPR_success;
}
