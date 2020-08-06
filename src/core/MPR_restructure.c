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
static int min_value_index(int* patch_num_array, int count);


MPR_return_code MPR_restructure_perform(MPR_file file, int start_var_index, int end_var_index)
{
	int rank = file->comm->simulation_rank; /* The rank of process */
	int procs_num = file->comm->simulation_nprocs; /* The number of processes */
	MPI_Comm comm = file->comm->simulation_comm; /* MPI Communicator */

	int global_box[MPR_MAX_DIMENSIONS]; /* The size of global dataset */
	int patch_box[MPR_MAX_DIMENSIONS]; /* The size of patch */
	memcpy(global_box, file->mpr->global_box, MPR_MAX_DIMENSIONS * sizeof(int)); /* Initialization */
	memcpy(patch_box, file->restructured_patch->patch_size, MPR_MAX_DIMENSIONS * sizeof(int)); /* Initialization */

	int patch_size = patch_box[0] * patch_box[1] * patch_box[2]; /* The size of regular patch */

	int bits = file->variable[start_var_index]->vps * file->variable[start_var_index]->bpv/8; /* bytes per data */

	int max_found_reg_patches = 1; /* The total number of patches for the global data */
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
		max_found_reg_patches *= ceil((float)file->mpr->global_box[d]/(float)file->restructured_patch->patch_size[d]);

	MPR_patch local_proc_patch = (MPR_patch)malloc(sizeof (*local_proc_patch)); /* Treat local dataset as a big patch */
	memset(local_proc_patch, 0, sizeof (*local_proc_patch)); /* Initial the local patch */
    for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
    {
      local_proc_patch->offset[d] = file->mpr->local_offset[d];
      local_proc_patch->size[d] = file->mpr->local_box[d];
    }

    int local_patch_num = max_found_reg_patches / procs_num; /* The local number of patches per process */
    int remain_patch_num = max_found_reg_patches % procs_num; /* Remainder */
    if (remain_patch_num != 0) /* Assign the extra patches while maintaining the load balance */
    {
    	int div = procs_num / remain_patch_num;
    	if (rank % div == 0  && rank / div < remain_patch_num)
    		local_patch_num += 1;
    }

    int required_local_patch_num[procs_num]; /* The required local number of patches per process */
    MPI_Allgather(&local_patch_num, 1, MPI_INT, required_local_patch_num, 1, MPI_INT, comm);

    /* Loop all the variables */
    for (int v = start_var_index; v < end_var_index; v++)
    {
    	MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
    	local_patch->patch_count = local_patch_num;

    	local_patch->patch = malloc(sizeof(MPR_patch*)*local_patch_num); /* Local patch array per variable */
    	/* Initialize all the patch pointer in local patch array and allocate the memory for patch buffer */
    	for (int i = 0; i < local_patch_num; i++)
    		local_patch->patch[i] = (MPR_patch)malloc(sizeof(*local_patch->patch[i]));

        int local_own_patch_num[procs_num]; /* the array of current number of patches per process */
        memset(local_own_patch_num, 0, procs_num*sizeof(int)); /* Initialization */

        int found_reg_patches_count = 0; /* The local found number of patches */
        int global_id = 0; /* The global id for each patch */

        MPR_patch* found_reg_patches = malloc(sizeof(MPR_patch*)*max_found_reg_patches); /* The found patches array for each process */
        memset(found_reg_patches, 0, sizeof(MPR_patch*)*max_found_reg_patches); /* Initialization */

        int local_patch_id = 0; /* The current owned number of patches per process*/

		/* Loop all the patches */
		for (int k = 0; k < global_box[2]; k += patch_box[2])
		{
			for (int j = 0; j < global_box[1]; j += patch_box[1])
			{
				for (int i = 0; i < global_box[0]; i += patch_box[0])
				{
					MPR_patch reg_patch = (MPR_patch)malloc(sizeof (*reg_patch)); /* Patch structure pointer */
					memset(reg_patch, 0, sizeof (*reg_patch)); /* Initialization */

					reg_patch->global_id = global_id; /* The global id for patch */
					reg_patch->is_boundary_patch = 0; /* 0 means the patch isn't the edge patch, otherwise, it is */

					/* Interior regular patches */
					reg_patch->offset[0] = i;
					reg_patch->offset[1] = j;
					reg_patch->offset[2] = k;
					reg_patch->size[0] = patch_box[0];
					reg_patch->size[1] = patch_box[1];
					reg_patch->size[2] = patch_box[2];

					/* Initialize the physical offset and size */
					memcpy(reg_patch->physical_offset, reg_patch->offset, MPR_MAX_DIMENSIONS * sizeof(int));
					memcpy(reg_patch->physical_size, reg_patch->size, MPR_MAX_DIMENSIONS * sizeof(int));

					int is_own = 0; /* If the local patch intersect with current patch */
					int own_ranks[procs_num]; /* the process array which intersect with current patch*/
					memset(own_ranks, 0, sizeof (*own_ranks)); /* Initial*/
					/* Check if this patch intersect with local patch for each process */
					if (intersect_patch(reg_patch, local_proc_patch))
					{   /* Check if this patch exists */
						if (!contains_patch(reg_patch, found_reg_patches, found_reg_patches_count))
						{
							int patch_end[MPR_MAX_DIMENSIONS] = {(reg_patch->offset[0] + reg_patch->size[0]), (reg_patch->offset[1] + reg_patch->size[1]), (reg_patch->offset[2] + reg_patch->size[2])};
							int local_end[MPR_MAX_DIMENSIONS] = {(local_proc_patch->offset[0] + local_proc_patch->size[0]), (local_proc_patch->offset[1] + local_proc_patch->size[1]), (local_proc_patch->offset[2] + local_proc_patch->size[2])};

							/* Calculate the real offset and real size */
							if (patch_end[0] > local_end[0] || patch_end[1] > local_end[1] || patch_end[2] > local_end[2])
							{
								if (patch_end[0] > local_end[0])
									reg_patch->physical_size[0] = local_end[0] - reg_patch->offset[0];
								if (patch_end[1] > local_end[1])
									reg_patch->physical_size[1] = local_end[1] - reg_patch->offset[1];
								if (patch_end[2] > local_end[2])
									reg_patch->physical_size[2] = local_end[2] - reg_patch->offset[2];
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

							/* Copy current patch to local found patches array */
							found_reg_patches[found_reg_patches_count] = (MPR_patch)malloc(sizeof (*reg_patch));
							memcpy(found_reg_patches[found_reg_patches_count], reg_patch, sizeof (*reg_patch));
							found_reg_patches_count++;
							is_own = 1; /* 1 means own this patch */
						}
					}

					/***** Patch Assignment Start *****/
					int owned_ranks = 0; /* The number of ranks that share one patch */
					MPI_Allreduce(&is_own, &owned_ranks, 1, MPI_INT, MPI_SUM, comm);
					MPI_Allgather(&is_own, 1, MPI_INT, own_ranks, 1, MPI_INT, comm);

					int flag = 0; /* If the patch has been assigned to any process */
					int a = 0;
					for (a = 0; a < procs_num; a++)
					{
						if (own_ranks[a] == 1 && local_own_patch_num[a] < required_local_patch_num[a])
						{
							local_own_patch_num[a] += 1; /* the number of patches of rank a add 1 */
							flag = 1; /* 1 means the current patch is assigned */
							break;
						}
					}
					if (flag == 0) /* If the current patch didn't be assigned to a process */
					{
						a = min_value_index(local_own_patch_num, procs_num); /* Found the minimum number of patches among process */
						local_own_patch_num[a] += 1; /* the number of patches of rank a add 1 */
					}
					/***** Patch Assignment End *****/

					/***** Patch Exchange and Merge Start *****/
					int share_physical_sizes[procs_num * MPR_MAX_DIMENSIONS];
					MPI_Allgather(reg_patch->physical_size, MPR_MAX_DIMENSIONS, MPI_INT, share_physical_sizes, MPR_MAX_DIMENSIONS, MPI_INT, comm);

					int patch_share_offset[MPR_MAX_DIMENSIONS] = {(reg_patch->physical_offset[0] - reg_patch->offset[0]),
							(reg_patch->physical_offset[1] - reg_patch->offset[1]), (reg_patch->physical_offset[2] - reg_patch->offset[2])};
					int patch_share_offsets[procs_num * MPR_MAX_DIMENSIONS];
					MPI_Allgather(patch_share_offset, MPR_MAX_DIMENSIONS, MPI_INT, patch_share_offsets, MPR_MAX_DIMENSIONS, MPI_INT, comm);

					/* Transform the patch to the process which owns it.
					 * Non-blocking Point to Point communication */
					int req_i = 0;
					MPI_Request req[owned_ranks];
					MPI_Status stat[owned_ranks];

					/* Receive data from other processes which share the patch */
					if (rank == a)
					{
						memcpy(local_patch->patch[local_patch_id], reg_patch, sizeof (*reg_patch));
			    		local_patch->patch[local_patch_id]->buffer = malloc(patch_size * bits);
			    		memset(local_patch->patch[local_patch_id]->buffer, 0, patch_size * bits);
						for (int r = 0; r < procs_num; r++)
						{
							if (own_ranks[r] == 1)
							{
								/* Creating patch receive data type */
								int array_size[MPR_MAX_DIMENSIONS] = {reg_patch->size[0]*bits, reg_patch->size[1], reg_patch->size[2]};
								int array_subsize[MPR_MAX_DIMENSIONS] = {share_physical_sizes[r*MPR_MAX_DIMENSIONS]*bits, share_physical_sizes[r*MPR_MAX_DIMENSIONS + 1], share_physical_sizes[r*MPR_MAX_DIMENSIONS + 2]};
								int subarray_offset[MPR_MAX_DIMENSIONS] = {patch_share_offsets[r*MPR_MAX_DIMENSIONS]*bits, patch_share_offsets[r*MPR_MAX_DIMENSIONS + 1], patch_share_offsets[r*MPR_MAX_DIMENSIONS + 2]};

								MPI_Datatype recv_type;
								MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, array_size, array_subsize, subarray_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &recv_type);
								MPI_Type_commit(&recv_type);

								/* MPI Recv function */
								MPI_Irecv(local_patch->patch[local_patch_id]->buffer, 1, recv_type, r, r, comm, &req[req_i]);
								req_i++;
								MPI_Type_free(&recv_type);
							}
						}
						/* Set the physical size and offset for edge patch */
						memcpy(local_patch->patch[local_patch_id]->physical_offset, reg_patch->offset, MPR_MAX_DIMENSIONS * sizeof(int));
						memcpy(local_patch->patch[local_patch_id]->physical_size, reg_patch->size, MPR_MAX_DIMENSIONS * sizeof(int));
						if ((i + patch_box[0]) > global_box[0])
						{
							local_patch->patch[local_patch_id]->is_boundary_patch = 1;
							local_patch->patch[local_patch_id]->physical_size[0] = global_box[0] - i;
						}
						if ((j + patch_box[1]) > global_box[1])
						{
							local_patch->patch[local_patch_id]->is_boundary_patch = 1;
							local_patch->patch[local_patch_id]->physical_size[1] = global_box[1] - j;
						}
						if ((k + patch_box[2]) > global_box[2])
						{
							local_patch->patch[local_patch_id]->is_boundary_patch = 1;
							local_patch->patch[local_patch_id]->physical_size[2] = global_box[2] - k;
						}

						local_patch_id++;
					}

					if (own_ranks[rank] == 1)
					{
						/* Create patch send data type */
						int array_size[MPR_MAX_DIMENSIONS] = {local_proc_patch->size[0]*bits, local_proc_patch->size[1], local_proc_patch->size[2]};
						int array_subsize[MPR_MAX_DIMENSIONS] = {reg_patch->physical_size[0]*bits, reg_patch->physical_size[1], reg_patch->physical_size[2]};
						int local_send_off[MPR_MAX_DIMENSIONS] = {(reg_patch->physical_offset[0] - local_proc_patch->offset[0])*bits,
									(reg_patch->physical_offset[1] - local_proc_patch->offset[1]), (reg_patch->physical_offset[2] - local_proc_patch->offset[2])};

						MPI_Datatype send_type;
						MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, array_size, array_subsize, local_send_off, MPI_ORDER_FORTRAN, MPI_BYTE, &send_type);
						MPI_Type_commit(&send_type);

						/* MPI Send function */
						MPI_Isend(local_patch->buffer, 1, send_type, a, rank, comm, &req[req_i]);
						req_i++;
						MPI_Type_free(&send_type);
					}
					MPI_Waitall(req_i, req, stat); /* Wait all the send and receive to be finished */
					/***** Patch Exchange and Merge End *****/

					global_id++;
					free(reg_patch);
				}
			}
		}

//		if (rank == 3 && v == 0)
//		{
//			for (int i = 0; i < local_patch_num; i++)
//			{
//				if (local_patch->patch[i]->global_id == 8)
//				{
//					for (int j = 0; j < patch_size; j++)
//					{
//						float a;
//						memcpy(&a, &local_patch->patch[i]->buffer[j*sizeof(float)], sizeof(float));
//						printf("%f\n", a);
//					}
//				}
////				printf("rank: %d: v: %d, id: %d, edge: %d, offset: %dx%dx%d, size: %dx%dx%d, phy_off: %dx%dx%d, phy_size: %dx%dx%d\n", rank, v, local_patch->patch[i]->global_id, local_patch->patch[i]->is_boundary_patch,
////						local_patch->patch[i]->offset[0], local_patch->patch[i]->offset[1], local_patch->patch[i]->offset[2],
////						local_patch->patch[i]->size[0], local_patch->patch[i]->size[1], local_patch->patch[i]->size[2],
////						local_patch->patch[i]->physical_offset[0], local_patch->patch[i]->physical_offset[1], local_patch->patch[i]->physical_offset[2],
////						local_patch->patch[i]->physical_size[0], local_patch->patch[i]->physical_size[1], local_patch->patch[i]->physical_size[2]);
//			}
//
//		}


	    /* Clean up */
	    for (int i = 0; i < found_reg_patches_count; i++)
	    {
	      free(found_reg_patches[i]);
	      found_reg_patches[i] = 0;
	    }
	    free(found_reg_patches);
    }

    free(local_proc_patch);
	return MPR_success;
}

/* Find minimum value in a array (param: array and size)*/
static int min_value_index(int* patch_num_array, int count)
{
	int id = -1;
	int min = INT_MAX;
	for (int i = 0; i < count; i++)
	{
		if (patch_num_array[i] < min)
		{
			min = patch_num_array[i];
			id = i;
		}
	}
	return id;
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
