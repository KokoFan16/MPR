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

	int* global_box = file->mpr->global_box;
	int* patch_box = file->mpr->patch_box;
	int* local_box = file->mpr->local_box;
	int* local_offset = file->mpr->local_offset;

	int patch_size = patch_box[0] * patch_box[1] * patch_box[2]; /* The size of regular patch */
	/***************************************************************************/

	/******************** Calculate total number of patches *********************/
	int patch_dimensional_counts[MPR_MAX_DIMENSIONS];
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
		patch_dimensional_counts[d] = ceil((float)global_box[d]/patch_box[d]);
	/* The total number of patches for the global data */
	int total_patch_num = patch_dimensional_counts[0] * patch_dimensional_counts[1] * patch_dimensional_counts[2];
	file->mpr->total_patches_num = total_patch_num; /* The global total number of patches */
	/***************************************************************************/

	/************************ Gather local patch info **************************/
	int local_patch_offset_array[procs_num * MPR_MAX_DIMENSIONS];
	MPI_Allgather(local_offset, MPR_MAX_DIMENSIONS, MPI_INT, local_patch_offset_array, MPR_MAX_DIMENSIONS, MPI_INT, comm);
	/***************************************************************************/

	/******************** Calculate local number of patches *********************/
    int local_patch_num = total_patch_num / procs_num; /* The local number of patches per process */
    int remain_patch_num = total_patch_num % procs_num; /* Remainder */
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

	/***************************** Find shared patches and ranks *******************************/
    int local_own_patch_count = 0;
	int local_own_patch_ids[total_patch_num]; /* the array of current number of patches per process */
	memset(local_own_patch_ids, -1, total_patch_num * sizeof(int)); /* Initialization */

	int cur_local_offset[MPR_MAX_DIMENSIONS];
	int local_end[MPR_MAX_DIMENSIONS];
	int cur_local_end[MPR_MAX_DIMENSIONS];
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		cur_local_offset[d] = (local_offset[d]/patch_box[d]);
		local_end[d] = local_offset[d] + local_box[d];
		cur_local_end[d] = ceil(local_end[d]/(float)patch_box[d]);
	}

	int local_count = 0;
	for (int k = cur_local_offset[2]; k < cur_local_end[2]; k++)
	{
		for (int j = cur_local_offset[1]; j < cur_local_end[1]; j++)
		{
			for (int i = cur_local_offset[0]; i < cur_local_end[0]; i++)
			{
				int patch_id =  k * patch_dimensional_counts[1] * patch_dimensional_counts[0] + j * patch_dimensional_counts[0] + i;
				local_own_patch_ids[local_own_patch_count++] = patch_id;
			}
		}
	}
	/********************************************************************************************/

	int max_local_pnum = 0;
	MPI_Allreduce(&local_own_patch_count, &max_local_pnum, 1, MPI_INT, MPI_MAX, comm);

	int* owned_patches = malloc(procs_num * max_local_pnum * sizeof(int));
	MPI_Allgather(&local_own_patch_ids, max_local_pnum, MPI_INT, owned_patches, max_local_pnum, MPI_INT, comm);

	int shared_rank_count[total_patch_num];
	memset(shared_rank_count, 0, total_patch_num * sizeof(int));

	int max_owned_patch_count = pow(2, MPR_MAX_DIMENSIONS);
	int shared_patch_ranks[total_patch_num][max_owned_patch_count];

	for(int i = 0; i < procs_num; i++)
	{
		for (int j = 0; j < max_local_pnum; j++)
		{
			int patch_id = owned_patches[i*max_local_pnum + j];
			if (patch_id != -1)
			{
				shared_patch_ranks[patch_id][shared_rank_count[patch_id]] = i;
				shared_rank_count[patch_id] += 1;
			}
		}
	}

    /***************************** Patch assignment *******************************/
	int cur_assign_patch_num[procs_num];
	memset(cur_assign_patch_num, 0, procs_num * sizeof(int));

	int patch_assignment[total_patch_num];
	memset(patch_assignment, 0, total_patch_num * sizeof(int));

	int local_assigned_patches[local_patch_num];
	memset(local_assigned_patches, -1, local_patch_num * sizeof(int));
	int local_assigned_count = 0;

	for (int i = 0; i < total_patch_num; i++)
	{
		int flag = 0; /* If the patch has been assigned to any process */
		int assigned_rank = 0;
		for (int p = 0; p < shared_rank_count[i]; p++)
		{
			int process_id = shared_patch_ranks[i][p];
			if (cur_assign_patch_num[process_id] < required_local_patch_num[process_id])
			{
				assigned_rank = process_id;
				flag = 1; /* 1 means the current patch is assigned */
				break;
			}
		}
		if (flag == 0) /* If the current patch didn't be assigned to a process */
		{
			for (int p = 0; p < procs_num; p++)
			{
				if (cur_assign_patch_num[p] < required_local_patch_num[p])
				{
					assigned_rank = p;
					break;
				}
			}
		}
		cur_assign_patch_num[assigned_rank] += 1; /* the number of patches of rank a add 1 */
		patch_assignment[i] = assigned_rank;
		if (rank == assigned_rank)
			local_assigned_patches[local_assigned_count++] = i;
	}
	/******************************************************************************/

	/*********************************** Data exchange and merge *********************************/
	for (int v = start_var_index; v < end_var_index; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		local_patch->patch_count = local_patch_num;
		local_patch->patch = malloc(sizeof(MPR_patch*)*local_patch_num);

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		int req_i = 0;
		MPI_Request req[max_local_pnum * max_owned_patch_count];
		MPI_Status stat[max_local_pnum * max_owned_patch_count];

		/*********** Receive data (non-blocking point-to-point communication) **********/
		int array_size[MPR_MAX_DIMENSIONS] = {patch_box[0]*bytes, patch_box[0], patch_box[0]};
		for (int i = 0; i < local_patch_num; i++)
		{
			int patch_id = local_assigned_patches[i];
			local_patch->patch[i] = (MPR_patch)malloc(sizeof(*local_patch->patch[i]));
			local_patch->patch[i]->global_id = patch_id;

			local_patch->patch[i]->buffer = malloc(patch_size * bytes);
			memset(local_patch->patch[i]->buffer, 0, patch_size * bytes);
			local_patch->patch[i]->patch_buffer_size = patch_size * bytes;

			int z = patch_id / (patch_dimensional_counts[0] * patch_dimensional_counts[1]);
			int remain = patch_id - z * (patch_dimensional_counts[0] * patch_dimensional_counts[1]);
			int y = remain / patch_dimensional_counts[0];
			int x = remain % patch_dimensional_counts[0];

			local_patch->patch[i]->offset[0] = x * patch_box[0];
			local_patch->patch[i]->offset[1] = y * patch_box[1];
			local_patch->patch[i]->offset[2] = z * patch_box[2];

			int patch_end[MPR_MAX_DIMENSIONS];
			for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
			{
				local_patch->patch[i]->size[d] = patch_box[d];
				patch_end[d] = local_patch->patch[i]->offset[d] + patch_box[d];
			}

			int physical_size[MPR_MAX_DIMENSIONS];
			int physical_offset[MPR_MAX_DIMENSIONS];
			int patch_share_offset[MPR_MAX_DIMENSIONS];
			int local_end[MPR_MAX_DIMENSIONS];

			int shared_processes_count = shared_rank_count[patch_id];
			for (int j = 0; j < shared_processes_count; j++)
			{
				int process_id = shared_patch_ranks[patch_id][j];

				int local_offset[MPR_MAX_DIMENSIONS];
				memcpy(local_offset, &local_patch_offset_array[process_id * MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));

				/* Calculate the physical size */
				for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
				{
					local_end[d] = local_offset[d] + local_box[d];
					physical_offset[d] = local_patch->patch[i]->offset[d];
					physical_size[d] = patch_box[d];

					if (patch_end[d] > local_end[d])
						physical_size[d] = local_end[d] - local_patch->patch[i]->offset[d];

					if (local_patch->patch[i]->offset[d] < local_offset[d])
					{
						physical_offset[d] = local_offset[d];
						physical_size[d] = patch_end[d] - local_offset[d];
					}
					patch_share_offset[d] = physical_offset[d] - local_patch->patch[i]->offset[d];
				}
				patch_share_offset[0] *= bytes;
				physical_size[0] *= bytes;

				MPI_Datatype recv_type;
				MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, array_size, physical_size, patch_share_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &recv_type);
				MPI_Type_commit(&recv_type);

				/* MPI Recv function */
				MPI_Irecv(local_patch->patch[i]->buffer, 1, recv_type, process_id, process_id, comm, &req[req_i]);
				req_i++;
				MPI_Type_free(&recv_type);
			}
		}

		/*********** Send data (non-blocking point-to-point communication) **********/
		int sent_array[MPR_MAX_DIMENSIONS] = {local_box[0] * bytes, local_box[1], local_box[2]};
		for (int i = 0; i < local_own_patch_count; i++)
		{
			int patch_id = local_own_patch_ids[i];

			int z = patch_id / (patch_dimensional_counts[0] * patch_dimensional_counts[1]);
			int remain = patch_id - z * (patch_dimensional_counts[0] * patch_dimensional_counts[1]);
			int y = remain / patch_dimensional_counts[0];
			int x = remain % patch_dimensional_counts[0];

			int offset[MPR_MAX_DIMENSIONS] = {x * patch_box[0], y * patch_box[1], z * patch_box[2]};
			int patch_end[MPR_MAX_DIMENSIONS] = {((x + 1) * patch_box[0]), (y + 1) * patch_box[1], (z + 1) * patch_box[2]};

			int physical_size[MPR_MAX_DIMENSIONS];
			int physical_offset[MPR_MAX_DIMENSIONS];
			int send_offset[MPR_MAX_DIMENSIONS];

			for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
			{
				physical_offset[d] = offset[d];
				physical_size[d] = patch_box[d];

				if (patch_end[d] > local_end[d])
					physical_size[d] = local_end[d] - offset[d];

				if (offset[d] < local_offset[d])
				{
					physical_offset[d] = local_offset[d];
					physical_size[d] = patch_end[d] - local_offset[d];
				}
				send_offset[d] = physical_offset[d] - local_offset[d];
			}
			physical_size[0] *= bytes;
			send_offset[0] *= bytes;

			MPI_Datatype send_type;
			MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, sent_array, physical_size, send_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &send_type);
			MPI_Type_commit(&send_type);

			/* MPI Send function */
			MPI_Isend(local_patch->buffer, 1, send_type, patch_assignment[patch_id], rank, comm, &req[req_i]);
			req_i++;
			MPI_Type_free(&send_type);
		}
		MPI_Waitall(req_i, req, stat); /* Wait all the send and receive to be finished */
	}
	/**********************************************************************************************/

	printf("The number of patches of process %d is %d\n", file->comm->simulation_rank, local_patch_num);

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
