
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
static MPR_point patch_box;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static void read_file_parallel();
static void set_mpr_file(int ts);
static void set_mpr_variable(int var);
static void create_synthetic_simulation_data();
static void destroy_data();

char *usage = "Serial Usage: ./multi_res_pre_write -g 32x32x32 -l 32x32x32 -p 40x40x40 -v 2 -t 4 -f output_idx_file_name -n 4 -o 4 -z 1 -c 0\n"
                     "Parallel Usage: mpirun -n 8 ./idx_write -g 64x64x64 -l 32x32x32 -p 40x40x40 -v 2 -t 4 -f output_idx_file_name -n 4 -o 4 -z 1 -c 0 -m 1 -e 1\n"
                     "  -g: global dimensions\n"
                     "  -l: local (per-process) dimensions\n"
                     "  -p: patch box dimension\n"
					 "  -i: input file name\n"
                     "  -f: file name template\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
					 "  -n: the number of processes per node\n"
					 "  -o: the number of out files\n"
					 "  -z: compression mode: (1: accuracy, 2: precision)\n"
		             "  -c: compression parameter(double)\n"
					 "  -m: aggregation mode (0: fixed-patch-count, 0: fixed-size)\n"
		             "  -e: aggregation order (0: row-order, 1: z-order)\n";

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

	/* If there is input file, read file in parallel, otherwise, create local simulation sdata */
	if (strcmp(input_file, "") == 0)
		create_synthetic_simulation_data();  /* Create local simulation data */
	else
	{
		if (rank == 0)
			printf("Read data from file %s\n", input_file);
		read_file_parallel();   /* Read file in parallel */
	}

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

	destroy_data();

	/* MPI close */
	shutdown_mpi();
	return 0;
}

/* Parse arguments */
static void parse_args(int argc, char **argv)
{
  char flags[] = "g:l:p:i:f:t:v:n:o:z:c:m:e:";
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

    case('p'): // local dimension
      if ((sscanf(optarg, "%dx%dx%d", &patch_box_size[0], &patch_box_size[1], &patch_box_size[2]) == EOF) ||
          (patch_box_size[0] < 1 || patch_box_size[1] < 1 || patch_box_size[2] < 1))
        terminate_with_error_msg("Invalid restructuring box dimension\n%s", usage);
      MPR_set_point(patch_box, patch_box_size[X], patch_box_size[Y], patch_box_size[Z]);
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

    case('n'): // The number of processes per node
      if (sscanf(optarg, "%d", &proc_num_per_node) == EOF || proc_num_per_node < 1)
        terminate_with_error_msg("Invalid number of processes per node\n%s", usage);
      break;

    case('o'): // The number of out files
      if (sscanf(optarg, "%d", &out_file_num) < 0 || out_file_num > process_count)
        terminate_with_error_msg("Invalid number of out files\n%s", usage);
      break;

    case('z'): // The number of out files
      if (sscanf(optarg, "%d", &compress_mode) < 1)
        terminate_with_error_msg("Invalid compression mode\n%s", usage);
      break;

    case('c'): // The number of out files
      if (sscanf(optarg, "%f", &compress_param) < 0)
        terminate_with_error_msg("Invalid compression parameter\n%s", usage);
      break;

    case('m'): // The number of out files
      if (sscanf(optarg, "%d", &is_fixed_file_size) < 0 || is_fixed_file_size > 1)
        terminate_with_error_msg("Invalid aggregation mode\n%s", usage);
      break;

    case('e'): // The number of out files
      if (sscanf(optarg, "%d", &is_z_order) < 0 || is_z_order > 1)
        terminate_with_error_msg("Invalid aggregation order (z-order or row-order)\n%s", usage);
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

/* Create a subarray for read non contiguous data */
MPI_Datatype create_subarray()
{
	int bytes = (bpv[0]/8) * vps[0];
	int tmp_global_box[NUM_DIMS] = {global_box_size[0] * bytes, global_box_size[1], global_box_size[2]};
	int tmp_local_box[NUM_DIMS] = {local_box_size[0] * bytes, local_box_size[1], local_box_size[2]};
	int tmp_local_offset[NUM_DIMS] = {local_box_offset[0] * bytes, local_box_offset[1], local_box_offset[2]};

	if (local_box_size[0] + local_box_offset[0] > global_box_size[0])
		tmp_local_offset[0] = (global_box_size[0] - local_box_offset[0]) * bytes;
	if (local_box_size[1] + local_box_offset[1] > global_box_size[1])
		tmp_local_offset[1] = global_box_size[1] - local_box_offset[1];
	if (local_box_size[2] + local_box_offset[2] > global_box_size[2])
		tmp_local_offset[2] = global_box_size[2] - local_box_offset[2];

    MPI_Datatype subarray;
    MPI_Type_create_subarray(3, tmp_global_box, tmp_local_box, tmp_local_offset, MPI_ORDER_FORTRAN, MPI_CHAR, &subarray);
    MPI_Type_commit(&subarray);
    return subarray;
}

/* Read file in parallel */
static void read_file_parallel()
{
	data = malloc(sizeof(*data) * variable_count);
	memset(data, 0, sizeof(*data) * variable_count);

	int bytes = (bpv[0]/8) * vps[0];
	int size = local_box_size[X] * local_box_size[Y] * local_box_size[Z] * bytes; // Local size

	data[0] = malloc(sizeof (*(data[0])) * size); // The first variable
    MPI_Datatype subarray = create_subarray(); // Self-define MPI data type
    MPI_File fh;
    MPI_Status status;
    int count = 0;

    MPI_File_open(MPI_COMM_WORLD, input_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_CHAR, subarray, "native", MPI_INFO_NULL);
    MPI_File_read(fh, data[0], size, MPI_CHAR, &status);
    MPI_Get_count(&status, MPI_CHAR, &count);
    if (count != size)
    	terminate_with_error_msg("ERROR: Read file failed!\n");
    else
    {
    	if (rank == 0)
    		printf("The file %s has been read successful!\n", input_file);
    }

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

  ret = MPR_file_create(output_file_name, MPR_MODE_CREATE, p_access, global_size, local_size, local_offset, patch_box, &file);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_file_create\n");

  MPR_set_current_time_step(file, ts);   /* Set the current timestep */
  MPR_set_variable_count(file, variable_count);   /* Set the number of variables */
  MPR_set_io_mode(file, MPR_MUL_RES_PRE_IO);   /* Select I/O mode */
  MPR_set_out_file_num(file, out_file_num);
  MPR_set_procs_num_per_node(file, proc_num_per_node);
  MPR_set_compression_mode(file, compress_mode);
  MPR_set_compression_parameter(file, compress_param);
  MPR_set_aggregation_mode(file, is_fixed_file_size);
  MPR_set_aggregation_order(file, is_z_order);

  return;
}

static void set_mpr_variable(int var)
{
  MPR_return_code ret = 0;

  // Set variable name, number of bits, typename
  ret = MPR_variable_create(var_name[var], bpv[var] * vps[var], type_name[var], &variable[var]);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_variable_create");

  ret = MPR_variable_write_data(variable[var], data[var]);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_variable_create");

  /* Set variable array */
  ret = MPR_append_and_write_variable(file, variable[var]);
  if (ret != MPR_success)
	  terminate_with_error_msg("MPR_append_and_write_variable");

  return;
}


static void create_synthetic_simulation_data()
{
  int var = 0;
  data = malloc(sizeof(*data) * variable_count);
  memset(data, 0, sizeof(*data) * variable_count);

  // Synthetic simulation data
  for (var = 0; var < variable_count; var++)
  {
    uint64_t i, j, k, val_per_sample = 0;
    data[var] = malloc(sizeof (*(data[var])) * local_box_size[X] * local_box_size[Y] * local_box_size[Z] * (bpv[var]/8) * vps[var]);

    unsigned char cvalue = 0;
    short svalue = 0;
    float fvalue = 0;
    double dvalue = 0;
    int ivalue = 0;
    uint64_t u64ivalue = 0;
    int64_t i64value = 0;

    for (k = 0; k < local_box_size[Z]; k++)
      for (j = 0; j < local_box_size[Y]; j++)
        for (i = 0; i < local_box_size[X]; i++)
        {
          uint64_t index = (uint64_t) (local_box_size[X] * local_box_size[Y] * k) + (local_box_size[X] * j) + i;

          for (val_per_sample = 0; val_per_sample < vps[var]; val_per_sample++)
          {
            if (strcmp(type_name[var], MPR_DType.UINT8) == 0 || strcmp(type_name[var], MPR_DType.UINT8_GA) == 0 || strcmp(type_name[var], MPR_DType.UINT8_RGB) == 0)
            {
              cvalue = (int)(var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i)));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(unsigned char), &cvalue, sizeof(unsigned char));
            }
            if (strcmp(type_name[var], MPR_DType.INT16) == 0 || strcmp(type_name[var], MPR_DType.INT16_GA) == 0 || strcmp(type_name[var], MPR_DType.INT16_RGB) == 0)
            {
              svalue = (int)(var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i)));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(short), &svalue, sizeof(short));
            }
            if (strcmp(type_name[var], MPR_DType.INT32) == 0 || strcmp(type_name[var], MPR_DType.INT32_GA) == 0 || strcmp(type_name[var], MPR_DType.INT32_RGB) == 0)
            {
              ivalue = (int)( 100 + var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i)));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(int), &ivalue, sizeof(int));
            }
            else if (strcmp(type_name[var], MPR_DType.FLOAT32) == 0 || strcmp(type_name[var], MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name[var], MPR_DType.FLOAT32_RGB) == 0)
            {
              fvalue = (float)( 100 + var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i)));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(float), &fvalue, sizeof(float));
            }
            else if (strcmp(type_name[var], MPR_DType.FLOAT64) == 0 || strcmp(type_name[var], MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name[var], MPR_DType.FLOAT64_RGB) == 0)
            {
              dvalue = (double) 100 + var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(double), &dvalue, sizeof(double));
            }
            else if (strcmp(type_name[var], MPR_DType.INT64) == 0 || strcmp(type_name[var], MPR_DType.INT64_GA) == 0 || strcmp(type_name[var], MPR_DType.INT64_RGB) == 0)
            {
              i64value = (int64_t) 100 + var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(int64_t), &i64value, sizeof(int64_t));
            }
            else if (strcmp(type_name[var], MPR_DType.UINT64) == 0 || strcmp(type_name[var], MPR_DType.UINT64_GA) == 0 || strcmp(type_name[var], MPR_DType.UINT64_RGB) == 0)
            {
              u64ivalue = (uint64_t) 100 + var + val_per_sample + ((global_box_size[X] * global_box_size[Y]*(local_box_offset[Z] + k))+(global_box_size[X]*(local_box_offset[Y] + j)) + (local_box_offset[X] + i));
              memcpy(data[var] + (index * vps[var] + val_per_sample) * sizeof(uint64_t), &u64ivalue, sizeof(uint64_t));
            }
          }
        }
  }
}


static void destroy_data()
{
  int var = 0;
  for (var = 0; var < variable_count; var++)
  {
    free(data[var]);
    data[var] = 0;
  }
  free(data);
  data = 0;
}

