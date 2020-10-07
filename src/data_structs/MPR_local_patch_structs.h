/*
 * MPR_local_patch_structs.h
 *
 *  Created on: Jul 11, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_LOCAL_PATCH_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_LOCAL_PATCH_STRUCTS_H_

struct mpr_local_patch_struct
{
	int patch_count;                         /* Number of patches in the local patch */

	int bounding_box[MPR_MAX_DIMENSIONS*2];  /* The bounding box for each out file */

	unsigned char* buffer;                   /* data buffer */

	unsigned long long out_file_size;        /* the size out file */

	MPR_patch *patch;                        /* Pointer to the patches that are included in the local dataset */

	int agg_patch_count;                     /* Number of patches in the local patch after aggregation */
	int* patch_id_array;                     /* A array for patches id */
	unsigned long long* agg_patch_disps;     /* The displacement in the buffer of each patch after aggregation */
};
typedef struct mpr_local_patch_struct* MPR_local_patch;

#endif /* SRC_DATA_STRUCTS_MPR_LOCAL_PATCH_STRUCTS_H_ */
