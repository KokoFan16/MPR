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

	MPR_local_patch local_patch = file->variable[svi]->local_patch;
	local_patch->agg_patch_id_array = malloc(file->mpr->total_patches_num * sizeof(int));
	local_patch->agg_patch_disps = malloc(file->mpr->total_patches_num  * sizeof(int));
	local_patch->agg_patch_size = malloc(file->mpr->total_patches_num  * sizeof(int));

	int required_patch_count = 0; /* the required patch count for each process */
	for (int i = 0; i < file_num; i++)
	{
		sprintf(file_name, "%s/time%09d/%d", file->mpr->filename, file->mpr->current_time_step, file->mpr->open_file_ids[i]);
		MPR_file_related_metadata_parse(file_name, file, svi, &required_patch_count);  /* parse file related meta-data */

//		FILE * fp = fopen(file_name, "r"); /* Open bounding box meta-data file */
//		if (fp == NULL)
//		{
//			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
//			return MPR_err_file;
//		}
//
//		int metadata_size = file->mpr->file_metadata_count * bytes;
//
//		int required_patch_count = file->variable[svi]->local_patch->agg_patch_count;
//		MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */
//		local_patch->patch = malloc(sizeof(MPR_patch*)*required_patch_count); /* Local patch array per variable */
//
//		for (int p = 0; p < required_patch_count; p++)
//		{
//			local_patch->patch[p] = (MPR_patch)malloc(sizeof(*local_patch->patch[p]));/* Initialize patch pointer */
//			local_patch->patch[p]->global_id = local_patch->agg_patch_id_array[p];
//			local_patch->patch[p]->patch_buffer_size = local_patch->agg_patch_size[p] * bytes;
//			local_patch->patch[p]->buffer = malloc(local_patch->patch[p]->patch_buffer_size);
//			int offset = local_patch->agg_patch_disps[p] + metadata_size;
//			fseek(fp,  offset, SEEK_SET);
//			fread(local_patch->patch[p]->buffer, sizeof(char), local_patch->patch[p]->patch_buffer_size, fp);
//		}
//		fclose(fp);
	}
	local_patch->agg_patch_count = required_patch_count;

	printf("%d\n", required_patch_count);

	return MPR_success;
}
