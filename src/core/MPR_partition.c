/*
 * MPR_restructure.c
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

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


MPR_return_code MPR_is_partition(MPR_file file, int svi, int evi)
{
	int is_same = 0;
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		int local = file->mpr->local_box[d];
		if (local + file->mpr->local_offset[d] > file->mpr->global_box[d])
			local = file->mpr->global_box[d] - file->mpr->local_offset[d];
		if (local == file->mpr->patch_box[d])
			is_same += 1;
	}

	int min_same = 0;
	MPI_Allreduce(&is_same, &min_same, 1, MPI_INT, MPI_MIN, file->comm->simulation_comm);

	if (min_same == MPR_MAX_DIMENSIONS)
	{
		if (MPR_processing(file, svi, evi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}
	else
	{
		/* Perform restructure phase */
		if (MPR_partition_perform(file, svi, evi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}

	return MPR_success;
}


MPR_return_code MPR_processing(MPR_file file, int svi, int evi)
{
	int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2];
	file->mpr->total_patches_num = file->comm->simulation_nprocs;

	for (int v = svi; v < evi; v++)
	{
		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		local_patch->patch = (MPR_patch*)malloc(sizeof(MPR_patch*));
		local_patch->patch_count = 1;
		local_patch->patch[0] = (MPR_patch)malloc(sizeof(*local_patch->patch[0]));
		local_patch->patch[0]->global_id = file->comm->simulation_rank;
		local_patch->patch[0]->patch_buffer_size = patch_size * bytes;

		local_patch->patch[0]->buffer = (unsigned char*) malloc(patch_size * bytes);
		memcpy(local_patch->patch[0]->buffer, local_patch->buffer, patch_size*bytes);
	}
	return MPR_success;
}


MPR_return_code MPR_partition_perform(MPR_file file, int start_var_index, int end_var_index)
{
//	double total_start = MPI_Wtime();
//
//	double sync_start = MPI_Wtime();
//	MPI_Barrier(file->comm->simulation_comm);
//	double sync_end = MPI_Wtime();

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
//	double gather_start = MPI_Wtime();
	int patch_dimensional_counts[MPR_MAX_DIMENSIONS];
	int process_dimensional_counts[MPR_MAX_DIMENSIONS];
	int local_end[MPR_MAX_DIMENSIONS];
	for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
	{
		local_end[d] = local_offset[d] + local_box[d];
		patch_dimensional_counts[d] = ceil((float)global_box[d]/patch_box[d]);
		process_dimensional_counts[d] = ceil((float)global_box[d]/local_box[d]);
	}
	/* The total number of patches for the global data */
	int total_patch_num = patch_dimensional_counts[0] * patch_dimensional_counts[1] * patch_dimensional_counts[2];
	file->mpr->total_patches_num = total_patch_num; /* The global total number of patches */
	/***************************************************************************/

	/************************ Gather local patch info **************************/
	int local_patch_offset_array[procs_num * MPR_MAX_DIMENSIONS];
	MPI_Allgather(local_offset, MPR_MAX_DIMENSIONS, MPI_INT, local_patch_offset_array, MPR_MAX_DIMENSIONS, MPI_INT, comm);
//	double gather_end = MPI_Wtime();
	/***************************************************************************/

	/******************** Calculate local number of patches *********************/
//	double cal_count_start = MPI_Wtime();
    int local_patch_num = total_patch_num / procs_num; /* The local number of patches per process */
    int remain_patch_num = total_patch_num % procs_num; /* Remainder */

    if (remain_patch_num > 0)
    {
		int gap = procs_num / remain_patch_num;
		if(rank % gap == 0 && rank < (gap*remain_patch_num))
			local_patch_num += 1;
    }

    int required_local_patch_num[procs_num]; /* The required local number of patches per process */
    MPI_Allgather(&local_patch_num, 1, MPI_INT, required_local_patch_num, 1, MPI_INT, comm);
//    double cal_count_end = MPI_Wtime();
    /***************************************************************************/

	/***************************** Find max shared patches count *******************************/
//    double max_count_start = MPI_Wtime();
    int patch_offsets[total_patch_num][MPR_MAX_DIMENSIONS];

	int max_owned_patch_count = 0;

	int global_id = 0; /* The global id for each patch */
	for (int k = 0; k < global_box[2]; k += patch_box[2])
	{
		for (int j = 0; j < global_box[1]; j += patch_box[1])
		{
			for (int i = 0; i < global_box[0]; i += patch_box[0])
			{
				int start[MPR_MAX_DIMENSIONS] = {i, j, k};
				int end[MPR_MAX_DIMENSIONS] = {i + patch_box[0], j + patch_box[1], k + patch_box[2]};

				int process_count = 1;
				for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
				{
					if (end[d] > global_box[d])
						end[d] = global_box[d];
					int start_dimensions = start[d] / local_box[d];
					int end_dimensions = (end[d] - 1) / local_box[d];
					process_count *= (end_dimensions - start_dimensions + 1);
				}

				if (process_count > max_owned_patch_count)
					max_owned_patch_count = process_count;

				memcpy(patch_offsets[global_id], start, MPR_MAX_DIMENSIONS * sizeof(int));
				global_id++;
			}
		}
	}
//	double max_count_end = MPI_Wtime();

    /***************************** Patch assignment *******************************/
//	double assign_start = MPI_Wtime();
	int local_own_patch_count = 0;
	int local_own_patch_ids[total_patch_num]; /* the array of current number of patches per process */
	memset(local_own_patch_ids, -1, total_patch_num * sizeof(int)); /* Initialization */

	int cur_assign_patch_num[procs_num];
	memset(cur_assign_patch_num, 0, procs_num * sizeof(int));

	int patch_assignment[total_patch_num];
	memset(patch_assignment, 0, total_patch_num * sizeof(int));

	int local_assigned_patches[local_patch_num];
	memset(local_assigned_patches, -1, local_patch_num * sizeof(int));
	int local_assigned_count = 0;

	int local_shared_rank_count[local_patch_num];
	int local_shared_patches_ranks[local_patch_num][max_owned_patch_count];

	for (int i = 0; i < total_patch_num; i++)
	{

		int start_dimensions[MPR_MAX_DIMENSIONS];
		int end_dimensions[MPR_MAX_DIMENSIONS];
		int end[MPR_MAX_DIMENSIONS];

		for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
		{
			end[d] = patch_offsets[i][d] + patch_box[d];
			if (end[d] > global_box[d])
				end[d] = global_box[d];
			start_dimensions[d] = patch_offsets[i][d] / local_box[d];
			end_dimensions[d] = (end[d] - 1) / local_box[d];
		}

		int process_count = 0;
		int shared_patch_ranks[max_owned_patch_count];

		for (int z = start_dimensions[2]; z < end_dimensions[2] + 1; z++)
			for (int y = start_dimensions[1]; y < end_dimensions[1] + 1; y++)
				for (int x = start_dimensions[0]; x < end_dimensions[0] + 1; x++)
				{
					int process_id = z * process_dimensional_counts[1] * process_dimensional_counts[0] + y * process_dimensional_counts[0] + x;
					shared_patch_ranks[process_count++] = process_id;
					if (rank == process_id)
						local_own_patch_ids[local_own_patch_count++] = i;
				}

		int flag = 0; /* If the patch has been assigned to any process */
		int assigned_rank = 0;
		for (int p = 0; p < process_count; p++)
		{
			int process_id = shared_patch_ranks[p];
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
		{
			local_assigned_patches[local_assigned_count] = i;
			memcpy(&local_shared_patches_ranks[local_assigned_count], &shared_patch_ranks, process_count * sizeof(int));
			local_shared_rank_count[local_assigned_count] = process_count;
			local_assigned_count++;
		}
	}
//	double assign_end = MPI_Wtime();
	/******************************************************************************/

	/*********************************** Data exchange and merge *********************************/
//	double comm_start = MPI_Wtime();
	for (int v = start_var_index; v < end_var_index; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		local_patch->patch_count = local_patch_num;
		local_patch->patch = (MPR_patch*)malloc(sizeof(MPR_patch*)*local_patch_num);

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		int req_i = 0;
		MPI_Request req[local_own_patch_count + local_patch_num * max_owned_patch_count];
		MPI_Status stat[local_own_patch_count + local_patch_num * max_owned_patch_count];

		/*********** Receive data (non-blocking point-to-point communication) **********/
		int receive_array[MPR_MAX_DIMENSIONS] = {patch_box[0] * bytes, patch_box[1], patch_box[2]};
		for (int i = 0; i < local_patch_num; i++)
		{
			int patch_id = local_assigned_patches[i];
			local_patch->patch[i] = (MPR_patch)malloc(sizeof(*local_patch->patch[i]));
			local_patch->patch[i]->global_id = patch_id;

			local_patch->patch[i]->buffer = (unsigned char*)malloc(patch_size * bytes);
			memset(local_patch->patch[i]->buffer, 0, patch_size * bytes);
			local_patch->patch[i]->patch_buffer_size = patch_size * bytes;

			int patch_end[MPR_MAX_DIMENSIONS];
			for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
			{
				local_patch->patch[i]->offset[d] = patch_offsets[patch_id][d];
				local_patch->patch[i]->size[d] = patch_box[d];
				patch_end[d] = local_patch->patch[i]->offset[d] + patch_box[d];

				if (patch_end[d] > global_box[d])
					patch_end[d] = global_box[d];
			}

			int physical_size[MPR_MAX_DIMENSIONS];
			int physical_offset[MPR_MAX_DIMENSIONS];
			int patch_share_offset[MPR_MAX_DIMENSIONS];
			int local_end[MPR_MAX_DIMENSIONS];

			int shared_processes_count = local_shared_rank_count[i];
			for (int j = 0; j < shared_processes_count; j++)
			{
				int process_id = local_shared_patches_ranks[i][j];

				int local_offset[MPR_MAX_DIMENSIONS];
				memcpy(local_offset, &local_patch_offset_array[process_id * MPR_MAX_DIMENSIONS], MPR_MAX_DIMENSIONS * sizeof(int));

				/* Calculate the physical size */
				for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
				{
					local_end[d] = local_offset[d] + local_box[d];

					physical_offset[d] = patch_offsets[patch_id][d];
					physical_size[d] = patch_end[d] - physical_offset[d];

					if (patch_end[d] > local_end[d])
						physical_size[d] = local_end[d] - patch_offsets[patch_id][d];

					if (patch_offsets[patch_id][d] < local_offset[d])
					{
						physical_offset[d] = local_offset[d];
						physical_size[d] = patch_end[d] - local_offset[d];
					}
					patch_share_offset[d] = physical_offset[d] - patch_offsets[patch_id][d];
				}
				patch_share_offset[0] *= bytes;
				physical_size[0] *= bytes;

				MPI_Datatype recv_type;
				MPI_Type_create_subarray(MPR_MAX_DIMENSIONS, receive_array, physical_size, patch_share_offset, MPI_ORDER_FORTRAN, MPI_BYTE, &recv_type);
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

			int patch_end[MPR_MAX_DIMENSIONS];
			int physical_size[MPR_MAX_DIMENSIONS];
			int physical_offset[MPR_MAX_DIMENSIONS];
			int send_offset[MPR_MAX_DIMENSIONS];

			for (int d = 0; d < MPR_MAX_DIMENSIONS; d++)
			{
				patch_end[d] = patch_offsets[patch_id][d] + patch_box[d];
				physical_size[d] = patch_box[d];
				physical_offset[d] = patch_offsets[patch_id][d];

				if (patch_end[d] > global_box[d])
				{
					patch_end[d] = global_box[d];
					physical_size[d] = patch_end[d] - physical_offset[d];
				}

				if (patch_end[d] > local_end[d])
					physical_size[d] -= (patch_end[d] - local_end[d]);

				if (patch_offsets[patch_id][d] < local_offset[d])
				{
					physical_offset[d] = local_offset[d];
					physical_size[d] -= (local_offset[d] - patch_offsets[patch_id][d]);
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
//	double comm_end = MPI_Wtime();
	/**********************************************************************************************/

//	double total_end = MPI_Wtime();

//	printf("Partition %d: total %f [ sync %f gather %f cc %f cmc %f assign %f comm %f ] \n", rank, (total_end - total_start), (sync_end - sync_start), (gather_end - gather_start), (cal_count_end - cal_count_start),
//				(max_count_end - max_count_start), (assign_end - assign_start), (comm_end - comm_start));

	return MPR_success;
}

