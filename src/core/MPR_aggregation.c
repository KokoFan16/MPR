/*
 * MPR_aggregation.c
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

static void decide_aggregator(MPR_file file, int* agg_ranks);
static int calculate_agg_num_with_node(MPR_file file);
static unsigned int calZOrder(unsigned int x, unsigned int y, unsigned int z);

MPR_return_code MPR_aggregation_perform(MPR_file file, int svi, int evi)
{
	/* If the aggregation mode isn't set */
	if (file->mpr->aggregation_mode == -1)
	{
		file->mpr->is_aggregator = 1; /* all the processes are aggregators */
		for (int v = svi; v < evi; v++)
		{
			unsigned long long offset = 0;
			MPR_local_patch local_patch = file->variable[v]->local_patch;

			int patch_count = local_patch->patch_count; /* patch count per process */
			int bits = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

			local_patch->agg_patch_count = patch_count;

			local_patch->patch_id_array = malloc(patch_count*sizeof(int));
			local_patch->agg_patch_disps = malloc(patch_count*sizeof(unsigned long long));

			unsigned long long max_size = patch_count * file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2];
			local_patch->buffer = malloc(max_size * bits);
			for (int i = 0; i < patch_count; i++)
			{
				local_patch->patch_id_array[i] = local_patch->patch[i]->global_id;
				local_patch->agg_patch_disps[i] = offset;
				int buffer_size = local_patch->patch[i]->patch_buffer_size;
				memcpy(&local_patch->buffer[offset], local_patch->patch[i]->buffer, buffer_size);
				offset += buffer_size;
			}
			local_patch->buffer = (unsigned char*) realloc(local_patch->buffer, offset);
			local_patch->out_file_size = offset;
		}
	}
	else   /* Aggregation */
	{
		if (MPR_aggregation(file, svi, evi) != MPR_success)
		{
			fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
			return MPR_err_file;
		}
	}
	return MPR_success;
}


MPR_return_code MPR_aggregation(MPR_file file, int svi, int evi)
{
	int proc_num = file->comm->simulation_nprocs;  /* The number of processes */
	int rank = file->comm->simulation_rank; /* The rank of each process */
	MPI_Comm comm = file->comm->simulation_comm; /* The MPI communicator */

	int mode = file->mpr->aggregation_mode;  /* Aggregation Mode: 0 (fixed-size) 1 (fixed-patch-number) */
	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */
	int total_patch_num = file->mpr->total_patches_num; /* The number of total patches */
	int node_num = file->mpr->node_num; /* the number of nodes */

	int max_pcount = total_patch_num / proc_num + 1;

	for (int v  = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int patch_count = local_patch->patch_count; /* the number of patches per process */

		int local_patch_size_id_rank[max_pcount * 3];
		memset(local_patch_size_id_rank, -1, max_pcount * 3 * sizeof(int));
		for (int i = 0; i < patch_count; i++)
		{
			local_patch_size_id_rank[i * 3] = local_patch->patch[i]->global_id;
			local_patch_size_id_rank[i * 3 + 1] = local_patch->patch[i]->patch_buffer_size;
			local_patch_size_id_rank[i * 3 + 2] = rank;
		}

		int* patch_size_id = malloc(max_pcount * proc_num * 3 * sizeof(int));
		MPI_Allgather(local_patch_size_id_rank, max_pcount * 3, MPI_INT, patch_size_id, max_pcount * 3, MPI_INT, comm);

		int* patch_sizes = malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the size of patch i */
		int* patch_ranks = malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the owned rank of patch i */
		for (int i = 0; i < max_pcount * proc_num; i++)
		{
			int id = patch_size_id[i*3];
			if (id > -1)
			{
				patch_sizes[id] = patch_size_id[i*3 + 1];
				patch_ranks[id] = patch_size_id[i*3 + 2];
			}
		}
		free(patch_size_id);

		unsigned long long total_size = 0; /* The total size of all the patches across all the processes */
		for (int i = 0; i < total_patch_num; i++)
			total_size += patch_sizes[i];
		long double average_file_size = (double)total_size / out_file_num; /* The idea average file size*/

		/* Calculate the patch count in each dimension, and its next power 2 value (e.g., 3x3x3 -> 4x4x4)*/
		int patch_count_xyz[MPR_MAX_DIMENSIONS];
		int next_2_power_xyz[MPR_MAX_DIMENSIONS];
		for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
		{
			patch_count_xyz[i] = ceil((float)file->mpr->global_box[i] / file->mpr->patch_box[i]);
			next_2_power_xyz[i] = pow(2, ceil(log2(patch_count_xyz[i])));
		}

		/* Reorder the patch size array and id array with z-order curve */
		int patch_count_power2 = next_2_power_xyz[0] * next_2_power_xyz[1] * next_2_power_xyz[2];
		int* patch_sizes_zorder = (int*)malloc(patch_count_power2 * sizeof(int)); /* patch size with z-order */
		memset(patch_sizes_zorder, 0, patch_count_power2 * sizeof(int));
		int* patch_ids_zorder = (int*)malloc(patch_count_power2 * sizeof(int));  /* patch id with z-order */
		memset(patch_ids_zorder, -1, patch_count_power2 * sizeof(int));
		for (unsigned int z = 0; z < patch_count_xyz[2]; z++)
		{
			for (unsigned int y = 0; y < patch_count_xyz[1]; y++)
			{
				for (unsigned int x = 0; x < patch_count_xyz[0]; x++)
				{
					unsigned int zorder = calZOrder(x, y, z);  /* Calculate the index with z-order */
					unsigned int index = z * patch_count_xyz[1] * patch_count_xyz[0] + y * patch_count_xyz[0] + x;
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

		unsigned long long agg_sizes[out_file_num]; /* the current size of aggregators */
		memset(agg_sizes, 0, out_file_num * sizeof(unsigned long long));

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
		/**********************************************************************/

		unsigned long long agg_size = 0;
		for (int i = 0; i < out_file_num; i++)
		{
			if (rank == agg_ranks[i])
				agg_size = agg_sizes[i];
		}

		local_patch->patch_id_array = malloc(recv_num * sizeof(int));
		local_patch->agg_patch_disps = malloc(recv_num * sizeof(unsigned long long));

		/********************** Point-to-point communication **********************/
		local_patch->buffer = malloc(agg_size); /* reuse the local buffer per variable */
		local_patch->out_file_size = agg_size;

		int comm_count = patch_count + recv_num; /* the maximum transform times */
		MPI_Request req[comm_count];
		MPI_Status stat[comm_count];
		int req_id = 0;
		unsigned long long offset = 0;

		/* Send data */
		for (int i = 0; i < patch_count; i++)
		{
			int id = local_patch->patch[i]->global_id;
			int buffer_size = local_patch->patch[i]->patch_buffer_size;
			MPI_Isend(local_patch->patch[i]->buffer, buffer_size, MPI_BYTE, patch_assign_array[id], id, comm, &req[req_id]);
			req_id++;
		}

		/* Recv data */
		for (int i = 0; i < recv_num; i++)
		{
			MPI_Irecv(&local_patch->buffer[offset], patch_sizes[recv_array[i]], MPI_BYTE, patch_ranks[recv_array[i]], recv_array[i], comm, &req[req_id]);
			local_patch->patch_id_array[i] = recv_array[i];
			local_patch->agg_patch_disps[i] = offset;
			offset += patch_sizes[recv_array[i]];
			req_id++;
		}
		MPI_Waitall(req_id, req, stat);
		free(patch_ranks);
		free(patch_sizes);
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


static unsigned int calZOrder(unsigned int x, unsigned int y, unsigned int z)
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

	unsigned int result = x | (y << 1) | (z << 2);
	return result;
}

static int calculate_agg_num_with_node(MPR_file file)
{
	int total_patch_num = file->mpr->total_patches_num; /* The number of total patches */
	int node_num = file->mpr->node_num; /* the number of nodes */
	int rank = file->comm->simulation_rank; /* The rank of each process */
	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */

	int avg_patches_per_agg = total_patch_num / out_file_num; /* the average number of patches per aggregator */
	int reminder = total_patch_num % out_file_num; /* reminder (extra patches ) */
	int agg_patch_num = 0;
	if (file->mpr->is_aggregator == 1)
	{
		if (node_num == 1) /* If there is just one node */
			agg_patch_num = (rank < reminder)? (avg_patches_per_agg + 1): avg_patches_per_agg;
		else /* If there are multiple nodes */
		{
			int avg_rem = reminder / node_num; /* the average number of extra patch per node */
			int rem_assign_array[node_num]; /* array: the number of extra patches */
			rem_assign_array[node_num - 1] = (avg_rem < file->mpr->proc_num_last_node)? avg_rem : file->mpr->proc_num_last_node;
			reminder -= rem_assign_array[node_num - 1];
			avg_rem = reminder / (node_num - 1);
			for (int i = 0; i < (node_num - 1); i++)
				rem_assign_array[i] = (i < (reminder % (node_num - 1)))? (avg_rem + 1): avg_rem;
			/* Calculate the number of patches per aggregator */
			int id = rank / file->mpr->proc_num_per_node;
			int rem_agg_patch = rank % file->mpr->proc_num_per_node;
			agg_patch_num = (rem_agg_patch < rem_assign_array[id])?  (avg_patches_per_agg + 1): avg_patches_per_agg;
		}
	}
	return agg_patch_num;
}
