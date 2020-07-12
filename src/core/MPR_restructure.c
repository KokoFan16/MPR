/*
 * MPR_restructure.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"
#include <math.h>

static int intersect_patch(MPR_patch A, MPR_patch B);
static int contains_patch(MPR_patch reg_patch, MPR_patch* patches, int count);


MPR_return_code MPR_restructure_setup(MPR_file file, int start_var_index, int end_var_index)
{
	int global_box[MPR_MAX_DIMENSIONS]; /* Global size */
	int patch_size[MPR_MAX_DIMENSIONS]; /* Patch size */
	memcpy(global_box, file->mpr->global_box, MPR_MAX_DIMENSIONS * sizeof(int));
	memcpy(patch_size, file->restructured_patch->patch_size, MPR_MAX_DIMENSIONS * sizeof(int));

	uint64_t max_found_reg_patches = 1; /* Maximum number of regular patches */
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
		max_found_reg_patches *= ceil((float)global_box[d]/(float)patch_size[d]);
	/* The found regular patches per process */
	MPR_patch* found_reg_patches = malloc(sizeof(MPR_patch*)*max_found_reg_patches);
	memset(found_reg_patches, 0, sizeof(MPR_patch*)*max_found_reg_patches);
	/* Convert the local dataset to a patch */
	MPR_patch local_proc_patch = (MPR_patch)malloc(sizeof (*local_proc_patch));
	memset(local_proc_patch, 0, sizeof (*local_proc_patch));
    for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
    {
      local_proc_patch->offset[d] = file->mpr->local_offset[d];  /* Local offset */
      local_proc_patch->size[d] = file->mpr->local_box[d];   /* Local size */
    }

    printf("%d: offset %dx%dx%d\n", file->comm->simulation_rank, file->mpr->local_offset[0], file->mpr->local_offset[1], file->mpr->local_offset[2]);

    int found_reg_patches_count = 0; /* The number of regular patches per process */
    int count = 0;
	for (int i = 0; i < global_box[0]; i += patch_size[0])
	{
		for (int j = 0; j < global_box[1]; j += patch_size[1])
		{
			for (int k = 0; k < global_box[2]; k += patch_size[2])
			{
				MPR_patch reg_patch = (MPR_patch)malloc(sizeof (*reg_patch));
				memset(reg_patch, 0, sizeof (*reg_patch));

				/* Interior regular patches (offset and size) */
				reg_patch->offset[0] = i;
				reg_patch->offset[1] = j;
				reg_patch->offset[2] = k;
				reg_patch->size[0] = patch_size[0];
				reg_patch->size[1] = patch_size[1];
				reg_patch->size[2] = patch_size[2];

				/* Edge regular patches (The size is probably smaller than regular patch) */
				if ((i + patch_size[0]) > global_box[0])
					reg_patch->size[0] = global_box[0] - i;
				if ((j + patch_size[1]) > global_box[1])
					reg_patch->size[1] = global_box[1] - j;
				if ((k + patch_size[2]) > global_box[2])
					reg_patch->size[2] = global_box[2] - k;

				reg_patch->global_id = count;
				memcpy(reg_patch->physical_offset, reg_patch->offset, MPR_MAX_DIMENSIONS * sizeof(int));
				memcpy(reg_patch->physical_size, reg_patch->size, MPR_MAX_DIMENSIONS * sizeof(int));

				/* Check if the current patch intersects with local patch (local dataset) */
				if (intersect_patch(reg_patch, local_proc_patch))
				{
					int patch_end[MPR_MAX_DIMENSIONS] = {(reg_patch->offset[0] + reg_patch->size[0]), (reg_patch->offset[1] + reg_patch->size[1]), (reg_patch->offset[2] + reg_patch->size[2])};
					int pro_end[MPR_MAX_DIMENSIONS] = {(local_proc_patch->offset[0] + local_proc_patch->size[0]), (local_proc_patch->offset[1] + local_proc_patch->size[1]), (local_proc_patch->offset[2] + local_proc_patch->size[2])};

					if (patch_end[0] > pro_end[0] || patch_end[1] > pro_end[1] || patch_end[2] > pro_end[2])
					{
						if (patch_end[0] > pro_end[0])
							reg_patch->physical_size[0] = pro_end[0] - reg_patch->offset[0];
						if (patch_end[1] > pro_end[1])
							reg_patch->physical_size[1] = pro_end[1] - reg_patch->offset[1];
						if (patch_end[2] > pro_end[2])
							reg_patch->physical_size[2] = pro_end[2] - reg_patch->offset[2];
					}

					if (reg_patch->offset[0] < local_proc_patch->offset[0] || reg_patch->offset[1] < local_proc_patch->offset[1] || reg_patch->offset[2] < local_proc_patch->offset[2])
					{
						if (reg_patch->offset[0] < local_proc_patch->offset[0])
						{
							reg_patch->physical_offset[0] = local_proc_patch->offset[0];
							reg_patch->physical_size[0] = patch_end[0] - local_proc_patch->offset[0];
						}
						if (reg_patch->offset[1] < local_proc_patch->offset[1])
						{
							reg_patch->physical_offset[1] = local_proc_patch->offset[1];
							reg_patch->physical_size[1] = patch_end[1] - local_proc_patch->offset[1];
						}
						if (reg_patch->offset[2] < local_proc_patch->offset[2])
						{
							reg_patch->physical_offset[2] = local_proc_patch->offset[2];
							reg_patch->physical_size[2] = patch_end[2] - local_proc_patch->offset[2];
						}
					}

					/* Check if the current patch has already been included */
					if (!contains_patch(reg_patch, found_reg_patches, found_reg_patches_count))
					{
						found_reg_patches[found_reg_patches_count] = (MPR_patch)malloc(sizeof (*reg_patch));
						memcpy(found_reg_patches[found_reg_patches_count], reg_patch, sizeof (*reg_patch));
						found_reg_patches_count++;
					}
				}
				free(reg_patch);
				count++;
			}
		}
	}
	file->mpr->local_patch_count = found_reg_patches_count;
	free(local_proc_patch);

//	for (int i = 0; i < found_reg_patches_count; i++)
//	{
//		printf("%d: local %d: global: %d, %dx%dx%d, %dx%dx%d, physical: %dx%dx%d, %dx%dx%d\n", file->comm->simulation_rank, i, found_reg_patches[i]->global_id,
//				found_reg_patches[i]->offset[0], found_reg_patches[i]->offset[1], found_reg_patches[i]->offset[2],
//				found_reg_patches[i]->size[0], found_reg_patches[i]->size[1], found_reg_patches[i]->size[2],
//				found_reg_patches[i]->physical_offset[0], found_reg_patches[i]->physical_offset[1], found_reg_patches[i]->physical_offset[2],
//				found_reg_patches[i]->physical_size[0], found_reg_patches[i]->physical_size[1], found_reg_patches[i]->physical_size[2]);
//	}

	for (int i = 0; i < found_reg_patches_count; i++)
	{
		free(found_reg_patches[i]);
		found_reg_patches[i] = 0;
	}
	free(found_reg_patches);

	return MPR_success;
}

/* Function to check if patch A and B intersects */
static int intersect_patch(MPR_patch A, MPR_patch B)
{
  int d = 0, check_bit = 0;
  for (d = 0; d < MPR_MAX_DIMENSIONS; d++)
    check_bit = check_bit || (A->offset[d] + A->size[d] - 1) < B->offset[d] || (B->offset[d] + B->size[d] - 1) < A->offset[d];

  return !(check_bit);
}

/* Check if the current patch has already been included */
static int contains_patch(MPR_patch reg_patch, MPR_patch* patches, int count)
{
  for (int i = 0; i < count; i++)
  {
    int matches = 0;
    for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
    {
      if (reg_patch->offset[d] == patches[i]->offset[d] && reg_patch->size[d] == patches[i]->size[d])
        matches++;
    }

    if (matches == MPR_MAX_DIMENSIONS)
      return 1;
  }
  return 0;
}
