/*
 * metadata_io.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */


#include "../MPR_inc.h"
#include <errno.h>

MPR_return_code MPR_create_folder_structure(MPR_file file, int svi, int evi)
{
	char* file_name = file->mpr->filename;

	char *directory_path;
	char *data_set_path;

	directory_path = malloc(sizeof(*directory_path) * PATH_MAX);
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file_name, strlen(file_name) - 4);

	data_set_path = malloc(sizeof(*data_set_path) * PATH_MAX);
	memset(data_set_path, 0, sizeof(*data_set_path) * PATH_MAX);

	char time_template[512];
	sprintf(time_template, "%%s/%s", file->mpr->filename_time_template);
	sprintf(data_set_path, time_template, directory_path, file->mpr->current_time_step);
	free(directory_path);

	char last_path[PATH_MAX] = {0};
	char this_path[PATH_MAX] = {0};
	char tmp_path[PATH_MAX] = {0};
	char* pos;
	if (file->comm->simulation_rank == 0)
	{
		strcpy(this_path, data_set_path);
		if ((pos = strrchr(this_path, '/')))
		{
			pos[1] = '\0';
			if (strcmp(this_path, last_path) != 0)
			{
				/* make sure if this directory exists */
				strcpy(last_path, this_path);
				memset(tmp_path, 0, PATH_MAX * sizeof (char));
				/* walk up path and mkdir each segment */
				for (int j = 0; j < (int)strlen(this_path); j++)
				{
					if (j > 0 && this_path[j] == '/')
					{
						int ret = mkdir(tmp_path, S_IRWXU | S_IRWXG | S_IRWXO);
						if (ret != 0 && errno != EEXIST)
						{
							fprintf(stderr, "Error: failed to mkdir %s\n", tmp_path);
							return MPR_err_file;
						}
					}
					tmp_path[j] = this_path[j];
				}
			}
		}
	}
	MPI_Barrier(file->comm->simulation_comm);
	free(data_set_path);
	return MPR_success;
}

MPR_return_code MPR_basic_info_metadata_write_out(MPR_file file)
{
	char* file_name = file->mpr->filename; /* file name for */
	/* Check if the data format is .mpr */
	if (strncmp(".mpr", &file_name[strlen(file_name) - 4], 4) != 0)
	{
		fprintf(stderr, "[%s] [%d] Bad file name extension.\n", __FILE__, __LINE__);
		return 1;
	}

	if (file->comm->simulation_rank == 0)
	{
		FILE* fp = fopen(file_name, "w"); /* open file */
	    if (!fp) /* Check file handle */
	    {
			fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);
			return -1;
	    }
	    /* Write IO Mode */
	    if (file->mpr->io_type == MPR_RAW_IO)
	    	fprintf(fp, "(IO mode)\nRAW\n");
	    else if (file->mpr->io_type == MPR_MUL_RES_IO)
	    	fprintf(fp, "(IO mode)\nMUL_RES\n");
	    else if (file->mpr->io_type == MPR_MUL_PRE_IO)
			fprintf(fp, "(IO mode)\nMUL_PRE\n");
	    else if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
	    	fprintf(fp, "(IO mode)\nMUL_RES_PRE\n");
	    /* Write global box and patch box */
	    fprintf(fp, "(Global box)\n%d %d %d\n", file->mpr->global_box[0], file->mpr->global_box[1], file->mpr->global_box[2]);
	    fprintf(fp, "(Patch box)\n%d %d %d\n", file->mpr->patch_box[0], file->mpr->patch_box[1], file->mpr->patch_box[2]);
	    /* Write the number of out files */
	    fprintf(fp, "(Out file num)\n%d\n", file->mpr->out_file_num);
	    /* Write total number of patches */
	    fprintf(fp, "(Total patch number)\n%d\n", file->mpr->total_patches_num);
	    /* Write variables */
	    fprintf(fp, "(Fields)\n");
	    for (int i = 0; i < file->mpr->variable_count; i++)
	    {
	      fprintf(fp, "%s %s", file->variable[i]->var_name, file->variable[i]->type_name);
	      if (i != file->mpr->variable_count - 1)
	        fprintf(fp, " + \n");
	      else
	    	fprintf(fp, "\n");
	    }
	    /* Write number of process per node */
	    fprintf(fp, "(Process number per node)\n%d\n", file->mpr->proc_num_per_node);
	    fprintf(fp, "(Process number last node)\n%d\n", file->mpr->proc_num_last_node);
	    /* Write compression rate and mode */
	    if (file->mpr->compression_type == 0)
	    	fprintf(fp, "(Compression type)\nNo compression\n");
	    else if (file->mpr->compression_type == 1)
	    	fprintf(fp, "(Compression type)\nZFP accuracy\n");
	    else if (file->mpr->compression_type == 2)
	    	fprintf(fp, "(Compression type)\nZFP precision\n");
	    fprintf(fp, "(Compression bit rate)\n%f\n", file->mpr->compression_bit_rate);
	    fprintf(fp, "(Compression parameter)\n%f\n", file->mpr->compression_param);
	    /* Write wavalet levels */
	    fprintf(fp, "(Wavelet level)\n%d\n", file->mpr->wavelet_trans_num);
	    /* Write timesteps */
	    fprintf(fp, "(time)\n%d %d time%%09d/", file->mpr->first_tstep, file->mpr->current_time_step);

		fclose(fp);
	}
	return MPR_success;
}

MPR_return_code MPR_out_file_metadata_write_out(MPR_file file, int svi, int evi)
{
	char directory_path[PATH_MAX];
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

	char tmp_path[PATH_MAX];
	memset(tmp_path, 0, sizeof(*tmp_path) * PATH_MAX);
	sprintf(tmp_path, "%s_file_metadata", directory_path);

	// file name of out file related meta-data
	char out_file_info_path[PATH_MAX];
	memset(out_file_info_path, 0, sizeof(*out_file_info_path) * PATH_MAX);
	sprintf(out_file_info_path, "%s/file_%d", tmp_path, file->comm->simulation_rank);

	if (file->mpr->is_aggregator == 1)
	{
		FILE* fp = fopen(out_file_info_path, "w");
		if (fp == NULL)
		{
			fprintf(stderr, "Error: failed to open %s\n", out_file_info_path);
			return MPR_err_file;
		}
		else
		{
			for (int v = svi; v < evi; v++)
			{
				MPR_local_patch local_patch = file->variable[v]->local_patch;
				for (int i = 0; i < local_patch->agg_patch_count; i++)
					fprintf(fp, "%d %d %llu\n", v, local_patch->patch_id_array[i], local_patch->agg_patch_disps[i]);
			}
		}
		fclose(fp);
	}
	return MPR_success;
}

