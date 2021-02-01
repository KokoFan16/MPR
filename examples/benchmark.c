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
//char output_file_name[512];
//unsigned char **data;

static void parse_args(int argc, char **argv);
static int parse_var_list();
static int generate_vars();
static int read_size_file(char* input_file);
static int linear_interpolation();

char *usage = "Parallel Usage: mpirun -n 8 ./benchmark -g 8x8x8 -p 8x8x8 -i input_file -v 2 -t 4 -f output_file_name\n"
                     "  -g: patch count dimensions (patch count in x y z)\n"
					 "  -p: patch count dimensions in input file (patch count in x y z)\n"
					 "  -i: input file name\n"
                     "  -f: file name template\n"
                     "  -t: number of timesteps\n"
                     "  -v: number of variables (or file containing a list of variables)\n"
					 "  -d: whether to dump the logs\n (1: dump the logs)";


int main(int argc, char **argv)
{
	int ts = 0, var = 0;
	/* Init MPI and MPI vars (e.g. rank and process_count) */
	init_mpi(argc, argv);

	/* Parse input arguments and initialize */
	parse_args(argc, argv);


	read_size_file(input_file);

	linear_interpolation();


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
	// 2 2 2
	int file_patch_count = patch_box_size[0] * patch_box_size[1] * patch_box_size[2];
	// 4 2 2
	patch_count = global_box_size[0] * global_box_size[1] * global_box_size[2];

	if (patch_count == file_patch_count)
		patch_sizes = origin_patch_sizes;
	else
	{
		patch_sizes = malloc(patch_count * sizeof(int));

		float factor_x = patch_box_size[0] / (float) global_box_size[0]; // 0.5
		float factor_y = patch_box_size[1] / (float) global_box_size[1]; // 1
		float factor_z = patch_box_size[2] / (float) global_box_size[2]; // 1

		for (int k = 0; k < global_box_size[2]; k++) // 0 1
		{
			for (int j = 0; j < global_box_size[1]; j++) // 0 1
			{
				for (int i = 0; i < global_box_size[0]; i++) // 0 1 2 3
				{
					float z = k * factor_z; // 0 1
					float y = j * factor_y; // 0 1
					float x = i * factor_x; // 0 0.5 1 1.5

					int z_int = (int)floor(k * factor_z); // 0 1
					int y_int = (int)floor(j * factor_y); // 0 1
					int x_int = (int)floor(i * factor_x); // 0 0 1 1

					float w = z - z_int; // 0
					float u = y - y_int; // 0
					float v = x - x_int; // 0 0.5 0 0.5

					if (x_int + 1 == patch_box_size[0]) // 2
						x_int -= 1;  // 1
					if (y_int + 1 == patch_box_size[1]) // 2
						y_int -= 1;
					if (z_int + 1 == patch_box_size[2]) // 2
						z_int -= 1;

					int c000 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int];
					int c001 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int + 1];
					int c011 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + (y_int + 1) * patch_box_size[0] + x_int + 1];
					int c010 = origin_patch_sizes[z_int * patch_box_size[1] * patch_box_size[0] + (y_int + 1) * patch_box_size[0] + x_int];
					int c100 = origin_patch_sizes[(z_int + 1) * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int];
					int c101 = origin_patch_sizes[(z_int + 1) * patch_box_size[1] * patch_box_size[0] + y_int * patch_box_size[0] + x_int + 1];
					int c111 = origin_patch_sizes[(z_int + 1) * patch_box_size[1] * patch_box_size[0] + (y_int + 1) * patch_box_size[0] + x_int + 1];
					int c110 = origin_patch_sizes[(z_int + 1) * patch_box_size[1] * patch_box_size[0] + (y_int + 1) * patch_box_size[0] + x_int];

					int index = k * global_box_size[1] * global_box_size[0] + j * global_box_size[0] + i;
					patch_sizes[index] = (c000 * (1 - v) * (1 - u) * (1 - w)
							+ c100 * v * (1 - u) * (1 - w)
							+ c010 * (1- v) * u * (1 - w)
							+ c001 * (1 - v) * (1 - u) * w
							+ c101 * v * (1 - u) * w
							+ c011 * (1 - v) * u * w
							+ c110 * v * u * (1 - w)
							+ c111 * v * u * v);
				}
			}
		}

		if (rank == 0)
		{
			for (int i = 0; i < patch_count; i++)
			{
				printf("%d\n", patch_sizes[i]);
			}
		}

	}

	return MPR_success;
}

static void parse_args(int argc, char **argv)
{
  char flags[] = "g:p:i:f:t:v:d:";
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

