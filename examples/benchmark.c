/*
 * benchmark.c
 *
 *  Created on: Jan 31, 2021
 *      Author: kokofan
 */


#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"

char var_name[MAX_VAR_COUNT][512];
int bpv[MAX_VAR_COUNT];
char type_name[MAX_VAR_COUNT][512];
int vps[MAX_VAR_COUNT];
//MPR_variable* variable;
char input_file[512];
char output_file_template[512];
char var_list[512];

int* patch_sizes;
int* origin_patch_sizes;
int patch_count;

unsigned char* local_buffer;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static int read_size_file(char* input_file);
static int linear_interpolation();
static int aggregation_perform();
static int generate_random_local_data();

char *usage = "Parallel Usage: mpirun -n 8 ./benchmark -g 8x8x8 -p 8x8x8 -i input_file -v 2 -t 4 -f output_file_name\n"
                     "  -g: patch count dimensions (patch count in x y z)\n"
					 "  -p: patch count dimensions in input file (patch count in x y z)\n"
					 "  -i: input file name\n"
                     "  -f: file name template\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
					 "  -o: the number of out files\n"
					 "  -d: whether to dump the logs\n (1: dump the logs)";


int main(int argc, char **argv)
{
	int ts = 0, var = 0;
	/* Init MPI and MPI vars (e.g. rank and process_count) */
	init_mpi(argc, argv);

	/* Parse input arguments and initialize */
	parse_args(argc, argv);

	/* Read histogram */
	read_size_file(input_file);

	/* Create new histogram by using linear interpolation */
	linear_interpolation();

	/* Create local data per process */
	generate_random_local_data();

	/* Aggregation */
	aggregation_perform();

	/* MPI close */
	shutdown_mpi();
	return 0;
}


static int read_size_file(char* input_file)
{
	int file_patch_count = patch_box_size[0] * patch_box_size[1] * patch_box_size[2];
	origin_patch_sizes = malloc(file_patch_count * sizeof(int));

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

	if (patch_count == file_patch_count)
		patch_sizes = origin_patch_sizes;
	else
	{
		patch_sizes = malloc(patch_count * sizeof(int));

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
	return MPR_success;
}


static int generate_random_local_data()
{
	local_buffer = malloc(patch_sizes[rank]);
	for (int i = 0; i < patch_sizes[rank]; i++)
		local_buffer[i] = 'a' + (random() % 26);

	return MPR_success;
}



static int aggregation_perform()
{
	long long int total_size = 0;
	for (int i = 0; i < patch_count; i++)
		total_size += patch_sizes[i];

	long long int average_file_size = total_size / out_file_num;

	int cur_agg_count = 0;
	long long int agg_sizes[out_file_num]; /* the current size of aggregators */
	memset(agg_sizes, 0, out_file_num * sizeof(long long int));

	int patch_assign_array[patch_count];
	memset(patch_assign_array, -1, patch_count * sizeof(int));

	int under = 0;
	int pcount = 0;
	/* Patches assigned to aggregators */
	while (pcount < patch_count && cur_agg_count < out_file_num)
	{
		if (agg_sizes[cur_agg_count] >= average_file_size)
		{
			if (agg_sizes[cur_agg_count] >= average_file_size) // update average value
			{
				agg_sizes[cur_agg_count] -= patch_sizes[--pcount];
				under = 1 - under;
				total_size -= agg_sizes[cur_agg_count];
				cur_agg_count++;
				average_file_size = total_size / (out_file_num - cur_agg_count);
			}
		}
		patch_assign_array[pcount] = cur_agg_count;
		agg_sizes[cur_agg_count] += patch_sizes[pcount];
		pcount++;
	}

	int agg_ranks[out_file_num]; /* AGG Array */
	int gap = process_count / out_file_num;

	int cagg = 0;
	int is_aggregator = 0;
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

	long long int agg_size = 0;
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
	MPI_Request* req = malloc(comm_count * sizeof(MPI_Request));
	MPI_Status* stat = malloc(comm_count * sizeof(MPI_Status));
	int req_id = 0;
	int offset = 0;

	unsigned char* recv_buffer = NULL;
	if (is_aggregator == 1)
		recv_buffer = malloc(agg_size);

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


static void parse_args(int argc, char **argv)
{
  char flags[] = "g:p:i:f:t:v:o:d:";
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

    case('o'): // The number of out files
      if (sscanf(optarg, "%d", &out_file_num) < 0 || out_file_num > process_count)
        terminate_with_error_msg("Invalid number of out files\n%s", usage);
      break;

    case('d'): // is_log
      if (sscanf(optarg, "%d", &logs) < 0 || logs > 1)
        terminate_with_error_msg("Invalid logs parameter (0 or 1)\n%s", usage);
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

