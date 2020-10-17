/*
 * MPR_dataset_structs.h
 *
 *  Created on: Jul 4, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_DATASET_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_DATASET_STRUCTS_H_

struct mpr_dataset_struct
{
	enum MPR_io_type io_type;                         /* I/O format and layout we want to use */

	int current_time_step;                            /* The current timestep selected */
	int first_tstep;                                  /* Index of the frist timestep */
	int last_tstep;                                   /* Index of the last timestep */

	int variable_count;                               /* The number of variables contained in the dataset */

	int total_patches_num;                            /* The total number of patches */

	int proc_num_per_node;                            /* The number of processes per node */
	int node_num;                                     /* The number of nodes */
	int proc_num_last_node;                           /* The number of processes of last node */

	char filename[MPR_FILE_PATH_LENGTH];              /* The .mpr file path */
	char filename_template[MPR_FILE_PATH_LENGTH];     /* Filename template use to resolve the path of the .mpr file and binaries */
	char filename_time_template[MPR_FILE_PATH_LENGTH];/* Filename template used to resolve the path of time folders */

	int global_box[MPR_MAX_DIMENSIONS];               /* Current global box to write/read */
	int local_box[MPR_MAX_DIMENSIONS];                /* Local box of dataset */
	int local_offset[MPR_MAX_DIMENSIONS];             /* Local offset of local dataset */
	int patch_box[MPR_MAX_DIMENSIONS];                /* Patch box */

	int origin_global_box[MPR_MAX_DIMENSIONS];        /* (only for read) The global box of the dataset */
	int global_offset[MPR_MAX_DIMENSIONS];            /* (only for read) The start offset to read */

	int compression_type;                             /* Compression Mode */
	float compression_param;                          /* Compression parameter */

	int out_file_num;                                 /* The number of out file */
	unsigned long long file_size;                     /* The total size of each out file */

	int is_aggregator;                                /* If the rank is a aggregator */

	int wavelet_trans_num;                            /* The maximum wavelet level based on the brick size */
};
typedef struct mpr_dataset_struct* MPR_dataset;

#endif /* SRC_DATA_STRUCTS_MPR_DATASET_STRUCTS_H_ */
