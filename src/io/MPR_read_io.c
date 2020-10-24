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
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	return MPR_success;
}


/* Read data with multiple resolution I/O mode */
MPR_return_code MPR_multi_res_read(MPR_file file, int svi)
{
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	return MPR_success;
}


/* Read data with multiple precision mode */
MPR_return_code MPR_multi_pre_read(MPR_file file, int svi)
{
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	return MPR_success;
}


/* Read data with multiple resolution and precision mode */
MPR_return_code MPR_multi_pre_res_read(MPR_file file, int svi)
{
	if (MPR_read_data(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
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

	MPR_check_required_patches(file, svi); /* convert read box to patch ids */

	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */
	int required_patch_count = local_patch->agg_patch_count;
//	local_patch->patch = malloc(sizeof(MPR_patch*) * required_patch_count); /* Local patch array per variable */

	int* patches_offset = malloc(file->mpr->total_patches_num * sizeof(int)); /* patch offset array */
	int* patches_size = malloc(file->mpr->total_patches_num * sizeof(int)); /* patch size array */
	int* patches_subbands = malloc(file->mpr->total_patches_num * (file->mpr->wavelet_trans_num * 7 + 1) * sizeof(int)); /* patch sub-bands size array */

	for (int i = 0; i < file_num; i++)
	{
		sprintf(file_name, "%s/time%09d/%d", file->mpr->filename, file->mpr->current_time_step, file->mpr->open_file_ids[i]);
		MPR_file_related_metadata_parse(file_name, file, svi, patches_offset, patches_size, patches_subbands);  /* parse file related meta-data */

		FILE * fp = fopen(file_name, "r"); /* Open bounding box meta-data file */
		if (fp == NULL)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}

		for (int p = 0; p < required_patch_count; p++)
		{
			int global_id = local_patch->agg_patch_id_array[p];
			if (patches_offset[global_id] != -1)
			{
//				printf("%d: %dx%dx%d, %dx%dx%d\n", global_id, local_patch->patch[p]->offset[0], local_patch->patch[p]->offset[1], local_patch->patch[p]->offset[2],
//						local_patch->patch[p]->size[0], local_patch->patch[p]->size[1], local_patch->patch[p]->size[2]);
				local_patch->patch[p]->patch_buffer_size = patches_size[global_id];
				local_patch->patch[p]->buffer = malloc(local_patch->patch[p]->patch_buffer_size);

				fseek(fp,  patches_offset[global_id], SEEK_SET);  /* set offset */
				int read_size = fread(local_patch->patch[p]->buffer, sizeof(char), local_patch->patch[p]->patch_buffer_size, fp);
				if (read_size != local_patch->patch[p]->patch_buffer_size)
				{
					fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
					return MPR_err_file;
				}
			}
		}
		fclose(fp); /* close file */
	}

	for (int i = 0; i < 512; i++)
	{
		float a;
		memcpy(&a, &local_patch->patch[0]->buffer[i*sizeof(float)], sizeof(float));
		printf("%f\n", a);
	}

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
		local_end_xyz[i] = ceil((local_offset_xyz[i] + file->mpr->local_box[i]) / (float)file->mpr->patch_box[i]);
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
				local_patch->patch[patch_id] = (MPR_patch)malloc(sizeof(*local_patch->patch[patch_id]));/* Initialize patch pointer */
				local_patch->patch[patch_id]->global_id = patch_id;
				local_patch->patch[patch_id]->offset[0] = i * file->mpr->patch_box[0];
				local_patch->patch[patch_id]->offset[1] = j * file->mpr->patch_box[1];
				local_patch->patch[patch_id]->offset[2] = k * file->mpr->patch_box[2];
				memcpy(local_patch->patch[patch_id]->size, file->mpr->patch_box, MPR_MAX_DIMENSIONS*sizeof(int));
				local_patch->agg_patch_id_array[required_patch_count++] = patch_id;
			}
		}
	}
	local_patch->agg_patch_count = required_patch_count;
	local_patch->patch = realloc(local_patch->patch, sizeof(MPR_patch*) * required_patch_count);

	return MPR_success;
}
