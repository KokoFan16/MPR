
#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"

//#include "../include/logging_api.h"
#include <caliper/cali.h>
#include <caliper/cali-manager.h>

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

int agg_version;

int ntimestep = 1;
int curTs = 0;
int nprocs = 1;
int curRank = 0;
std::string namespath = "";

char configstr[512];

float* time_buffer;

double cali_cost = 0;
double write_cost = 0;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static void read_file_parallel();
static void set_mpr_file(int ts);
static void set_mpr_variable(int var);
static void create_synthetic_simulation_data();
static void destroy_data();

char *usage = "Parallel Usage: mpirun -n 8 ./multi_res_pre_write -g 64x64x64 -l 32x32x32 -p 40x40x40 -v 2 -t 4 -f output_file_name -o 4 -z 1 -c 0 -m 1\n"
                     "  -g: global dimensions\n"
                     "  -l: local (per-process) dimensions\n"
                     "  -p: patch box dimension\n"
					 "  -i: input file name\n"
                     "  -f: file name template\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
					 "  -o: the number of out files\n"
					 "  -z: compression mode: (0: accuracy, 1: precision)\n"
		             "  -c: compression parameter(double)\n"
					 "  -m: aggregation mode (0: fixed-patch-count, 1: fixed-size)\n"
					 "  -d: whether to dump the logs\n (1: dump the logs)";

int main(int argc, char **argv)
{
	cali_config_set("CALI_CALIPER_ATTRIBUTE_DEFAULT_SCOPE", "process");

	int ts = 0, var = 0;
	/* Init MPI and MPI vars (e.g. rank and process_count) */
	init_mpi(argc, argv);

	/* Parse input arguments and initialize */
	parse_args(argc, argv);

	double start = MPI_Wtime();
	cali::ConfigManager mgr;
	mgr.add(configstr);
    mgr.start();
	double end = MPI_Wtime();
	write_cost += (end - start);

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

//	int time_count = 42; //9
//	time_buffer = (float*)malloc(time_step_count * time_count * sizeof(float));
//	memset(time_buffer, 0, time_step_count * time_count * sizeof(float));
//	size_buffer = (long long int*)malloc(time_step_count * 5 * sizeof(long long int));
//	memset(size_buffer, 0, time_step_count * 5 * sizeof(long long int));

//	CALI_CXX_MARK_LOOP_BEGIN(mainloop, "main");
	double start_time = MPI_Wtime();

	start = MPI_Wtime();
	CALI_MARK_BEGIN("main");
	end = MPI_Wtime();
	cali_cost += (end - start);

	for (ts = 0; ts < time_step_count; ts++)
	{
		set_rank(rank, process_count);
		set_timestep(ts, time_step_count);
		set_namespath("");

//		Events e("main", "null");

		set_mpr_file(ts);

		for (var = 0; var < variable_count; var++)
			set_mpr_variable(var);

		MPR_close(file);

	}

	start = MPI_Wtime();
	CALI_MARK_END("main");
	end = MPI_Wtime();
	cali_cost += (end - start);

	double end_time = MPI_Wtime();
	double total_time = (end_time-start_time);
	double max_time;
	MPI_Reduce(&total_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (rank == 0) { printf("Caliper-time: %f\n", max_time); }

	MPI_Barrier(MPI_COMM_WORLD);

	start = MPI_Wtime();
	mgr.flush();
	end = MPI_Wtime();
	write_cost += (end - start);

	double cost_time = write_cost + cali_cost;
	double max_cost;
	MPI_Allreduce(&cost_time, &max_cost, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	if (cost_time == max_cost) { printf("caliper-cost: %f(%f, %f)\n", max_cost, cali_cost, write_cost); }

	//	std::string filename = "parallel_io_mulResPre_" + std::to_string(time_step_count) + "_" + std::to_string(process_count);
	//	write_output(filename);
	//	free(time_buffer);
	//	free(size_buffer);

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
  char flags[] = "g:l:p:i:f:t:v:o:z:c:m:d:a:P:";
  int one_opt = 0;

  while ((one_opt = myGetopt(argc, argv, flags)) != EOF)
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

    case('p'): // patch dimension
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

    case('o'): // The number of out files
      if (sscanf(optarg, "%d", &out_file_num) < 0 || out_file_num > process_count)
        terminate_with_error_msg("Invalid number of out files\n%s", usage);
      break;

    case('z'): // zfp mode
      if (sscanf(optarg, "%d", &compress_mode) < 0 || compress_mode > 1)
        terminate_with_error_msg("Invalid compression mode\n%s", usage);
      break;

    case('c'): // zfp para
      if (sscanf(optarg, "%f", &compress_param) < 0)
        terminate_with_error_msg("Invalid compression parameter\n%s", usage);
      break;

    case('m'): // is_fixed_file_size
      if (sscanf(optarg, "%d", &is_fixed_file_size) < 0 || is_fixed_file_size > 1)
        terminate_with_error_msg("Invalid aggregation mode\n%s", usage);
      break;

    case('d'): // is_log
      if (sscanf(optarg, "%d", &logs) < 0 || logs > 1)
        terminate_with_error_msg("Invalid logs parameter (0 or 1)\n%s", usage);
      break;

    case('a'): // to be delete
      if (sscanf(optarg, "%d", &agg_version) < 0)
        terminate_with_error_msg("Invalid aggregation parameter\n%s", usage);
      break;

    case('P'): // output file name
      if (sprintf(configstr, "%s", optarg) < 0)
        terminate_with_error_msg("Invalid caliper config string \n%s", usage);
      break;

      printf("%d: %s\n", rank, configstr);

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
	data = (unsigned char**)malloc(sizeof(*data) * variable_count);
	memset(data, 0, sizeof(*data) * variable_count);

	int bytes = (bpv[0]/8) * vps[0];
	int size = local_box_size[X] * local_box_size[Y] * local_box_size[Z] * bytes; // Local size

	data[0] = (unsigned char*)malloc(sizeof (*(data[0])) * size); // The first variable
    MPI_Datatype subarray = create_subarray(); // Self-define MPI data type
    MPI_File fh;
    MPI_Status status;
    int count = 0;

    MPI_File_open(MPI_COMM_WORLD, input_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_CHAR, subarray, "native", MPI_INFO_NULL);
    MPI_File_read(fh, data[0], size, MPI_CHAR, &status);
    MPI_Get_count(&status, MPI_CHAR, &count);
    if (count != size)
    {
    	printf("Read %d, need %d\n", count, size);
    	terminate_with_error_msg("ERROR: Read file failed!\n");
    }
    else
    {
    	if (rank == 0)
    		printf("The file %s has been read successful!\n", input_file);
    }

    MPI_File_close(&fh);

    // For other variables except first one, just copy the data of first variable.
	for (int var = 1; var < variable_count; var++)
	{
		data[var] = (unsigned char*)malloc(sizeof(*(data[var])) * size * (bpv[var]/8) * vps[var]);
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
  MPR_set_last_time_step(file, time_step_count);
  MPR_set_variable_count(file, variable_count);   /* Set the number of variables */
  MPR_set_io_mode(file, MPR_MUL_RES_PRE_IO);   /* Select I/O mode */
  MPR_set_out_file_num(file, out_file_num);
  MPR_set_compression_mode(file, compress_mode);
  MPR_set_compression_parameter(file, compress_param);
  MPR_set_aggregation_mode(file, is_fixed_file_size);
  MPR_set_logs(file, logs);

  file->mpr->agg_version = agg_version;

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
  data = (unsigned char**) malloc(sizeof(*data) * variable_count);
  memset(data, 0, sizeof(*data) * variable_count);

  // Synthetic simulation data
  for (var = 0; var < variable_count; var++)
  {
    uint64_t i, j, k, val_per_sample = 0;
    data[var] = (unsigned char*) malloc(sizeof (*(data[var])) * local_box_size[X] * local_box_size[Y] * local_box_size[Z] * (bpv[var]/8) * vps[var]);

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

