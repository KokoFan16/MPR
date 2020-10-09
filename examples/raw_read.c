/*
 * raw_read.c
 *
 *  Created on: Oct 8, 2020
 *      Author: kokofan
 */


#include "../src/utils/MPR_windows_utils.h"
#include "MPR_example_utils.h"

char input_file[512]; /* input file name */
int current_ts = 0;   /* the time step index to read */
int variable_index = 0;
int start_index[NUM_DIMS];

static void parse_args(int argc, char **argv);
static void set_pidx_file(int ts);


char *usage = "Serial Usage: ./raw_read -g 16x16x16 -l 16x16x16 -s 0x0x0 -i input_file_name -t 0 -v 0\n"
                    "Parallel Usage: mpirun -n 8 ./raw_read -g 16x16x16 -l 8x8x8 -s 0x0x0 -i input_file_name -t 0 -v 0\n"
					"  -g: the global box to read\n"
					"  -l: the local chunk per process to read\n"
					"  -s: the start index of global box to read\n"
					"  -i: input file name\n"
					"  -t: time step index to read\n"
					"  -v: variable index to read\n";

int main(int argc, char **argv)
{
	init_mpi(argc, argv);  /* Init MPI and MPI vars (e.g. rank and process_count) */

	parse_args(argc, argv);  /* Parse input arguments and initialize */

	/* Initialize per-process local domain */
	calculate_per_process_offsets();

	create_mpr_point_and_access();

//	MPR_create_access(&p_access); /* create MPI access */
//	MPR_set_mpi_access(p_access, MPI_COMM_WORLD);

	set_pidx_file(current_ts);

	if (MPR_close_access(p_access) != MPR_success) /* close access */
		terminate_with_error_msg("MPR_close_access");

	/* MPI close */
	shutdown_mpi();
	return 0;
}

static void parse_args(int argc, char **argv)
{
	char flags[] = "g:l:s:i:t:v:";
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
				if ((sscanf(optarg, "%dx%dx%d", &start_index[0], &start_index[1], &start_index[2]) == EOF) ||
				(start_index[0] < 0 || start_index[1] < 0 || start_index[2] < 0))
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

	    default:
	      terminate_with_error_msg("Wrong arguments\n%s", usage);
	    }
	}
}

static void set_pidx_file(int ts)
{
	MPR_file_open(input_file, p_access, &file);

	MPR_set_current_time_step(file, ts);   /* Set the current timestep */
}
