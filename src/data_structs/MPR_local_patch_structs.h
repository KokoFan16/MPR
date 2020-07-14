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
	int patch_count;          /* Number of patches in the local patch */

	unsigned char* buffer;    /* data buffer */

	MPR_patch *patch;         /* Pointer to the patches that are included in the local dataset */
};
typedef struct mpr_local_patch_struct* MPR_local_patch;

#endif /* SRC_DATA_STRUCTS_MPR_LOCAL_PATCH_STRUCTS_H_ */
