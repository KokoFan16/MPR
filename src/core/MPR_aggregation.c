/*
 * MPR_aggregation.c
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_aggregation(MPR_file file, int svi, int evi)
{
	int mode = file->mpr->aggregation_mode;  /* Aggregation Mode: 0 (fixed-size) 1 (fixed-patch-number) */
	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */

	int proc_num = file->comm->simulation_nprocs;  /* The number of processes */
	int rank = file->comm->simulation_rank; /* The rank of each process */

	if (mode == 0) /* fixed-size mode */
	{
		printf("mode: fixed-size\n");
	}
	else if (mode == 1)  /* fixed-patch-number mode */
	{
//	    printf("rank %d: test\n", file->comm->simulation_rank);
//		printf("rank %d: %d\n", file->comm->simulation_rank, file->mpr->total_patches_num);
	}
	else
	{
		return MPR_err_unsupported_flags;
	}
	return MPR_success;
}
