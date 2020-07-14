/*
 * MPR_file_structs.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_FILE_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_FILE_STRUCTS_H_

#include "MPR_comm_structs.h"
#include "MPR_patch_structs.h"
#include "MPR_time_structs.h"
#include "MPR_variable_structs.h"
#include "MPR_dataset_structs.h"
#include "MPR_local_patch_structs.h"

struct mpr_file_descriptor
{

	int flags;							  /* File open and create mode */

	int fs_block_size;                    /* File system block size which is queried once at the beginning */

	MPR_comm comm;                        /* MPI communication related parameters */

	MPR_dataset mpr;                      /* Basic information */

	MPR_patch restructured_patch;         /* Patch related information */

	MPR_variable variable[MPR_MAX_VARIABLE_COUNT];  /* Array of variables */

	MPR_time time;                        /* For detailed time profiling of all phases */

	int local_variable_index;             /* starting index of variable that needs to be written out before a flush */
	int local_variable_count;             /* total number of variables that is written out in a flush */
	int variable_index_tracker;           /* tracking upto which variable io has been done (used for flushing) */

	int max_wavelet_level;                /* The maximum wavelet level based on the brick size */
};
typedef struct mpr_file_descriptor* MPR_file;

#endif /* SRC_DATA_STRUCTS_MPR_FILE_STRUCTS_H_ */
