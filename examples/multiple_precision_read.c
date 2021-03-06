#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"

char input_file[512]; /* input file name */
int current_ts = 0;   /* the time step index to read */
int variable_index = 0;
int values_per_sample = 0;
int bits_per_sample = 0;
int global_offset[NUM_DIMS];
MPR_variable variable;

static void parse_args(int argc, char **argv);
static void set_mpr_file(int ts);
static void set_mpr_variable_and_create_buffer();


char *usage = "Parallel Usage: mpirun -n 8 ./multi_pre_read -g 16x16x16 -l 8x8x8 -s 0x0x0 -i input_file_name -t 0 -v 0\n"
					"  -g: the global box to read\n"
					"  -l: the local chunk per process to read\n"
					"  -s: the global offset to read\n"
					"  -i: input file name\n"
					"  -t: time step index to read\n"
					"  -v: variable index to read\n"
					"  -w: whether to write the data out\n";

int main(int argc, char **argv)
{
	init_mpi(argc, argv);  /* Init MPI and MPI vars (e.g. rank and process_count) */

	parse_args(argc, argv);  /* Parse input arguments and initialize */

	check_args();  	/* Check arguments */

	calculate_per_process_offsets();  /* Initialize per-process local domain */

	create_mpr_point_and_access(); 	/* Create MPI access and point */

	set_mpr_file(current_ts); 	/* Set file structure of current time step */

	set_mpr_variable_and_create_buffer();

	MPR_close(file);

	if (MPR_close_access(p_access) != MPR_success) /* close access */
		terminate_with_error_msg("MPR_close_access");

	/* MPI close */
	shutdown_mpi();
	return 0;
}

static void parse_args(int argc, char **argv)
{
	char flags[] = "g:l:s:i:t:v:w:";
	int one_opt = 0;

	while ((one_opt = getopt(argc, argv, flags)) != EOF)
	{
	    /* postpone error checking for after while loop */
	    switch (one_opt)
	    {
	    	case('g'): // global dimension
	    		if ((sscanf(optarg, "%dx%dx%d", &global_box_size[0], &global_box_size[1], &global_box_size[2]) == EOF) ||
	    		(global_box_size[0] < 1 || global_box_size[1] < 1 || global_box_size[2] < 1))
	    			terminate_with_error_msg("Invalid global dimensions\n%s", usage);
	    		break;

	    	case('l'): // local dimension
				if ((sscanf(optarg, "%dx%dx%d", &local_box_size[0], &local_box_size[1], &local_box_size[2]) == EOF) ||
				(local_box_size[0] < 1 || local_box_size[1] < 1 || local_box_size[2] < 1))
					terminate_with_error_msg("Invalid local dimension\n%s", usage);
	    		break;

	    	case('s'): // local dimension
				if ((sscanf(optarg, "%dx%dx%d", &global_offset[0], &global_offset[1], &global_offset[2]) == EOF) ||
				(global_offset[0] < 0 || global_offset[1] < 0 || global_offset[2] < 0))
					terminate_with_error_msg("Invalid local dimension\n%s", usage);
	    		break;

	    	case('i'): // input file name
				if (sprintf(input_file, "%s", optarg) < 0)
					terminate_with_error_msg("Invalid input file name\n%s", usage);
	    		break;

			case('t'): // number of timesteps
				if (sscanf(optarg, "%d", &current_ts) < 0)
					terminate_with_error_msg("Invalid variable file\n%s", usage);
				break;

			case('v'): // number of variables
				if (sscanf(optarg, "%d", &variable_index) < 0)
					terminate_with_error_msg("Invalid variable file\n%s", usage);
				break;

			case('w'): // whether to write the data out
				if (sscanf(optarg, "%d", &is_write) < 0 || is_write > 1)
					terminate_with_error_msg("Invalid write parameter (0, 1)\n%s", usage);
				break;


	    default:
	      terminate_with_error_msg("Wrong arguments\n%s", usage);
	    }
	}
}


static void set_mpr_file(int ts)
{
	if (MPR_file_open(input_file, MPR_MODE_RDONLY, p_access, global_size, local_size, local_offset, &file) != MPR_success)
		terminate_with_error_msg("MPR file open failed.\n");

	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		if (global_size[d] > file->mpr->origin_global_box[d] || (global_offset[d] + global_size[d]) > file->mpr->origin_global_box[d])
			terminate_with_error_msg("Read global box cannot exceed the original global box.\n");
	}

	if (ts < file->mpr->first_tstep || ts > file->mpr->last_tstep)
		terminate_with_error_msg("Invalid time-step.\n");

	MPR_set_current_time_step(file, ts);   /* Set the current timestep */

	MPR_set_global_offset(file, global_offset);

	MPR_set_is_write(file, is_write);
}

static void set_mpr_variable_and_create_buffer()
{
	if (variable_index >= file->mpr->variable_count)
		terminate_with_error_msg("Variable index more than variable count\n");

	if (MPR_set_current_variable_index(file, variable_index) != MPR_success)
		terminate_with_error_msg("MPR_set_current_variable_index\n");

	MPR_get_current_variable(file, &variable);

	MPR_values_per_datatype(variable->type_name, &values_per_sample, &bits_per_sample);
}
