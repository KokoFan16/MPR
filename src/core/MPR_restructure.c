/*
 * MPR_restructure.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

//static int intersect_patch(MPR_patch A, MPR_patch B);
static int intersect_patch(int* a_size, int* a_offset, int* b_size, int* b_offset);
//static int contains_patch(MPR_patch reg_patch, MPR_patch* patches, int count);

/* Set the default restructuring box (32x32x32) */
MPR_return_code MPR_set_patch_box_size(MPR_file file, int svi)
{
	if ((file->mpr->patch_box[0] == -1 || file->mpr->patch_box[1] == -1 || file->mpr->patch_box[2] == -1)
		|| (file->mpr->patch_box[0] == 0 &&  file->mpr->patch_box[1] == 0 && file->mpr->patch_box[2] == 0))
	{
		fprintf(stderr,"Warning: patch box is not set, using default 32x32x32 size. [File %s Line %d]\n", __FILE__, __LINE__);
		file->mpr->patch_box[0]=32;file->mpr->patch_box[1]=32;file->mpr->patch_box[2]=32;
	}

	return MPR_success;
}

MPR_return_code MPR_processing(MPR_file file, int svi, int evi)
{
	int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2];
	file->mpr->total_patches_num = file->comm->simulation_nprocs;

	int node_num = ceil((float)file->comm->simulation_nprocs / file->mpr->proc_num_per_node); /* The number of nodes based on the number of processes per node */
	file->mpr->node_num = node_num;

	int proc_reminder = file->comm->simulation_nprocs % file->mpr->proc_num_per_node;
	int proc_num_last_node = (proc_reminder == 0)? file->mpr->proc_num_per_node: proc_reminder; /* the number of processes of last node */
	file->mpr->proc_num_last_node = proc_num_last_node;

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		local_patch->patch = malloc(sizeof(MPR_patch*));
		local_patch->patch_count = 1;
		local_patch->patch[0] = (MPR_patch)malloc(sizeof(*local_patch->patch[0]));
		local_patch->patch[0]->global_id = file->comm->simulation_rank;

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		local_patch->patch[0]->buffer = malloc(patch_size * bytes);
		memcpy(local_patch->patch[0]->buffer, local_patch->buffer, patch_size*bytes);
	}
	return MPR_success;
}

MPR_return_code MPR_restructure_perform(MPR_file file, int start_var_index, int end_var_index)
{
	/************************** Basic information *******************************/
	int rank = file->comm->simulation_rank; /* The rank of process */
	int procs_num = file->comm->simulation_nprocs; /* The number of processes */
	MPI_Comm comm = file->comm->simulation_comm; /* MPI Communicator */

	int global_box[MPR_MAX_DIMENSIONS]; /* The size of global dataset */
	int patch_box[MPR_MAX_DIMENSIONS]; /* The size of patch */
	memcpy(global_box, file->mpr->global_box, MPR_MAX_DIMENSIONS * sizeof(int)); /* Initialization */
	memcpy(patch_box, file->mpr->patch_box, MPR_MAX_DIMENSIONS * sizeof(int)); /* Initialization */

	int patch_size = patch_box[0] * patch_box[1] * patch_box[2]; /* The size of regular patch */
	/***************************************************************************/

	/******************** Calculate total number of patches *********************/
	int max_found_reg_patches = 1; /* The total number of patches for the global data */
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
		max_found_reg_patches *= ceil((float)global_box[d]/patch_box[d]);
	file->mpr->total_patches_num = max_found_reg_patches; /* The global total number of patches */
	/***************************************************************************/

	/************************ Gather local patch info **************************/
	int local_patch_offset_array[procs_num*MPR_MAX_DIMENSIONS];
	int local_patch_size_array[procs_num*MPR_MAX_DIMENSIONS];
	MPI_Allgather(file->mpr->local_offset, MPR_MAX_DIMENSIONS, MPI_INT, local_patch_offset_array, MPR_MAX_DIMENSIONS, MPI_INT, comm);
	MPI_Allgather(file->mpr->local_box, MPR_MAX_DIMENSIONS, MPI_INT, local_patch_size_array, MPR_MAX_DIMENSIONS, MPI_INT, comm);
	/***************************************************************************/

	/******************** Calculate local number of patches *********************/
    int local_patch_num = max_found_reg_patches / procs_num; /* The local number of patches per process */
    int remain_patch_num = max_found_reg_patches % procs_num; /* Remainder */
    int node_num = ceil((float)procs_num / file->mpr->proc_num_per_node); /* The number of nodes based on the number of processes per node */
    file->mpr->node_num = node_num;

    /* If all the processes belong to one node */
    if (node_num == 1)
    	local_patch_num = (rank < remain_patch_num)? (local_patch_num + 1): local_patch_num;
    else  /* If they belong to multiple nodes */
    {
		int avg_rem_patch_num_per_node = remain_patch_num / node_num; /* The average number of extra patches per node */

		int proc_reminder = procs_num % file->mpr->proc_num_per_node;
		int proc_num_last_node = (proc_reminder == 0)? file->mpr->proc_num_per_node: proc_reminder; /* the number of processes of last node */
		file->mpr->proc_num_last_node = proc_num_last_node;

		int rem_patch_assign_array[node_num]; /* extra patches assignment array for nodes */
		/* calculate the number of extra patches assigned for the last node first */
		rem_patch_assign_array[node_num-1] = (proc_num_last_node < avg_rem_patch_num_per_node)? proc_num_last_node: avg_rem_patch_num_per_node;
		remain_patch_num -= rem_patch_assign_array[node_num-1];
		avg_rem_patch_num_per_node = remain_patch_num / (node_num - 1); /* minus the number of last node */
		/* calculate the number extra patches assigned for others */
		for (int i = 0; i < node_num - 1; i++)
			rem_patch_assign_array[i] = (i < (remain_patch_num % (node_num - 1)))? (avg_rem_patch_num_per_node + 1): avg_rem_patch_num_per_node;
		/* calculate the number of patches for each process */
		int id = rank / file->mpr->proc_num_per_node;
		if (rank % file->mpr->proc_num_per_node < rem_patch_assign_array[id])
			local_patch_num += 1;
    }
    int required_local_patch_num[procs_num]; /* The required local number of patches per process */
    MPI_Allgather(&local_patch_num, 1, MPI_INT, required_local_patch_num, 1, MPI_INT, comm);
    /***************************************************************************/

	int found_reg_patches_count = 0; /* The local found number of patches */
	MPR_patch* found_reg_patches = malloc(sizeof(MPR_patch*)*max_found_reg_patches); /* The found patches array for each process */
	memset(found_reg_patches, 0, sizeof(MPR_patch*)*max_found_reg_patches); /* Initialization */

	MPR_local_patch local_patch_v0 = file->variable[0]->local_patch; /* Local patch pointer */
	local_patch_v0->patch = malloc(sizeof(MPR_patch*)*local_patch_num); /* Local patch array per variable */
	/* Initialize all the patch pointer in local patch array and allocate the memory for patch buffer */
	for (int i = 0; i < local_patch_num; i++)
		local_patch_v0->patch[i] = (MPR_patch)malloc(sizeof(*local_patch_v0->patch[i]));

    /***************************** Patch assignment *******************************/
	int local_own_patch_num[procs_num]; /* the array of current number of patches per process */
	memset(local_own_patch_num, 0, procs_num*sizeof(int)); /* Initialization */

	int patch_shared_ranks[local_patch_num][procs_num];
	int share_physical_sizes[local_patch_num][procs_num * MPR_MAX_DIMENSIONS];
	int patch_share_offsets[local_patch_num][procs_num * MPR_MAX_DIMENSIONS];

	int local_patch_id = 0;
	int global_id = 0; /* The global id for each patch */
	for (int k = 0; k < global_box[2]; k += patch_box[2])
	{
		for (int j = 0; j < global_box[1]; j += patch_box[1])
		{
			for (int i = 0; i < global_box[0]; i += patch_box[0])
			{
				MPR_patch reg_patch = (MPR_patch)malloc(sizeof (*reg_patch)); /* Patch structure pointer */
				memset(reg_patch, 0, sizeof (*reg_patch)); /* Initialization */

				reg_patch->global_id = global_id; /* The global id for patch */

				/* Interior regular patches */
				reg_patch->offset[0] = i;
				reg_patch->offset[1] = j;
				reg_patch->offset[2] = k;
				reg_patch->size[0] = patch_box[0];
				reg_patch->size[1] = patch_box[1];
				reg_patch->size[2] = patch_box[2];

				/* Find all the processes that intersect with this patch */
				int physical_sizes[procs_num*MPR_MAX_DIMENSIONS];
				int physical_offsets[procs_num*MPR_MAX_DIMENSIONS];
				int patch_share_offset[procs_num*MPR_MAX_DIMENSIONS];

				int own_ranks[procs_num];
				memset(own_ranks, 0, procs_num*sizeof(int));
				for (int r = 0; r < procs_num; r++)
				{
					memcpy(&physical_sizes[r*MPR_MAX_DIMENSIONS], reg_patch->size, MPR_MAX_DIMENSIONS * sizeof(int));
					memcpy(&physical_offsets[r*MPR_MAX_DIMENSIONS], reg_patch->offset, MPR_MAX_DIMENSIONS * sizeof(int));

					int local_patch_offset[MPR_MAX_DIMENSIONS];
					int local_patch_size[MPR_MAX_DIMENSIONS];
					memcpy(local_patch_offset, &local_patch_offset_array[r*MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));
					memcpy(local_patch_size, &local_patch_size_array[r*MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));

					if (intersect_patch(reg_patch->size, reg_patch->offset, local_patch_size, local_patch_offset))
					{
						own_ranks[r] = 1;
						int patch_end[MPR_MAX_DIMENSIONS] = {(reg_patch->offset[0] + reg_patch->size[0]), (reg_patch->offset[1] + reg_patch->size[1]), (reg_patch->offset[2] + reg_patch->size[2])};
						int local_end[MPR_MAX_DIMENSIONS] = {(local_patch_offset[0] + local_patch_size[0]), (local_patch_offset[1] + local_patch_size[1]), (local_patch_offset[2] + local_patch_size[2])};

						/* Calculate the physical size */
						if (patch_end[0] > local_end[0] || patch_end[1] > local_end[1] || patch_end[2] > local_end[2])
						{
							if (patch_end[0] > local_end[0])
								physical_sizes[r*MPR_MAX_DIMENSIONS] = local_end[0] - reg_patch->offset[0];
							if (patch_end[1] > local_end[1])
								physical_sizes[r*MPR_MAX_DIMENSIONS + 1] = local_end[1] - reg_patch->offset[1];
							if (patch_end[2] > local_end[2])
								physical_sizes[r*MPR_MAX_DIMENSIONS + 2] = local_end[2] - reg_patch->offset[2];
						}

						/* Calculate the physical offset */
						if (reg_patch->offset[0] < local_patch_offset[0] || reg_patch->offset[1] < local_patch_offset[1] || reg_patch->offset[2] < local_patch_offset[2])
						{
							if (reg_patch->offset[0] <local_patch_offset[0])
							{
								physical_offsets[r*MPR_MAX_DIMENSIONS] = local_patch_offset[0];
								physical_sizes[r*MPR_MAX_DIMENSIONS] = patch_end[0] - local_patch_offset[0];
							}
							if (reg_patch->offset[1] < local_patch_offset[1])
							{
								physical_offsets[r*MPR_MAX_DIMENSIONS + 1] = local_patch_offset[1];
								physical_sizes[r*MPR_MAX_DIMENSIONS + 1] = patch_end[1] - local_patch_offset[1];
							}
							if (reg_patch->offset[2] < local_patch_offset[2])
							{
								physical_offsets[r*MPR_MAX_DIMENSIONS + 2] = local_patch_offset[2];
								physical_sizes[r*MPR_MAX_DIMENSIONS + 2] = patch_end[2] - local_patch_offset[2];
							}
						}
					}
					patch_share_offset[r*MPR_MAX_DIMENSIONS] = physical_offsets[r*MPR_MAX_DIMENSIONS] - reg_patch->offset[0];
					patch_share_offset[r*MPR_MAX_DIMENSIONS + 1] = physical_offsets[r*MPR_MAX_DIMENSIONS + 1] - reg_patch->offset[1];
					patch_share_offset[r*MPR_MAX_DIMENSIONS + 2] = physical_offsets[r*MPR_MAX_DIMENSIONS + 2] - reg_patch->offset[2];
				}
				memcpy(reg_patch->physical_offset, &physical_offsets[rank*MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));
				memcpy(reg_patch->physical_size, &physical_sizes[rank*MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));

				/************************** Patch Assignment ******************************/
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
					for (a = 0; a < procs_num; a++)
					{
						if (local_own_patch_num[a] < required_local_patch_num[a])
						{
							local_own_patch_num[a] += 1;
							break;
						}
					}
				}
				/*********************************************************************************/

				reg_patch->owned_rank = a;
				if (own_ranks[rank] == 1)
				{
					/* Copy current patch to local found patches array */
					found_reg_patches[found_reg_patches_count] = (MPR_patch)malloc(sizeof (*reg_patch));
					memcpy(found_reg_patches[found_reg_patches_count], reg_patch, sizeof (*reg_patch));
					found_reg_patches_count++;
				}
				if (rank == a)
				{
					memcpy(local_patch_v0->patch[local_patch_id], reg_patch, sizeof (*reg_patch));
					memcpy(patch_shared_ranks[local_patch_id], own_ranks, procs_num*sizeof(int));
					memcpy(share_physical_sizes[local_patch_id], physical_sizes, procs_num*MPR_MAX_DIMENSIONS*sizeof(int));
					memcpy(patch_share_offsets[local_patch_id], patch_share_offset, procs_num*MPR_MAX_DIMENSIONS*sizeof(int));
					local_patch_id++;
				}
				free(reg_patch);
				global_id++;
			}
		}
	}

	/*********************************** Data exchange and merge *********************************/
	for (int v = start_var_index; v < end_var_index; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		local_patch->patch_count = local_patch_num;

		int bits = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		if (v != 0)
		{
			local_patch->patch = malloc(sizeof(MPR_patch*)*local_patch_num); /* Local patch array per variable */
			/* Initialize all the patch pointer in local patch array and allocate the memory for patch buffer */
			for (int i = 0; i < local_patch_num; i++)
			{
				local_patch->patch[i] = (MPR_patch)malloc(sizeof(*local_patch->patch[i]));
				memcpy(local_patch->patch[i], local_patch_v0->patch[i], sizeof (*local_patch_v0->patch[i]));
			}
		}

		/*********** Send data (non-blocking point-to-point communication) **********/
		int req_i = 0;
		MPI_Request req[max_found_reg_patches * procs_num];
		MPI_Status stat[max_found_reg_patches * procs_num];

		for (int i = 0; i < found_reg_patches_count; i++)
		{
			/* Create patch send data type */
			int array_size[MPR_MAX_DIMENSIONS] = {file->mpr->local_box[0]*bits, file->mpr->local_box[1], file->mpr->local_box[2]};
			int array_subsize[MPR_MAX_DIMENSIONS] = {found_reg_patches[i]->physical_size[0]*bits, found_reg_patches[i]->physical_size[1], found_reg_patches[i]->physical_size[2]};
			int local_send_off[MPR_MAX_DIMENSIONS] = {(found_reg_patches[i]->physical_offset[0] - file->mpr->local_offset[0])*bits,
							(found_reg_patches[i]->physical_offset[1] - file->mpr->local_offset[1]),
							(found_reg_patches[i]->physical_offset[2] - file->mpr->local_offset[2])};

			MPI_Datatype send_type;
			MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, array_size, array_subsize, local_send_off, MPI_ORDER_FORTRAN, MPI_BYTE, &send_type);
			MPI_Type_commit(&send_type);

			/* MPI Send function */
			MPI_Isend(local_patch->buffer, 1, send_type, found_reg_patches[i]->owned_rank, rank, comm, &req[req_i]);
			req_i++;
			MPI_Type_free(&send_type);
		}

		/*********** Receive data (non-blocking point-to-point communication) **********/
		for (int i = 0; i < local_patch_num; i++)
		{
			local_patch->patch[i]->buffer = malloc(patch_size * bits);
			memset(local_patch->patch[i]->buffer, 0, patch_size * bits);
			local_patch->patch[i]->patch_buffer_size = patch_size * bits;

			for (int j = 0; j < procs_num; j++)
			{
				if (patch_shared_ranks[i][j] == 1)
				{
					/* Creating patch receive data type */
					int array_size[MPR_MAX_DIMENSIONS] = {local_patch->patch[i]->size[0]*bits, local_patch->patch[i]->size[1], local_patch->patch[i]->size[2]};
					int array_subsize[MPR_MAX_DIMENSIONS] = {share_physical_sizes[i][j*MPR_MAX_DIMENSIONS]*bits, share_physical_sizes[i][j*MPR_MAX_DIMENSIONS + 1], share_physical_sizes[i][j*MPR_MAX_DIMENSIONS + 2]};
					int subarray_offset[MPR_MAX_DIMENSIONS] = {patch_share_offsets[i][j*MPR_MAX_DIMENSIONS]*bits, patch_share_offsets[i][j*MPR_MAX_DIMENSIONS + 1], patch_share_offsets[i][j*MPR_MAX_DIMENSIONS + 2]};

					MPI_Datatype recv_type;
					MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, array_size, array_subsize, subarray_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &recv_type);
					MPI_Type_commit(&recv_type);

					/* MPI Recv function */
					MPI_Irecv(local_patch->patch[i]->buffer, 1, recv_type, j, j, comm, &req[req_i]);
					req_i++;
					MPI_Type_free(&recv_type);
				}
			}
		}
		MPI_Waitall(req_i, req, stat); /* Wait all the send and receive to be finished */
	}
	/**********************************************************************************************/

	/* Clean up */
	for (int i = 0; i < found_reg_patches_count; i++)
	{
		free(found_reg_patches[i]);
		found_reg_patches[i] = 0;
	}
	free(found_reg_patches);
	return MPR_success;
}

static int intersect_patch(int* a_size, int* a_offset, int* b_size, int* b_offset)
{
	int d = 0, check_bit = 0;
	for (d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		check_bit = check_bit || (a_offset[d] + a_size[d] - 1) < b_offset[d] || (b_offset[d] + b_size[d] - 1) < a_offset[d];
	}
	return !(check_bit);
}

/* Function to check if patch A and B intersects */
//static int intersect_patch(MPR_patch A, MPR_patch B)
//{
//  int d = 0, check_bit = 0;
//  for (d = 0; d < MPR_MAX_DIMENSIONS; d++)
//    check_bit = check_bit || (A->offset[d] + A->size[d] - 1) < B->offset[d] || (B->offset[d] + B->size[d] - 1) < A->offset[d];
//
//  return !(check_bit);
//}

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
