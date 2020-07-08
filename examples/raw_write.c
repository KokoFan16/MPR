/*
 * MPR_raw_write.c
 *
 *  Created on: Jul 3, 2020
 *      Author: kokofan
 */


#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"

char var_name[MAX_VAR_COUNT][512];
int bpv[MAX_VAR_COUNT];
char type_name[MAX_VAR_COUNT][512];
int vps[MAX_VAR_COUNT];
MPR_variable* variable;
char input_file[512];
char output_file_template[512];
char var_list[512];
char output_file_name[512];
unsigned char **data;
static MPR_point rst_box;

static unsigned long long required_file_param = 0;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static void check_args();
static void read_file_parallel();
static void set_mpr_file(int ts);
static void set_mpr_variable(int var);

char *usage = "Serial Usage: ./idx_write -g 32x32x32 -l 32x32x32 -r 40x40x40 -v 2 -t 4 -f output_idx_file_name\n"
                     "Parallel Usage: mpirun -n 8 ./idx_write -g 64x64x64 -l 32x32x32 -r 40x40x40 -v 2 -t 4 -f output_idx_file_name\n"
                     "  -g: global dimensions\n"
                     "  -l: local (per-process) dimensions\n"
                     "  -r: restructured box dimension\n"
                     "  -f: file name template (without .idx)\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
                     "  -m: maximum file size\n";

int main(int argc, char **argv)
{
	int ts = 0, var = 0;
	/* Init MPI and MPI vars (e.g. rank and process_count) */
	init_mpi(argc, argv);

	/* Parse input arguments and initialize */
	parse_args(argc, argv);

	/* Check arguments */
	check_args();

	/* Initialize per-process local domain */
	calculate_per_process_offsets();

	/* Read file in parallel */
	read_file_parallel();

	create_mpr_point_and_access();

	/* Set variables */
	variable = (MPR_variable*)malloc(sizeof(*variable) * variable_count);
	memset(variable, 0, sizeof(*variable) * variable_count);

	for (ts = 0; ts < time_step_count; ts++)
	{
		set_mpr_file(ts);

	    for (var = 0; var < variable_count; var++)
	    	set_mpr_variable(var);

	    MPR_close(file);
	}

	if (MPR_close_access(p_access) != MPR_success)
		terminate_with_error_msg("MPR_close_access");

	free(variable);
	variable = 0;

	/* MPI close */
	shutdown_mpi();
	return 0;
}

/* Parse arguments */
static void parse_args(int argc, char **argv)
{
  char flags[] = "g:l:r:i:f:t:v:s:";
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

    case('l'): // local dimension
      if ((sscanf(optarg, "%dx%dx%d", &local_box_size[X], &local_box_size[Y], &local_box_size[Z]) == EOF) ||(local_box_size[X] < 1 || local_box_size[Y] < 1 || local_box_size[Z] < 1))
        terminate_with_error_msg("Invalid local dimension\n%s", usage);
      break;

    case('r'): // local dimension
      if ((sscanf(optarg, "%dx%dx%d", &rst_box_size[0], &rst_box_size[1], &rst_box_size[2]) == EOF) ||
          (rst_box_size[0] < 1 || rst_box_size[1] < 1 || rst_box_size[2] < 1))
        terminate_with_error_msg("Invalid restructuring box dimension\n%s", usage);
      MPR_set_point(rst_box, rst_box_size[X], rst_box_size[Y], rst_box_size[Z]);
      break;

    case('i'): // input file name
	  if (sprintf(input_file, "%s", optarg) < 0)
		terminate_with_error_msg("Invalid input file name\n%s", usage);
	  break;

    case('f'): // output file name
      if (sprintf(output_file_template, "%s", optarg) < 0)
        terminate_with_error_msg("Invalid output file name template\n%s", usage);
      sprintf(output_file_name, "%s%s", output_file_template, ".mpr");
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

    case('s'): // required parameter for out file (0 means the fixed size mode, 1 means fixed patch number)
      if (sscanf(optarg, "%lld", &required_file_param) < 0)
        terminate_with_error_msg("Invalid variable file\n%s", usage);
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

static void check_args()
{
  if (global_box_size[X] < local_box_size[X] || global_box_size[Y] < local_box_size[Y] || global_box_size[Z] < local_box_size[Z])
    terminate_with_error_msg("ERROR: Global box is smaller than local box in one of the dimensions\n");

  // check if the number of processes given by the user is consistent with the actual number of processes needed
  int brick_count = (int)((global_box_size[X] + local_box_size[X] - 1) / local_box_size[X]) *
                    (int)((global_box_size[Y] + local_box_size[Y] - 1) / local_box_size[Y]) *
                    (int)((global_box_size[Z] + local_box_size[Z] - 1) / local_box_size[Z]);
  if(brick_count != process_count)
    terminate_with_error_msg("ERROR: Number of sub-blocks (%d) doesn't match number of processes (%d)\n", brick_count, process_count);
}

/* Create a subarray for read non contiguous data */
MPI_Datatype create_subarray()
{
    MPI_Datatype subarray;
    MPI_Type_create_subarray(3, global_box_size, local_box_size, local_box_offset, MPI_ORDER_C, MPI_FLOAT, &subarray);
    MPI_Type_commit(&subarray);
    return subarray;
}

/* Read file in parallel */
static void read_file_parallel()
{
	data = malloc(sizeof(*data) * variable_count);
	memset(data, 0, sizeof(*data) * variable_count);

	int size = local_box_size[X] * local_box_size[Y] * local_box_size[Z]; // Local size

	data[0] = malloc(sizeof (*(data[0])) * size * (bpv[0]/8) * vps[0]); // The first variable
    MPI_Datatype subarray = create_subarray(); // Self-define MPI data type
    MPI_File fh;
    MPI_Status status;
    int count = 0;

    MPI_File_open(MPI_COMM_WORLD, input_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_FLOAT, subarray, "native", MPI_INFO_NULL);
    MPI_File_read(fh, data[0], size, MPI_FLOAT, &status);
    MPI_Get_count(&status, MPI_FLOAT, &count);
    if (count != size)
    	terminate_with_error_msg("ERROR: Read file failed!\n");
    MPI_File_close(&fh);

    // For other variables except first one, just copy the data of first variable.
	for (int var = 1; var < variable_count; var++)
	{
		data[var] = malloc(sizeof(*(data[var])) * size * (bpv[var]/8) * vps[var]);
		memcpy(data[var], data[0], sizeof(*(data[var])) * size * (bpv[var]/8) * vps[var]);
	}
}

static void set_mpr_file(int ts)
{
  MPR_return_code ret;

  ret = MPR_file_create(output_file_name, MPR_MODE_CREATE, p_access, global_size, &file);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_file_create\n");

  MPR_set_current_time_step(file, ts);   /* Set the current timestep */
  MPR_set_variable_count(file, variable_count);   /* Set the number of variables */
  MPR_set_io_mode(file, MPR_RAW_IO);   /* Select I/O mode */
  MPR_set_required_file_param(file, required_file_param);   /* Set required file size */
  MPR_set_restructuring_box(file, rst_box);  /* Set the restructuring box */

  return;
}

static void set_mpr_variable(int var)
{
  MPR_return_code ret = 0;

  // Set variable name, number of bits, typename
  ret = MPR_variable_create(var_name[var], bpv[var] * vps[var], type_name[var], &variable[var]);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_variable_create");

  ret = MPR_append_and_write_variable(file, variable[var]);
  if (ret != MPR_success)
	  terminate_with_error_msg("PIDX_append_and_write_variable");

  return;
}


