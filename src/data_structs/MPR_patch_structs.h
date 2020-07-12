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

	int patch_size[MPR_MAX_DIMENSIONS];             /* restructured grid size */
	int is_boundary_patch;                          /* 1 if the patch is at the boundary (non-pwer two dataset) 0 otherwise */

	int max_wavelet_level;                          /* The maximum wavelet level based on the brick size */

	int offset[MPR_MAX_DIMENSIONS];                 /* offset of patch in the 3D global space */
	int size[MPR_MAX_DIMENSIONS];                   /* size (extents) in each of the dimensions for patch */

	int physical_offset[MPR_MAX_DIMENSIONS];        /* the physical offset of a patch in 3D local space */
	int physical_size[MPR_MAX_DIMENSIONS];          /* the physical size in each dimensions for patch */

	int wavelet_level;                              /* The WAVELET level of the patch */
};
typedef struct mpr_patch_struct* MPR_patch;

#endif /* SRC_DATA_STRUCTS_MPR_PATCH_STRUCTS_H_ */
