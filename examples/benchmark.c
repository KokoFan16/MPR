/*
 * benchmark.c
 *
 *  Created on: Jan 31, 2021
 *      Author: kokofan
 */


#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"
#include <errno.h>

char var_name[MAX_VAR_COUNT][512];
int bpv[MAX_VAR_COUNT];
char type_name[MAX_VAR_COUNT][512];
int vps[MAX_VAR_COUNT];
char input_file[512];
char output_file_template[512];
char var_list[512];

int* patch_sizes = NULL;
int* origin_patch_sizes = NULL;
int patch_count = 0;
int is_collective = 0;
int is_aggregator = 0;
int is_balanced = 0;

unsigned char* local_buffer = NULL;
unsigned char* recv_buffer = NULL;
long long int agg_size = 0;
long long int total_size = 0;

double agg_time = 0;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static int read_size_file(char* input_file);
static int linear_interpolation();
static int aggregation_perform();
static int generate_random_local_data();
static void write_data(int ts);
static void create_folder(char* data_path);

char *usage = "Parallel Usage: mpirun -n 8 ./benchmark -g 8x8x8 -p 8x8x8 -i input_file -v 2 -t 4 -f output_file_name -w 1 -o 4 -b 1\n"
                     "  -g: patch count dimensions (patch count in x y z)\n"
					 "  -p: patch count dimensions in input file (patch count in x y z)\n"
					 "  -i: input file name\n"
                     "  -f: file name template\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
					 "  -o: the number of out files\n"
					 "  -b: whether need balanced aggregation"
					 "  -w: write mode (1 means MPI collective I/O)\n";


int main(int argc, char **argv)
{
	int ts = 0;
	/* Init MPI and MPI vars (e.g. rank and process_count) */
	init_mpi(argc, argv);
	/* Parse input arguments and initialize */
	parse_args(argc, argv);
	/* Read histogram */
	double read_start = MPI_Wtime();
	read_size_file(input_file);
	double read_end = MPI_Wtime();
	double read_time = read_end - read_start;

	/* Create new histogram by using linear interpolation */
	double inter_start = MPI_Wtime();
	linear_interpolation();
	double inter_end = MPI_Wtime();
	double inter_time = inter_end - inter_start;

	/* Create local data per process */
	double gene_start = MPI_Wtime();
	generate_random_local_data();
	double gene_end = MPI_Wtime();
	double gene_time = gene_end - gene_start;

	double total_time = read_time + inter_time + gene_time;
	double max_time = 0;
	MPI_Allreduce(&total_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	if (total_time == max_time)
		printf("%d (%d %d): r %f i %f g %f\n", process_count, is_collective, is_balanced, read_time, inter_time, gene_time);

	for (ts = 0; ts < time_step_count; ts++)
	{
		double write_start = MPI_Wtime();
		write_data(ts);
		double write_end = MPI_Wtime();
		double write_time = write_end - write_start;
		double max_write_time = 0;
		MPI_Allreduce(&write_time, &max_write_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
		if (max_write_time == write_time)
			printf("%d (%d %d): %d a %f w %f\n", process_count, is_collective, is_balanced, ts, agg_time, write_time);
	}

	free(origin_patch_sizes);
	free(patch_sizes);
	free(local_buffer);
	free(recv_buffer);

	/* MPI close */
	shutdown_mpi();
	return 0;
}


static int read_size_file(char* input_file)
{
	int file_patch_count = patch_box_size[0] * patch_box_size[1] * patch_box_size[2];
	origin_patch_sizes = (int*)malloc(file_patch_count * sizeof(int));

	FILE *fp = fopen(input_file, "r");
	if (fp == NULL)
	{
	    fprintf(stderr, "Error Opening %s\n", input_file);
	    return MPR_err_file;
	}

	for (int i = 0; i < file_patch_count; i++)
		fscanf(fp, "%d\n", &origin_patch_sizes[i]);

	fclose(fp);

	return MPR_success;
}


static int linear_interpolation()
{
	int file_patch_count = patch_box_size[0] * patch_box_size[1] * patch_box_size[2]; // the patch count from original histogram
	patch_count = global_box_size[0] * global_box_size[1] * global_box_size[2]; // the new patch count
	patch_sizes = (int*)malloc(patch_count * sizeof(int));

	if (patch_count == file_patch_count)
		memcpy(patch_sizes, origin_patch_sizes, patch_count * sizeof(int));
	else
	{
		float factor_x = patch_box_size[0] / (float)global_box_size[0];
		float factor_y = patch_box_size[1] / (float)global_box_size[1];
		float factor_z = patch_box_size[2] / (float)global_box_size[2];

		for (int k = 0; k < global_box_size[2]; k++)
		{
			float z = k * factor_z;
			int z_int = (int)((k + 0.5f) * factor_z);
			float w = z - z_int;
			int z_int_p1 = (z_int + 1) < patch_box_size[2] ? (z_int + 1) : z_int;

			for (int j = 0; j < global_box_size[1]; j++)
			{
				float y = j * factor_y;
				int y_int = (int)((j + 0.5f) * factor_y);
				float u = y - y_int;
				int y_int_p1 = (y_int + 1) < patch_box_size[1] ? (y_int + 1) : y_int;

				for (int i = 0; i < global_box_size[0]; i++)
				{
					float x = i * factor_x;
					int x_int = (int)((i + 0.5f) * factor_x);
					float v = x - x_int;
					int x_int_p1 = (x_int + 1) < patch_box_size[0] ? (x_int + 1) : x_int;

					// find 8 neighbors
					int c000 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int];
					int c001 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int_p1];
					int c011 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int_p1 * patch_box_size[0] + x_int_p1];
					int c010 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int_p1 * patch_box_size[0] + x_int];
					int c100 = origin_patch_sizes[z_int_p1 * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int];
					int c101 = origin_patch_sizes[z_int_p1 * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int_p1];
					int c111 = origin_patch_sizes[z_int_p1 * patch_box_size[1] * patch_box_size[0] + y_int_p1 * patch_box_size[0] + x_int_p1];
					int c110 = origin_patch_sizes[z_int_p1 * patch_box_size[1] * patch_box_size[0] + y_int_p1 * patch_box_size[0] + x_int];

					// calculate interpolated values
					int index = k * global_box_size[1] * global_box_size[0] + j * global_box_size[0] + i;
					patch_sizes[index] = (c000 * (1 - v) * (1 - u) * (1 - w)
							+ c100 * v * (1 - u) * (1 - w)
							+ c010 * (1- v) * u * (1 - w)
							+ c001 * (1 - v) * (1 - u) * w
							+ c101 * v * (1 - u) * w
							+ c011 * (1 - v) * u * w
							+ c110 * v * u * (1 - w)
							+ c111 * v * u * w);
				}
			}
		}
	}

	total_size = 0;
	for (int i = 0; i < patch_count; i++)
		total_size += patch_sizes[i];

	return MPR_success;
}

static int generate_random_local_data()
{
	local_buffer = (unsigned char* )malloc(patch_sizes[rank]);
	for (int i = 0; i < patch_sizes[rank]; i++)
		local_buffer[i] = 'a' + (random() % 26);

	return MPR_success;
}


static void write_data(int ts)
{
	char* data_set_path = (char* )malloc(sizeof(*data_set_path) * 512);
	memset(data_set_path, 0, sizeof(*data_set_path) * 512);
	sprintf(data_set_path, "%s/time%09d/", output_file_template, ts);

	if (is_collective == 0)
	{
		create_folder(data_set_path);

		/* Aggregation */
		double agg_start = MPI_Wtime();
		aggregation_perform();
		double agg_end = MPI_Wtime();
		agg_time = agg_end - agg_start;

		char file_name[512];
		memset(file_name, 0, 512 * sizeof(*file_name));
		sprintf(file_name, "%s%d", data_set_path, rank);

		if (is_aggregator == 1)
		{
			int fp = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0664);
			if (fp == -1)
				terminate_with_error_msg("ERROR: Cannot open write file or file already exist!\n");
			long long int write_file_size = write(fp, recv_buffer, agg_size);
			if (write_file_size != agg_size)
				terminate_with_error_msg("ERROR: Write count is not correct!\n");
			close(fp);
		}
	}
	else if (is_collective == 1)
	{
		long long int offset = 0;
		for (int i = 0; i < rank; i++)
			offset += patch_sizes[i];

		char file_name[512];
		memset(file_name, 0, 512 * sizeof(*file_name));
		sprintf(file_name, "%s_time%09d_data", output_file_template, ts);

		MPI_File fh;
		MPI_Status status;
		MPI_Info info;
		MPI_Info_create(&info);
		MPI_Info_set(info, "romio_cb_write" , "enable");
		if (total_size > 1073741824)
		{
			MPI_Info_set(info, "striping_factor", "48");
			MPI_Info_set(info, "striping_unit", "8388608");
		}
		int err = MPI_File_open(MPI_COMM_WORLD, file_name, MPI_MODE_WRONLY | MPI_MODE_CREATE, info, &fh);
		if (err)
			terminate_with_error_msg("ERROR: MPI open file failed!\n");
		MPI_File_write_at_all(fh, offset, local_buffer, patch_sizes[rank], MPI_BYTE, &status);
		MPI_Info_free(&info);
		MPI_File_close(&fh);
	}
	else if (is_collective == 2)
	{
		create_folder(data_set_path);

		int color = rank / 8;
		MPI_Comm split_comm;
		MPI_Comm_split(MPI_COMM_WORLD, color, rank, &split_comm);

		int split_rank, split_size;
		MPI_Comm_rank(split_comm, &split_rank);
		MPI_Comm_size(split_comm, &split_size);

		long long int offset = 0;
		for (int i = 0; i < split_rank; i++)
		{
			int index = i + color * split_size;
			offset += patch_sizes[index];
		}

		char file_name[512];
		memset(file_name, 0, 512 * sizeof(*file_name));
		sprintf(file_name, "%s%d", data_set_path, color);

		MPI_File fh;
		MPI_Status status;
		int err = MPI_File_open(split_comm, file_name, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
		if (err)
			terminate_with_error_msg("ERROR: MPI open file failed!\n");
		MPI_File_write_at_all(fh, offset, local_buffer, patch_sizes[rank], MPI_BYTE, &status);
		MPI_File_close(&fh);

		MPI_Comm_free(&split_comm);
	}
	else
	{
		create_folder(data_set_path);

		char file_name[512];
		memset(file_name, 0, 512 * sizeof(*file_name));
		sprintf(file_name, "%s%d", data_set_path, rank);

		int fp = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0664);
		if (fp == -1)
			terminate_with_error_msg("ERROR: Cannot open write file or file already exist!\n");
		long long int write_file_size = write(fp, local_buffer, patch_sizes[rank]);
		if (write_file_size != patch_sizes[rank])
			terminate_with_error_msg("ERROR: Write count is not correct!\n");
		close(fp);
	}
	free(data_set_path);
}


static int calZOrder(int x, int y, int z)
{
	static const unsigned int B[] = {0x09249249, 0x030C30C3, 0x0300F00F, 0x030000FF};
	static const unsigned int S[] = {2, 4, 8, 16};

	x = (x | (x << S[3])) & B[3];
	x = (x | (x << S[2])) & B[2];
	x = (x | (x << S[1])) & B[1];
	x = (x | (x << S[0])) & B[0];

	y = (y | (y << S[3])) & B[3];
	y = (y | (y << S[2])) & B[2];
	y = (y | (y << S[1])) & B[1];
	y = (y | (y << S[0])) & B[0];

	z = (z | (z << S[3])) & B[3];
	z = (z | (z << S[2])) & B[2];
	z = (z | (z << S[1])) & B[1];
	z = (z | (z << S[0])) & B[0];

	int result = x | (y << 1) | (z << 2);
	return result;
}

static int aggregation_perform()
{
	/****************** Convert to z-order ********************/
	int patch_count_power2 = 0;  /* z-order count */
	int* patch_sizes_zorder = NULL;
	int* patch_ids_zorder = NULL;
	int next_2_power_xyz[MPR_MAX_DIMENSIONS]; /* (e.g., 3x3x3 -> 4x4x4)*/
	int max_d = 0;
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
	{
		if (global_box_size[i] > max_d)
			max_d = global_box_size[i];
	}
	patch_count_power2 = pow(pow(2, ceil(log2(max_d))), 3); /* 27 -> 64 */

	/* Reorder the patch id array with z-order curve */
	patch_sizes_zorder = (int*)malloc(patch_count_power2 * sizeof(int)); /* patch size with z-order */
	memset(patch_sizes_zorder, 0, patch_count_power2 * sizeof(int));
	patch_ids_zorder = (int*)malloc(patch_count_power2 * sizeof(int));  /* patch id with z-order */
	memset(patch_ids_zorder, -1, patch_count_power2 * sizeof(int));
	for (int z = 0; z < global_box_size[2]; z++)
	{
		for (int y = 0; y < global_box_size[1]; y++)
		{
			for (int x = 0; x < global_box_size[0]; x++)
			{
				int zorder = calZOrder(x, y, z);  /* Calculate the index with z-order */
				int index = z * global_box_size[1] * global_box_size[0] + y * global_box_size[0] + x;
				patch_sizes_zorder[zorder] = patch_sizes[index];
				patch_ids_zorder[zorder] = index;
			}
		}
	}

	int cur_agg_count = 0;
	long long int agg_sizes[out_file_num]; /* the current size of aggregators */
	memset(agg_sizes, 0, out_file_num * sizeof(long long int));

	int patch_assign_array[patch_count];
	memset(patch_assign_array, -1, patch_count * sizeof(int));

	int pcount = 0;
	if (is_balanced == 0)
	{
		int avg_patch_num = ceil((float)patch_count / out_file_num);
		for (int i = 0; i < patch_count_power2; i++)
		{
			if (patch_ids_zorder[i] > -1)
			{
				if (pcount == ((cur_agg_count + 1) * avg_patch_num))
					cur_agg_count++;
				patch_assign_array[patch_ids_zorder[i]] = cur_agg_count;
				agg_sizes[cur_agg_count] += patch_sizes_zorder[i];
				pcount++;
			}
		}
	}
	else
	{
		long long int total_size_tmp = total_size;
		long long int average_file_size = total_size_tmp / out_file_num;

		/* Patches assigned to aggregators */
		while (pcount < patch_count_power2 && cur_agg_count < out_file_num)
		{
			if (patch_ids_zorder[pcount] > -1)
			{
				if (agg_sizes[cur_agg_count] >= average_file_size)
				{
					total_size_tmp -= agg_sizes[cur_agg_count];
					cur_agg_count++;
					average_file_size = total_size_tmp / (out_file_num - cur_agg_count);
				}
				patch_assign_array[patch_ids_zorder[pcount]] = cur_agg_count;
				agg_sizes[cur_agg_count] += patch_sizes_zorder[pcount];
			}
			pcount++;
		}
	}

	int agg_ranks[out_file_num]; /* AGG Array */
	int gap = process_count / out_file_num;

	int cagg = 0;
	is_aggregator = 0;
	for (int i = 0; i < process_count; i+= gap)
	{
		if (cagg < out_file_num)
		{
			agg_ranks[cagg++] = i;
			if (rank == i)
				is_aggregator = 1;
		}
		else
			break;
	}

	agg_size = 0;
	for (int i = 0; i < out_file_num; i++)
	{
		if (rank == agg_ranks[i])
			agg_size = agg_sizes[i];
	}

	int recv_array[patch_count]; /* Local receive array per process */
	int recv_num = 0;  /* number of received number of patches per aggregator */
	for (int i = 0; i < patch_count; i++)
	{
		if (rank == agg_ranks[patch_assign_array[i]])
			recv_array[recv_num++] = i;
	}

	/* Data exchange */
	int comm_count =  recv_num + 1; /* the maximum transform times */
	MPI_Request* req = (MPI_Request*)malloc(comm_count * sizeof(MPI_Request));
	MPI_Status* stat = (MPI_Status*)malloc(comm_count * sizeof(MPI_Status));
	int req_id = 0;
	int offset = 0;

	if (is_aggregator == 1)
		recv_buffer = (unsigned char*)malloc(agg_size);

	for (int i = 0; i < recv_num; i++)
	{
		MPI_Irecv(&recv_buffer[offset], patch_sizes[recv_array[i]], MPI_BYTE, recv_array[i], recv_array[i], MPI_COMM_WORLD, &req[req_id]);
		offset += patch_sizes[recv_array[i]];
		req_id++;
	}

	MPI_Isend(local_buffer, patch_sizes[rank], MPI_BYTE, agg_ranks[patch_assign_array[rank]], rank, MPI_COMM_WORLD, &req[req_id]);
	req_id++;

	MPI_Waitall(req_id, req, stat);

	return MPR_success;
}


static void create_folder(char* data_path)
{
	char last_path[512] = {0};
	char this_path[512] = {0};
	char tmp_path[512] = {0};
	char* pos;

	if (rank == 0)
	{
		strcpy(this_path, data_path);
		if ((pos = strrchr(this_path, '/')))
		{
			pos[1] = '\0';
			if (strcmp(this_path, last_path) != 0)
			{
				/* make sure if this directory exists */
				strcpy(last_path, this_path);
				memset(tmp_path, 0, 512 * sizeof (char));
				/* walk up path and mkdir each segment */
				for (int j = 0; j < (int)strlen(this_path); j++)
				{
					if (j > 0 && this_path[j] == '/')
					{
						int ret = mkdir(tmp_path, S_IRWXU | S_IRWXG | S_IRWXO);
						if (ret != 0 && errno != EEXIST)
							terminate_with_error_msg("ERROR: Create data folder failed!\n");
					}
					tmp_path[j] = this_path[j];
				}
			}
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
}


static void parse_args(int argc, char **argv)
{
  char flags[] = "g:p:i:f:t:v:w:o:b:";
  int one_opt = 0;

  while ((one_opt = getopt(argc, argv, flags)) != EOF)
  {
    /* postpone error checking for after while loop */
    switch (one_opt)
    {

    case('g'): // global dimension
      if ((sscanf(optarg, "%dx%dx%d", &global_box_size[X], &global_box_size[Y], &global_box_size[Z]) == EOF) || (global_box_size[X] < 1 || global_box_size[Y] < 1 || global_box_size[Z] < 1))
        terminate_with_error_msg("Invalid global dimensions\n%s", usage);
      break;

    case('p'): // global dimension
      if ((sscanf(optarg, "%dx%dx%d", &patch_box_size[X], &patch_box_size[Y], &patch_box_size[Z]) == EOF) || (patch_box_size[X] < 1 || patch_box_size[Y] < 1 || patch_box_size[Z] < 1))
        terminate_with_error_msg("Invalid patch dimensions\n%s", usage);
      break;

    case('i'): // input file name
	  if (sprintf(input_file, "%s", optarg) < 0)
		terminate_with_error_msg("Invalid input file name\n%s", usage);
	  break;

    case('f'): // output file name
      if (sprintf(output_file_template, "%s", optarg) < 0)
        terminate_with_error_msg("Invalid output file name template\n%s", usage);
//      sprintf(output_file_name, "%s%s", output_file_template, ".mpr");
      break;

    case('t'): // number of timesteps
      if (sscanf(optarg, "%d", &time_step_count) < 0)
        terminate_with_error_msg("Invalid variable file\n%s", usage);
      break;

    case('v'): // number of variables
	  if (!isNumber(optarg)){ // the param is a file with the list of variables
		if (sprintf(var_list, "%s", optarg) > 0)
		  parse_var_list();
		else
		  terminate_with_error_msg("Invalid variable list file\n%s", usage);
	  }else { // the param is a number of variables (default: 1*float32)
		if (sscanf(optarg, "%d", &variable_count) > 0)
		  generate_vars();
		else
		  terminate_with_error_msg("Invalid number of variables\n%s", usage);
	  }
	  break;

    case('w'): // The number of out files
      if (sscanf(optarg, "%d", &is_collective) < 0 || is_collective > 3)
        terminate_with_error_msg("Invalid write mode (1 means MPI collective I/O)\n%s", usage);
      break;

    case('o'): // The number of out files
      if (sscanf(optarg, "%d", &out_file_num) < 0 || out_file_num > process_count)
        terminate_with_error_msg("Invalid number of out files\n%s", usage);
      break;

    case('b'): // The number of out files
      if (sscanf(optarg, "%d", &is_balanced) < 0 || is_balanced > 1)
        terminate_with_error_msg("Invalid aggregation mode (1 means balanced mode)\n%s", usage);
      break;

    default:
      terminate_with_error_msg("Wrong arguments\n%s", usage);
    }
  }
}

/* parse variable list */
static int parse_var_list()
{
  FILE *fp = fopen(var_list, "r");
  if (fp == NULL)
  {
    fprintf(stderr, "Error Opening %s\n", var_list);
    return MPR_err_file;
  }

  int variable_counter = 0, count = 0, len = 0;
  char *pch1;
  char line [ 512 ];

  while (fgets(line, sizeof (line), fp) != NULL)
  {
    line[strcspn(line, "\r\n")] = 0;

    if (strcmp(line, "(fields)") == 0)
    {
      if ( fgets(line, sizeof line, fp) == NULL)
        return MPR_err_file;
      line[strcspn(line, "\r\n")] = 0;
      count = 0;
      variable_counter = 0;

      while (line[X] != '(')
      {
        pch1 = strtok(line, " +");
        while (pch1 != NULL)
        {
          if (count == 0)
          {
            char* temp_name = strdup(pch1);
            strcpy(var_name[variable_counter], temp_name);
            free(temp_name);
          }

          if (count == 1)
          {
            len = strlen(pch1) - 1;
            if (pch1[len] == '\n')
              pch1[len] = 0;

            strcpy(type_name[variable_counter], pch1);
            int ret;
            int bits_per_sample = 0;
            int sample_count = 0;
            ret = MPR_values_per_datatype(type_name[variable_counter], &sample_count, &bits_per_sample);
            if (ret != MPR_success)  return MPR_err_file;

            bpv[variable_counter] = bits_per_sample;
            vps[variable_counter] = sample_count;
          }
          count++;
          pch1 = strtok(NULL, " +");
        }
        count = 0;

        if ( fgets(line, sizeof line, fp) == NULL)
          return MPR_err_file;
        line[strcspn(line, "\r\n")] = 0;
        variable_counter++;
      }
      variable_count = variable_counter;
    }
  }
  fclose(fp);
  return MPR_success;
}

/* Generate variables */
static int generate_vars(){

  int variable_counter = 0;

  for (variable_counter = 0; variable_counter < variable_count; variable_counter++){
    int ret;
    int bits_per_sample = 0;
    int sample_count = 0;
    char temp_name[512];
    char* temp_type_name = "1*float32";
    sprintf(temp_name, "var_%d", variable_counter);
    strcpy(var_name[variable_counter], temp_name);
    strcpy(type_name[variable_counter], temp_type_name);

    ret = MPR_values_per_datatype(temp_type_name, &sample_count, &bits_per_sample);
    if (ret != MPR_success)
    	return MPR_err_file;

    bpv[variable_counter] = bits_per_sample;
    vps[variable_counter] = sample_count;
  }

  return 0;
}

