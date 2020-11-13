/*
 * metadata_io.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */


#include <MPR.h>
#include <errno.h>

static int intersect_patch(int* a_size, int* a_offset, int* b_size, int* b_offset);

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
	    fprintf(fp, "(IO mode)\n%d\n", file->mpr->io_type);

	    /* Write global box and patch box */
	    fprintf(fp, "(Global box)\n%d %d %d\n", file->mpr->global_box[0], file->mpr->global_box[1], file->mpr->global_box[2]);
	    fprintf(fp, "(Patch box)\n%d %d %d\n", file->mpr->patch_box[0], file->mpr->patch_box[1], file->mpr->patch_box[2]);
	    /* Write the number of out files */
	    fprintf(fp, "(File num)\n%d\n", file->mpr->out_file_num);
	    /* Write total number of patches */
	    fprintf(fp, "(Patch count)\n%d\n", file->mpr->total_patches_num);
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
	    fprintf(fp, "(Pnum per node)\n%d\n", file->mpr->proc_num_per_node);
	    fprintf(fp, "(Pnum last)\n%d\n", file->mpr->proc_num_last_node);
	    /* Write compression rate and mode */
	    fprintf(fp, "(Compression)\n%d %f \n", file->mpr->compression_type, file->mpr->compression_param);

	    /* Write wavalet levels */
	    fprintf(fp, "(Res level)\n%d\n", file->mpr->wavelet_trans_num);
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

	unsigned char* meta_buffer = NULL; /* The buffer for dounding box meta-data */

	int agg_ranks[file->comm->simulation_nprocs]; /* each element i specify whether process i is a aggregator */
	MPI_Gather(&file->mpr->is_aggregator, 1, MPI_INT, agg_ranks, 1, MPI_INT, 0, file->comm->simulation_comm);

	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch of the first variable */
	int bounding_buffer[MPR_MAX_DIMENSIONS * 2 * file->comm->simulation_nprocs]; /* Gather bounding box array per process to rank 0 */
	MPI_Gather(local_patch->bounding_box, MPR_MAX_DIMENSIONS*2, MPI_INT, bounding_buffer, MPR_MAX_DIMENSIONS*2, MPI_INT, 0, file->comm->simulation_comm);

	int MODE = file->mpr->io_type; /* Write IO type */
	int meta_count = (MPR_MAX_DIMENSIONS * 2 + 1) * file->mpr->out_file_num; /* The number of metadata count */
	int meta_id = 0; /* The current meta-data index */
	if (file->comm->simulation_rank == 0)
	{
		meta_buffer = malloc(meta_count * sizeof(int)); /* Allocate memory for meta-data buffer */
		for (int i = 0; i < file->comm->simulation_nprocs; i++)
		{
			if (agg_ranks[i] == 1)
			{
				memcpy(&meta_buffer[meta_id * sizeof(int)], &i, sizeof(int)); /* store the aggregator id */
				meta_id++;
				memcpy(&meta_buffer[meta_id * sizeof(int)], &bounding_buffer[i*MPR_MAX_DIMENSIONS*2], MPR_MAX_DIMENSIONS*2*sizeof(int));
				meta_id += MPR_MAX_DIMENSIONS*2;
			}
		}
	}

	if (MODE == MPR_MUL_PRE_IO || MODE == MPR_MUL_RES_PRE_IO) /* For these two modes, we should store all the bounding box for all the variables */
	{
		meta_count += meta_count * (file->mpr->variable_count - 1);
		if (file->comm->simulation_rank == 0) /* re-allocate memory for metadata buffer */
			meta_buffer = realloc(meta_buffer, meta_count * sizeof(int));

		for (int v = svi + 1; v < evi; v++) /* loop all the variables except first one */
		{
			local_patch = file->variable[v]->local_patch; /* Gather all the bounding box from other processes */
			MPI_Gather(local_patch->bounding_box, MPR_MAX_DIMENSIONS*2, MPI_INT, bounding_buffer, MPR_MAX_DIMENSIONS*2, MPI_INT, 0, file->comm->simulation_comm);

			if (file->comm->simulation_rank == 0)
			{
				for (int i = 0; i < file->comm->simulation_nprocs; i++)
				{
					if (agg_ranks[i] == 1)
					{
						memcpy(&meta_buffer[meta_id * sizeof(int)], &i, sizeof(int)); /* store the aggregator id */
						meta_id++;
						memcpy(&meta_buffer[meta_id * sizeof(int)], &bounding_buffer[i*MPR_MAX_DIMENSIONS*2], MPR_MAX_DIMENSIONS*2*sizeof(int));
						meta_id += MPR_MAX_DIMENSIONS*2;
					}
				}
			}
		}
	}

	/* Write bounding box metadata out */
	if (file->comm->simulation_rank == 0)
	{
		int fp = open(bounding_meta_path, O_CREAT | O_EXCL | O_WRONLY, 0664);
		if (fp == -1)
		{
			fprintf(stderr, "File %s is existed, please delete it.\n", bounding_meta_path);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		int write_count = write(fp, meta_buffer, meta_count * sizeof(int));
		if (write_count != meta_count * sizeof(int))
		{
			fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		close(fp);
	}

	free(meta_buffer);
	return MPR_success;
}

MPR_return_code MPR_file_metadata_write_out(MPR_file file, int svi, int evi)
{
	char directory_path[PATH_MAX]; /* file template */
	memset(directory_path, 0, sizeof(*directory_path) * PATH_MAX);
	strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

	if (file->mpr->is_aggregator == 1)
	{
		int meta_count = 1; /* the number of needed meta-data */
		unsigned char* meta_buffer = NULL; /* the buffer for meta-data */

		int MODE = file->mpr->io_type; /* write IO mode */
		if (MODE == MPR_RAW_IO || MODE == MPR_MUL_RES_IO) /* No compression involves */
		{
			meta_count += file->variable[svi]->local_patch->agg_patch_count; /* the number of patches per file */
			meta_buffer = malloc(meta_count * sizeof(int)); /* allocate memory */
			memcpy(meta_buffer, &meta_count, sizeof(int));
			for (int i = 1; i < meta_count; i++) /* the patch id */
				memcpy(&meta_buffer[i*sizeof(int)], &file->variable[svi]->local_patch->agg_patch_id_array[i-1], sizeof(int));
		}
		else if (MODE == MPR_MUL_PRE_IO || MODE == MPR_MUL_RES_PRE_IO) /* Compression involves */
		{
			meta_count += file->mpr->variable_count * 2; /* the patch count for aggregator per variable */
			meta_buffer = malloc(meta_count * sizeof(int));
			int meta_id = 1; /* the first one should be the total number of meta-data */

			int vars_agg_patch_count = 0; /* the total patch count for the aggregator across all the variables */
			for (int v = svi; v < evi; v++)
			{
				vars_agg_patch_count += file->variable[v]->local_patch->agg_patch_count;
				/* Meta-data: the patch count per variable */
				memcpy(&meta_buffer[meta_id*sizeof(int)], &file->variable[v]->local_patch->agg_patch_count, sizeof(int));
				memcpy(&meta_buffer[(meta_id + file->mpr->variable_count)*sizeof(int)], &file->variable[v]->local_patch->out_file_size, sizeof(int));
				meta_id++;
			}

			meta_id += file->mpr->variable_count;
			meta_count += vars_agg_patch_count * 3; /* Meta-data: patch-id, patch-offset, patch-size */
			meta_buffer = realloc(meta_buffer, meta_count * sizeof(int));

			for (int v = svi; v < evi; v++)
			{
				MPR_local_patch local_patch = file->variable[v]->local_patch;
				for (int i = 0; i < local_patch->agg_patch_count; i++)
				{
					int poffset_id = meta_id + local_patch->agg_patch_count * file->mpr->variable_count;
					int psize_id = meta_id + local_patch->agg_patch_count * file->mpr->variable_count * 2;

					memcpy(&meta_buffer[meta_id * sizeof(int)], &local_patch->agg_patch_id_array[i], sizeof(int));
					memcpy(&meta_buffer[poffset_id * sizeof(int)], &local_patch->agg_patch_disps[i], sizeof(int));
					memcpy(&meta_buffer[psize_id * sizeof(int)], &local_patch->agg_patch_size[i], sizeof(int));
					meta_id++;
				}
			}
			meta_id += vars_agg_patch_count * 2;

			if (MODE == MPR_MUL_RES_PRE_IO)
			{
				int subbands_num = file->mpr->wavelet_trans_num * 7 + 1; /* the number of sub-bands per patch */
				meta_count += vars_agg_patch_count * subbands_num; /* Meta-data: size of each sub-band */
				meta_buffer = realloc(meta_buffer, meta_count * sizeof(int));

				for (int v = svi; v < evi; v++)
				{
					MPR_local_patch local_patch = file->variable[v]->local_patch;
					for (int i = 0; i < local_patch->agg_patch_count; i++)
					{
						memcpy(&meta_buffer[meta_id * sizeof(int)], &local_patch->agg_subbands_size[i*subbands_num], subbands_num*sizeof(int));
						meta_id += subbands_num;
					}
				}
			}
			memcpy(&meta_buffer[0], &meta_count, sizeof(int)); /* the first metadata should be the total number of metadata*/
		}

		/* The file name for out files */
		char file_name[PATH_MAX];
		memset(file_name, 0, PATH_MAX * sizeof(*file_name));
		sprintf(file_name, "%s/time%09d/%d", directory_path, file->mpr->current_time_step, file->comm->simulation_rank);

		/* write meta-data out */
		int fp = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0664);
		if (fp == -1)
		{
			fprintf(stderr, "File %s is existed, please delete it.\n", file_name);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		int write_count = write(fp, meta_buffer, meta_count * sizeof(int));
		if (write_count != meta_count * sizeof(int))
		{
		  fprintf(stderr, "[%s] [%d] pwrite() failed.\n", __FILE__, __LINE__);
		  MPI_Abort(MPI_COMM_WORLD, -1);
		}
		close(fp);

		free(meta_buffer); /* Clean up */
	}
	return MPR_success;
}

/* Parse the metadata of basic information (xx.mpr) */
MPR_return_code MPR_basic_metatda_parse(char* file_name, MPR_file* file)
{
	if ((*file)->comm->simulation_rank == 0)
	{
		FILE *fp = fopen(file_name, "r");
	    if (fp == NULL)
	    {
	    	fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
	    	return MPR_err_file;
	    }
		char line [512];
		char* pch;
		int count;
		while (fgets(line, sizeof (line), fp) != NULL)
		{
			line[strcspn(line, "\r\n")] = 0;

			if (strcmp(line, "(IO mode)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->io_type = atoi(line);
			}

			if (strcmp(line, "(Global box)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;

				pch = strtok(line, " ");
				count = 0;
				while (pch != NULL)
				{
					(*file)->mpr->origin_global_box[count] = atoi(pch);
					count++;
					pch = strtok(NULL, " ");
				}
			}

			if (strcmp(line, "(Patch box)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;

				pch = strtok(line, " ");
				count = 0;
				while (pch != NULL)
				{
					(*file)->mpr->patch_box[count] = atoi(pch);
					count++;
					pch = strtok(NULL, " ");
				}
			}

			if (strcmp(line, "(File num)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->out_file_num = atoi(line);
			}

			if (strcmp(line, "(Patch count)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->total_patches_num = atoi(line);
			}

			if (strcmp(line, "(Fields)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;

				count = 0;
				int variable_counter = 0;
				while (line[0] != '(')
				{
			        (*file)->variable[variable_counter] = malloc(sizeof (*((*file)->variable[variable_counter])));
			        if ((*file)->variable[variable_counter] == NULL)
			        	return MPR_err_file;
			        memset((*file)->variable[variable_counter], 0, sizeof (*((*file)->variable[variable_counter])));

			        pch = strtok(line, " +");
			        while (pch != NULL)
			        {
			            if (count == 0)
			            {
							char* temp_name = strdup(pch);
							strcpy((*file)->variable[variable_counter]->var_name, temp_name);
							free(temp_name);
			            }

			            if (count == 1)
			            {
							int len = strlen(pch) - 1;
							if (pch[len] == '\n')
							  pch[len] = 0;

							strcpy((*file)->variable[variable_counter]->type_name, pch);
							int bits_per_sample = 0;
							int ret = MPR_default_bits_per_datatype((*file)->variable[variable_counter]->type_name, &bits_per_sample);
							if (ret != MPR_success)
							  return MPR_err_file;

							(*file)->variable[variable_counter]->bpv = bits_per_sample;
							(*file)->variable[variable_counter]->vps = 1;
			            }
			            count++;
			        	pch = strtok(NULL, " +");
			        }
			        count = 0;

			        if ( fgets(line, sizeof line, fp) == NULL)
			        	return MPR_err_file;
			        line[strcspn(line, "\r\n")] = 0;
			        variable_counter++;
				}
				(*file)->mpr->variable_count = variable_counter;
			}

			if (strcmp(line, "(Pnum per node)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->proc_num_per_node = atoi(line);
			}

			if (strcmp(line, "(Pnum last)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->proc_num_last_node = atoi(line);
			}

			if (strcmp(line, "(Compression)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;

				pch = strtok(line, " ");
				count = 0;
				while (pch != NULL)
				{
					if (count == 0)
						(*file)->mpr->compression_type = atoi(pch);
					if (count == 1)
						(*file)->mpr->compression_param = atof(pch);
					count++;
					pch = strtok(NULL, " ");
				}
			}

			if (strcmp(line, "(Res level)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;
				(*file)->mpr->wavelet_trans_num = atoi(line);
			}

			if (strcmp(line, "(time)") == 0)
			{
				if ( fgets(line, sizeof line, fp) == NULL)
					return MPR_err_file;
				line[strcspn(line, "\r\n")] = 0;

				pch = strtok(line, " ");
				count = 0;
				while (pch != NULL)
				{
					if (count == 0)
						(*file)->mpr->first_tstep = atoi(pch);
					if (count == 1)
						(*file)->mpr->last_tstep = atoi(pch);
					if (count == 2)
					{
						strcpy((*file)->mpr->filename_time_template, pch);
//						replace_str((*file)->mpr->filename_time_template, "%","%%");
					}
					count++;
					pch = strtok(NULL, " ");
				}
			}
		}
		fclose(fp);
	}
	return MPR_success;
}


MPR_return_code MPR_bounding_box_metatda_parse(char* file_name, MPR_file file)
{
	int count = file->mpr->out_file_num * 7; /* The meta-data count */
	int* buffer = malloc(count * sizeof(int));  /* buffer for bounding box */

	FILE * fp = fopen(file_name, "r"); /* Open bounding box meta-data file */
	if (fp == NULL)
	{
    	fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
    	return MPR_err_file;
	}
	int read_size = fread(buffer, sizeof(int), count, fp); /* Read data */
	if (read_size != count)
	{
    	fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
    	return MPR_err_file;
	}
	fclose(fp);

	int file_num = 0;  /* The number files which need to be opened for each process */
	int file_id[file->mpr->out_file_num]; /* The id of files which need to be opened for each process */

	int local_offset[MPR_MAX_DIMENSIONS]; /* The real offset to read in origin dataset */
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++) /* The current offset plus the global offset to read */
		local_offset[i] = file->mpr->local_offset[i] + file->mpr->global_offset[i];

	/* Loop the bounding box of each file */
	for (int i = 0; i < file->mpr->out_file_num; i++)
	{
		/* bounding offset and size of each file */
		int bounding_offset[MPR_MAX_DIMENSIONS] = {buffer[i*7+1] * file->mpr->patch_box[0], buffer[i*7+2]* file->mpr->patch_box[1], buffer[i*7+3] * file->mpr->patch_box[2]};
		int bounding_size[MPR_MAX_DIMENSIONS] = {(buffer[i*7+4] - buffer[i*7+1]) * file->mpr->patch_box[0], (buffer[i*7+5] - buffer[i*7+2]) * file->mpr->patch_box[1], (buffer[i*7+6] - buffer[i*7+3]) * file->mpr->patch_box[2]};
		/* Check whether need to open the current file */
		int ret = intersect_patch(bounding_size, bounding_offset, file->mpr->local_box, local_offset);
		if (ret)
			file_id[file_num++] = buffer[i*7];
	}
	free(buffer);

	file->mpr->open_file_num = file_num;
	file->mpr->open_file_ids = malloc(file_num * sizeof(int));
	memcpy(file->mpr->open_file_ids, file_id, file_num * sizeof(int));

	return MPR_success;
}


MPR_return_code MPR_file_related_metadata_parse(char* file_name, MPR_file file, int var_id, int* patches_offset, int* patches_size, int* patches_subbands)
{
	memset(patches_offset, -1, file->mpr->total_patches_num * sizeof(int));
	memset(patches_size, -1, file->mpr->total_patches_num * sizeof(int));

	int* buffer = malloc(sizeof(int));  /* buffer for file meta-data */

	FILE * fp = fopen(file_name, "r"); /* Open bounding box meta-data file */
	if (fp == NULL)
	{
    	fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
    	return MPR_err_file;
	}
	fread(buffer, sizeof(int), 1, fp); /* Read first data (the size of meta-data) */
	int meta_count = buffer[0]; /* get the size of meta-data*/
	fseek(fp, 0L, SEEK_SET);

	buffer = realloc(buffer, meta_count * sizeof(int));
	int read_count = fread(buffer, sizeof(int), meta_count, fp); /* Read all the meta-data */
	if (read_count != meta_count)
	{
    	fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
    	return MPR_err_file;
	}
	fclose(fp);
	file->mpr->file_metadata_count = meta_count;

	int bytes = file->variable[var_id]->vps * file->variable[var_id]->bpv/8; /* bytes per data */
	int metadata_size = file->mpr->file_metadata_count * bytes; /* the size of meta-data */

	int patch_count = 0; /* patch count of each process */
	int MODE = file->mpr->io_type; /* write IO mode */
	if (MODE == MPR_RAW_IO || MODE == MPR_MUL_RES_IO) /* No compression involves */
	{
		int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2] * bytes;
		patch_count = meta_count - 1;
		int var_size = var_id * patch_count * patch_size; /* The size of variable */
		for (int i = 0; i < patch_count; i++)
		{
			patches_offset[buffer[(i + 1)]] = metadata_size + var_size + i * patch_size;
			patches_size[buffer[(i + 1)]] = patch_size;
		}
	}
	else if (MODE == MPR_MUL_PRE_IO || MODE == MPR_MUL_RES_PRE_IO)
	{
		int total_var_patch_counts = 0;   /* the number of patch counts per file across all the variables */
		int var_patch_counts[file->mpr->variable_count];  /* each i is the number of patch count of variable i */
		int var_sizes[file->mpr->variable_count]; /* each i is the total size of variable i */
		for (int i = 0; i < file->mpr->variable_count; i++)
		{
			var_patch_counts[i] = buffer[i + 1];
			total_var_patch_counts += buffer[i + 1];
			var_sizes[i] = buffer[i + 1 + file->mpr->variable_count];
		}

		int patch_id_meta_offset = file->mpr->variable_count * 2 + 1; /* read offset for patch id */
		int patch_disp_meta_offset = file->mpr->variable_count * 2 + 1 + total_var_patch_counts; /* read offset for patch disp */
		int patch_size_meta_offset = file->mpr->variable_count * 2 + 1 + total_var_patch_counts * 2; /* read offset for patch size */
		int var_size = 0;
		for (int i = 0; i < var_id; i++)
		{
			patch_id_meta_offset += var_patch_counts[i];
			patch_disp_meta_offset += var_patch_counts[i];
			patch_size_meta_offset += var_patch_counts[i];
			var_size += var_sizes[i];
		}

		for (int i = 0; i < var_patch_counts[var_id]; i++)
		{
			int patch_id = buffer[patch_id_meta_offset + i];
			patches_offset[patch_id] = buffer[patch_disp_meta_offset + i] + metadata_size + var_size;
			patches_size[patch_id] = buffer[patch_size_meta_offset + i];
		}

		if (MODE == MPR_MUL_RES_PRE_IO)
		{
			int subband_num = file->mpr->wavelet_trans_num * 7 + 1;
			memset(patches_subbands, -1, file->mpr->total_patches_num * subband_num * sizeof(int));

			 /* read offset for sub-bands */
			int suband_meta_offset = file->mpr->variable_count * 2 + 1 + total_var_patch_counts * 3;
			for (int i = 0; i < var_id; i++)
				suband_meta_offset += var_patch_counts[i];

			for (int i = 0; i < var_patch_counts[var_id]; i++)
			{
				int patch_id = buffer[patch_id_meta_offset + i];
				memcpy(&patches_subbands[patch_id * subband_num], &buffer[suband_meta_offset + i * subband_num], subband_num * sizeof(int));

			}
		}
	}

//	if (file->comm->simulation_rank == 0)
//	{
//		for (int i = 0; i < meta_count; i++)
//		{
//			printf("%d\n", buffer[i]);
//		}
//	}

	free(buffer);
	return MPR_success;
}


static int intersect_patch(int* a_size, int* a_offset, int* b_size, int* b_offset)
{
	int d = 0, check_bit = 0;
	for (d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		check_bit = check_bit || (a_offset[d] + a_size[d] - 1) < b_offset[d] || (b_offset[d] + b_size[d] - 1) < a_offset[d];
	}
	return !(check_bit);
}

