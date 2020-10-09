/*
 * MPR_aggregation.c
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

static void decide_aggregator(MPR_file file, int* agg_ranks);
static int calZOrder(int x, int y, int z);

MPR_return_code MPR_aggregation_perform(MPR_file file, int svi, int evi)
{
	int proc_num = file->comm->simulation_nprocs;  /* The number of processes */
	int rank = file->comm->simulation_rank; /* The rank of each process */
	MPI_Comm comm = file->comm->simulation_comm; /* The MPI communicator */

	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */
	int total_patch_num = file->mpr->total_patches_num; /* The number of total patches */
	int node_num = file->mpr->node_num; /* the number of nodes */

	int max_pcount = total_patch_num / proc_num + 1;

	for (int v  = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int patch_count = local_patch->patch_count; /* the number of patches per process */

		/* gather the size all the sub-bands per patch */
		int* global_subband_sizes = NULL;
		int* subband_sizes = NULL;
		int* local_subband_sizes = NULL;
		int subbands_num = 0;
		if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
		{
			subbands_num = file->mpr->wavelet_trans_num * 7 + 1;
			global_subband_sizes = malloc(max_pcount * proc_num * subbands_num * sizeof(int));
			subband_sizes = malloc(total_patch_num * subbands_num * sizeof(int));
			local_subband_sizes = malloc(max_pcount * subbands_num * sizeof(int));
			memset(local_subband_sizes, 0, max_pcount * subbands_num * sizeof(int));
		}

		int local_patch_size_id_rank[max_pcount * 3];
		memset(local_patch_size_id_rank, -1, max_pcount * 3 * sizeof(int));
		for (int i = 0; i < patch_count; i++)
		{
			local_patch_size_id_rank[i * 3] = local_patch->patch[i]->global_id;
			local_patch_size_id_rank[i * 3 + 1] = local_patch->patch[i]->patch_buffer_size;
			local_patch_size_id_rank[i * 3 + 2] = rank;

			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				memcpy(&local_subband_sizes[i*subbands_num], local_patch->patch[i]->subbands_comp_size, subbands_num*sizeof(int));
		}

		int* patch_size_id = malloc(max_pcount * proc_num * 3 * sizeof(int));
		MPI_Allgather(local_patch_size_id_rank, max_pcount * 3, MPI_INT, patch_size_id, max_pcount * 3, MPI_INT, comm);

		if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
			MPI_Allgather(local_subband_sizes, max_pcount * subbands_num, MPI_INT, global_subband_sizes, max_pcount * subbands_num, MPI_INT, comm);
		free(local_subband_sizes);

		int* patch_sizes = malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the size of patch i */
		int* patch_ranks = malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the owned rank of patch i */
		for (int i = 0; i < max_pcount * proc_num; i++)
		{
			int id = patch_size_id[i*3];
			if (id > -1)
			{
				patch_sizes[id] = patch_size_id[i*3 + 1];
				patch_ranks[id] = patch_size_id[i*3 + 2];
				if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
					memcpy(&subband_sizes[id*subbands_num], &global_subband_sizes[i*subbands_num], subbands_num*sizeof(int));
			}
		}
		free(patch_size_id);
		free(global_subband_sizes);

		unsigned long long total_size = 0; /* The total size of all the patches across all the processes */
		for (int i = 0; i < total_patch_num; i++)
			total_size += patch_sizes[i];
		double average_file_size = total_size / (double)out_file_num; /* The idea average file size*/
		local_patch->compression_ratio = total_size;

		/* Calculate the patch count in each dimension, and its next power 2 value (e.g., 3x3x3 -> 4x4x4)*/
		int patch_count_xyz[MPR_MAX_DIMENSIONS];
		int next_2_power_xyz[MPR_MAX_DIMENSIONS];
		for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
		{
			local_patch->compression_ratio /= file->mpr->global_box[i];
			patch_count_xyz[i] = ceil((float)file->mpr->global_box[i] / file->mpr->patch_box[i]);
			next_2_power_xyz[i] = pow(2, ceil(log2(patch_count_xyz[i])));
		}

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */
		local_patch->compression_ratio /= bytes;

		/* Reorder the patch size array and id array with z-order curve */
		int patch_count_power2 = next_2_power_xyz[0] * next_2_power_xyz[1] * next_2_power_xyz[2];
		int* patch_sizes_zorder = (int*)malloc(patch_count_power2 * sizeof(int)); /* patch size with z-order */
		memset(patch_sizes_zorder, 0, patch_count_power2 * sizeof(int));
		int* patch_ids_zorder = (int*)malloc(patch_count_power2 * sizeof(int));  /* patch id with z-order */
		memset(patch_ids_zorder, -1, patch_count_power2 * sizeof(int));
		for (int z = 0; z < patch_count_xyz[2]; z++)
		{
			for (int y = 0; y < patch_count_xyz[1]; y++)
			{
				for (int x = 0; x < patch_count_xyz[0]; x++)
				{
					int zorder = calZOrder(x, y, z);  /* Calculate the index with z-order */
					int index = z * patch_count_xyz[1] * patch_count_xyz[0] + y * patch_count_xyz[0] + x;
					patch_sizes_zorder[zorder] = patch_sizes[index];
					patch_ids_zorder[zorder] = index;
				}
			}
		}

		int agg_ranks[out_file_num]; /* AGG Array */
		decide_aggregator(file, agg_ranks); /* Decide AGG */

		/************************* Assign patches **************************/
		int patch_assign_array[total_patch_num];
		memset(patch_assign_array, -1, total_patch_num * sizeof(int));

		int agg_sizes[out_file_num]; /* the current size of aggregators */
		memset(agg_sizes, 0, out_file_num * sizeof(int));

		int recv_array[total_patch_num]; /* Local receive array per process */
		int recv_num = 0;

		int p = 0;
		for (int a = 0; a < out_file_num; a++)
		{
			while (p < patch_count_power2)
			{
				if (agg_sizes[a] < average_file_size)
				{
					if (patch_ids_zorder[p] > -1)
					{
						patch_assign_array[patch_ids_zorder[p]] = agg_ranks[a];
						if (rank == agg_ranks[a])
							recv_array[recv_num++] = patch_ids_zorder[p];
					}
					agg_sizes[a] += patch_sizes_zorder[p];
					p++;
				}
				else
					break;
			}
		}
		free(patch_sizes_zorder);
		free(patch_ids_zorder);
		local_patch->agg_patch_count = recv_num;
		/**********************************************************************/

		int agg_size = 0;
		for (int i = 0; i < out_file_num; i++)
		{
			if (rank == agg_ranks[i])
				agg_size = agg_sizes[i];
		}

		local_patch->agg_patch_id_array = malloc(recv_num * sizeof(int));
		local_patch->agg_patch_disps = malloc(recv_num * sizeof(int));
		local_patch->agg_patch_size = malloc(recv_num * sizeof(int));
		if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
			local_patch->agg_subbands_size = malloc(recv_num * subbands_num * sizeof(int));

		/********************** Point-to-point communication **********************/
		local_patch->buffer = malloc(agg_size); /* reuse the local buffer per variable */
		local_patch->out_file_size = agg_size;

		int comm_count = patch_count + recv_num; /* the maximum transform times */
		MPI_Request req[comm_count];
		MPI_Status stat[comm_count];
		int req_id = 0;
		int offset = 0;

		/* Send data */
		for (int i = 0; i < patch_count; i++)
		{
			int id = local_patch->patch[i]->global_id;
			int buffer_size = local_patch->patch[i]->patch_buffer_size;
			MPI_Isend(local_patch->patch[i]->buffer, buffer_size, MPI_BYTE, patch_assign_array[id], id, comm, &req[req_id]);
			req_id++;
		}

		/* Recv data */
		int max_xyz[MPR_MAX_DIMENSIONS] = {0, 0, 0};
		int min_xyz[MPR_MAX_DIMENSIONS] = {INT_MAX, INT_MAX, INT_MAX};

		for (int i = 0; i < recv_num; i++)
		{
			int z = recv_array[i] / (patch_count_xyz[0] * patch_count_xyz[1]);
			int y = (recv_array[i] - (z * patch_count_xyz[0] * patch_count_xyz[1])) / patch_count_xyz[1];
			int x = recv_array[i] - z * patch_count_xyz[0] * patch_count_xyz[1] - y * patch_count_xyz[1];

			if (z < min_xyz[2]) min_xyz[2] = z;
			if (z > max_xyz[2]) max_xyz[2] = z;
			if (y < min_xyz[1]) min_xyz[1] = y;
			if (y > max_xyz[1]) max_xyz[1] = y;
			if (x < min_xyz[0]) min_xyz[0] = x;
			if (x > max_xyz[0]) max_xyz[0] = x;

			MPI_Irecv(&local_patch->buffer[offset], patch_sizes[recv_array[i]], MPI_BYTE, patch_ranks[recv_array[i]], recv_array[i], comm, &req[req_id]);
			local_patch->agg_patch_id_array[i] = recv_array[i];
			local_patch->agg_patch_disps[i] = offset;
			local_patch->agg_patch_size[i] = patch_sizes[recv_array[i]];
			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				memcpy(&local_patch->agg_subbands_size[i*subbands_num], &subband_sizes[recv_array[i]*subbands_num], subbands_num*sizeof(int));
			offset += patch_sizes[recv_array[i]];
			req_id++;
		}
		MPI_Waitall(req_id, req, stat);
		free(patch_ranks);
		free(patch_sizes);
		free(subband_sizes);

		for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
		{
			local_patch->bounding_box[i] = min_xyz[i];
			local_patch->bounding_box[i + MPR_MAX_DIMENSIONS] = max_xyz[i];
		}
	}
	return MPR_success;
}

/* Decide aggregators */
static void decide_aggregator(MPR_file file, int* agg_ranks)
{
	int rank = file->comm->simulation_rank; /* The rank of each process */
	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */
	int node_num = file->mpr->node_num; /* the number of nodes */
	/* Decide the aggregators */
	if (node_num == 1)  /* If the all the processes belong to one node */
	{
		if (rank < out_file_num)
			file->mpr->is_aggregator = 1;
	}
	else  /* If they belong to multiple nodes */
	{
		int avg_files_per_node = out_file_num / node_num;  /* The average number of files per node */
		int proc_num_last_node = file->mpr->proc_num_last_node; /* The number of processes of last node */

		int file_assign_per_node[node_num]; /* Array: the number of assigned files for all the nodes */
		/* The number of files for last node */
		file_assign_per_node[node_num - 1] = (avg_files_per_node < proc_num_last_node)? avg_files_per_node: proc_num_last_node;
		/* The number of files for others */
		out_file_num -= file_assign_per_node[node_num - 1];
		avg_files_per_node = out_file_num / (node_num - 1);
		for (int i = 0; i < (node_num - 1); i++)
			file_assign_per_node[i] = (i < (out_file_num % (node_num - 1)))? (avg_files_per_node + 1): avg_files_per_node;
		/* Decide aggregators */
		int r = 0;
		for (int i = 0; i < file->comm->simulation_nprocs; i++)
		{
			int id = i / file->mpr->proc_num_per_node;
			if (i % file->mpr->proc_num_per_node < file_assign_per_node[id])
			{
				agg_ranks[r++] = i;
				if (rank == i)
					file->mpr->is_aggregator = 1;
			}
		}
	}
}


static int calZOrder(int x, int y, int z)
{
	static const unsigned int B[] = {0x09249249, 0x030C30C3, 0x0300F00F, 0x030000FF};
	static const unsigned int S[] = {2, 4, 8, 16};

	x = (x | (x << S[3])) & B[3];
	x = (x | (x << S[2])) & B[2];
	x = (x | (x << S[1])) & B[1];
	x = (x | (x << S[0])) & B[0];

	y = (y | (y << S[3])) & B[3];
	y = (y | (y << S[2])) & B[2];
	y = (y | (y << S[1])) & B[1];
	y = (y | (y << S[0])) & B[0];

	z = (z | (z << S[3])) & B[3];
	z = (z | (z << S[2])) & B[2];
	z = (z | (z << S[1])) & B[1];
	z = (z | (z << S[0])) & B[0];

	int result = x | (y << 1) | (z << 2);
	return result;
}
