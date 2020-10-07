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

/* Write meta-data out */
MPR_return_code MPR_metadata_write_out(MPR_file file, int svi, int evi)
{
	/* Write basic information out */
	if (MPR_basic_info_metadata_write_out(file) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* Write bounding box metadata out */
	if (MPR_bounding_box_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	/* Write file related metadata out */
	if (MPR_file_metadata_write_out(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
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


MPR_return_code MPR_bounding_box_metadata_write_out(MPR_file file, int svi, int evi)
{
	char directory_path[PATH_MAX]; /* file template */
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

	char bounding_meta_path[PATH_MAX]; /* the patch bounding box metadata */
	memset(bounding_meta_path, 0, sizeof(*bounding_meta_path) * PATH_MAX);
	sprintf(bounding_meta_path, "%s_bounding_box", directory_path);

	for (int v = svi; v < evi; v++) /* Loop all the variables */
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int bounding_buffer[MPR_MAX_DIMENSIONS * 2 * file->comm->simulation_nprocs]; /* the bounding box for all the processes*/
		MPI_Gather(local_patch->bounding_box, MPR_MAX_DIMENSIONS*2, MPI_INT, bounding_buffer, MPR_MAX_DIMENSIONS*2, MPI_INT, 0, file->comm->simulation_comm);

		int agg_ranks[file->comm->simulation_nprocs]; /* each element i specify whether process i is a aggregator */
		MPI_Gather(&file->mpr->is_aggregator, 1, MPI_INT, agg_ranks, 1, MPI_INT, 0, file->comm->simulation_comm);

		if (file->comm->simulation_rank == 0)
		{
			FILE* fp = fopen(bounding_meta_path, "a"); /* open file (append mode) */
		    if (!fp) /* Check file handle */
		    {
				fprintf(stderr, " [%s] [%d] Write bounding box metadata failed!\n", __FILE__, __LINE__);
				return -1;
		    }
			for (int i = 0; i < file->comm->simulation_nprocs; i++)
			{
				if (agg_ranks[i] == 1)
				{
				    fprintf(fp, "%d %d ", v, i); /* write variable and aggregator rank */
				    for (int j = 0; j < MPR_MAX_DIMENSIONS * 2; j++)
				    	fprintf(fp, "%d ", bounding_buffer[i*MPR_MAX_DIMENSIONS*2 + j]); /* write bounding box */
				    fprintf(fp, "\n");
				}
			}
			fclose(fp);
		}
	}
	return MPR_success;
}

MPR_return_code MPR_file_metadata_write_out(MPR_file file, int svi, int evi)
{
	if (file->mpr->is_aggregator == 1)
	{
		int meta_count = 0; /* the number of needed meta-data */
		unsigned char* meta_buffer = NULL; /* the buffer for meta-data */

		int MODE = file->mpr->io_type; /* write IO mode */
		if (MODE == MPR_RAW_IO || MODE == MPR_MUL_RES_IO)
		{
			meta_count = file->variable[svi]->local_patch->agg_patch_count + 1;
			meta_buffer = malloc(meta_count * sizeof(int));
			memcpy(meta_buffer, &file->variable[svi]->local_patch->agg_patch_count, sizeof(int));
			for (int i = 1; i < meta_count; i++)
				memcpy(&meta_buffer[i*sizeof(int)], &file->variable[svi]->local_patch->patch_id_array[i-1], sizeof(int));
		}
		else if (MODE == MPR_MUL_PRE_IO || MPR_MUL_RES_PRE_IO)
		{
			meta_count += file->mpr->variable_count;
			meta_buffer = malloc(meta_count * sizeof(unsigned long long));
			int meta_id = 0;

			int max_agg_patch_count = 0;
			for (int v = svi; v < evi; v++)
			{
				if (file->variable[v]->local_patch->agg_patch_count > max_agg_patch_count)
					max_agg_patch_count = file->variable[v]->local_patch->agg_patch_count;
				unsigned long long patch_count = (unsigned long long)file->variable[v]->local_patch->agg_patch_count;
				memcpy(&meta_buffer[meta_id*sizeof(unsigned long long)], &patch_count, sizeof(unsigned long long));
				meta_id++;
			}

			meta_count += max_agg_patch_count * file->mpr->variable_count * 3;
			meta_buffer = realloc(meta_buffer, meta_count * sizeof(unsigned long long));

			for (int v = svi; v < evi; v++)
			{
				MPR_local_patch local_patch = file->variable[v]->local_patch;
				for (int i = 0; i < local_patch->agg_patch_count; i++)
				{
					unsigned long long pid = (unsigned long long)local_patch->patch_id_array[i];
					unsigned long long poffset = local_patch->agg_patch_disps[i];
					unsigned long long psize = (unsigned long long)local_patch->agg_patch_size[i];

					memcpy(&meta_buffer[meta_id*sizeof(unsigned long long)], &pid, sizeof(unsigned long long));
					memcpy(&meta_buffer[(meta_id + local_patch->agg_patch_count * file->mpr->variable_count)*sizeof(unsigned long long)], &poffset, sizeof(unsigned long long));
					memcpy(&meta_buffer[(meta_id + local_patch->agg_patch_count * 2 * file->mpr->variable_count)*sizeof(unsigned long long)], &psize, sizeof(unsigned long long));
					meta_id++;
				}
			}

//			if (MODE == MPR_MUL_RES_PRE_IO)
//			{
//				for (int v = svi; v < evi; v++)
//				{
//					MPR_local_patch local_patch = file->variable[v]->local_patch;
//					for (int i = 0; i < local_patch->agg_patch_count; i++)
//					{
//
//					}
//				}
//			}
		}

//		if (file->comm->simulation_rank == 0)
//		{
//			for (int i = 0; i < meta_count; i++)
//			{
//				unsigned long long a;
//				memcpy(&a, &meta_buffer[i*sizeof(unsigned long long)], sizeof(unsigned long long));
//				printf("%llu\n", a);
//			}
//		}
		free(meta_buffer); /* Clean up */
	}
	return MPR_success;
}

