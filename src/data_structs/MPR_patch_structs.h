/*
 * MPR_brick_structs.h
 *
 *  Created on: Jul 3, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_PATCH_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_PATCH_STRUCTS_H_

struct mpr_patch_struct
{
	int global_id;                                  /* global id for patch */

	unsigned char* buffer;                          /* data buffer of a patch */

	int patch_buffer_size;                          /* the size of the buffer */


	int offset[MPR_MAX_DIMENSIONS];                 /* offset of patch in the 3D global space */
	int size[MPR_MAX_DIMENSIONS];                   /* size (extents) in each of the dimensions for patch */

	int* subbands_comp_size;                        /* the compressed size of each sub-band */
};
typedef struct mpr_patch_struct* MPR_patch;

#endif /* SRC_DATA_STRUCTS_MPR_PATCH_STRUCTS_H_ */
