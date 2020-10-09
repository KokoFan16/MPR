/*
 * MPR_test_utils.h
 *
 *  Created on: Jul 3, 2020
 *      Author: kokofan
 */

#ifndef EXAMPLES_MPR_EXAMPLE_UTILS_H_
#define EXAMPLES_MPR_EXAMPLE_UTILS_H_

#include <MPR.h>

#define MAX_VAR_COUNT 256
enum { X, Y, Z, NUM_DIMS };

static int process_count = 1, rank = 0;
static int global_box_size[NUM_DIMS];
static int local_box_offset[NUM_DIMS];
static int local_box_size[NUM_DIMS];
static int patch_box_size[NUM_DIMS];
int sub_div[NUM_DIMS];
static int time_step_count = 1;
static int variable_count = 1;

static MPR_point global_size, local_offset, local_size;
static MPR_access p_access;
static MPR_file file;

static void init_mpi(int argc, char **argv);
static void check_args();
static void create_mpr_point_and_access();
static void terminate_with_error_msg(const char *format, ...);
static void calculate_per_process_offsets();
static int isNumber(char number[]);
static void shutdown_mpi();

/* MPI Environment Initialization */
static void init_mpi(int argc, char **argv)
{
	if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
		terminate_with_error_msg("ERROR: MPI_Init error\n");
	if (MPI_Comm_size(MPI_COMM_WORLD, &process_count) != MPI_SUCCESS)
	    terminate_with_error_msg("ERROR: MPI_Comm_size error\n");
	if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
	    terminate_with_error_msg("ERROR: MPI_Comm_rank error\n");
}

static void create_mpr_point_and_access()
{
  MPR_set_point(global_size, global_box_size[X], global_box_size[Y], global_box_size[Z]);
  MPR_set_point(local_offset, local_box_offset[X], local_box_offset[Y], local_box_offset[Z]);
  MPR_set_point(local_size, local_box_size[X], local_box_size[Y], local_box_size[Z]);
  //  Creating access
  MPR_create_access(&p_access);
  MPR_set_mpi_access(p_access, MPI_COMM_WORLD);
//  MPR_create_metadata_cache(&cache);
}

static void check_args()
{
	if (global_box_size[X] < local_box_size[X] || global_box_size[Y] < local_box_size[Y] || global_box_size[Z] < local_box_size[Z])
	  terminate_with_error_msg("ERROR: Global box is smaller than local box in one of the dimensions\n");

	// check if the number of processes given by the user is consistent with the actual number of processes needed
	int brick_count = (int)((global_box_size[X] + local_box_size[X] - 1) / local_box_size[X]) *
					(int)((global_box_size[Y] + local_box_size[Y] - 1) / local_box_size[Y]) *
					(int)((global_box_size[Z] + local_box_size[Z] - 1) / local_box_size[Z]);
	if (brick_count != process_count)
	  terminate_with_error_msg("ERROR: Number of sub-blocks (%d) doesn't match number of processes (%d)\n", brick_count, process_count);
}

/* Terminate the program */
static void terminate()
{
  MPI_Abort(MPI_COMM_WORLD, -1);
}

/* Show the error message when the program is terminated */
static void terminate_with_error_msg(const char *format, ...)
{
  va_list arg_ptr;
  va_start(arg_ptr, format);
  vfprintf(stderr, format, arg_ptr);
  va_end(arg_ptr);
  terminate();
}

static void calculate_per_process_offsets()
{
  sub_div[X] = (global_box_size[X] / local_box_size[X]);
  sub_div[Y] = (global_box_size[Y] / local_box_size[Y]);
  sub_div[Z] = (global_box_size[Z] / local_box_size[Z]);
  local_box_offset[Z] = (rank / (sub_div[X] * sub_div[Y])) * local_box_size[Z];
  int slice = rank % (sub_div[X] * sub_div[Y]);
  local_box_offset[Y] = (slice / sub_div[X]) * local_box_size[Y];
  local_box_offset[X] = (slice % sub_div[X]) * local_box_size[X];
}

static int isNumber(char number[])
{
    int i = 0;
    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++)
    {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}

static void shutdown_mpi()
{
  MPI_Finalize();
}

#endif /* EXAMPLES_MPR_EXAMPLE_UTILS_H_ */
