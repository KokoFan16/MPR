/*
 * MPR_aggregation.c
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

static void decide_aggregator(MPR_file file);

MPR_return_code MPR_aggregation(MPR_file file, int svi, int evi)
{
	int proc_num = file->comm->simulation_nprocs;  /* The number of processes */
	int rank = file->comm->simulation_rank; /* The rank of each process */
	MPI_Comm comm = file->comm->simulation_comm; /* The MPI communicator */

	int mode = file->mpr->aggregation_mode;  /* Aggregation Mode: 0 (fixed-size) 1 (fixed-patch-number) */
	int out_file_num = file->mpr->out_file_num;  /* The number of out files(aggregators) */
	int total_patch_num = file->mpr->total_patches_num; /* The number of total patches */
	int node_num = file->mpr->node_num; /* the number of nodes */
	MPR_local_patch local_patch = file->variable[svi]->local_patch;
	int patch_count = local_patch->patch_count; /* the number of patches per process */

	int patch_count_array[proc_num]; /* Array: item i is the number of patches of process i */
	MPI_Allgather(&patch_count, 1, MPI_INT, patch_count_array, 1, MPI_INT, comm);

//	int local_patch_id_array[patch_count]; /* local array: the global id of patches per process */
//	for (int i = 0; i < patch_count; i++)
//		local_patch_id_array[i] = file->variable[svi]->local_patch->patch[i]->global_id;

	int displs[proc_num]; /* the displacement for patch_id_array */
	int start_dis = 0;
	for (int i = 0; i < proc_num; i++)
	{
		displs[i] = start_dis;
		start_dis += patch_count_array[i];
	}
//	int patch_id_array[total_patch_num]; /* the global id array of patches */
//	MPI_Allgatherv(local_patch_id_array, patch_count, MPI_INT, patch_id_array, patch_count_array, displs, MPI_INT, comm);

	int assign_patch_array[total_patch_num]; /* patch assignment array */
	memset(assign_patch_array, -1, total_patch_num * sizeof(int));

	decide_aggregator(file); /* Decide aggregators */
	int recv_count[proc_num]; /* the number of elements that are to be received from each process */
	MPI_Allgather(&file->mpr->is_aggregator, 1, MPI_INT, recv_count, 1, MPI_INT, comm);
	int agg_disp[proc_num]; /* Entry i specifies the displacement at which to place the incoming data from process i */
	int start_disp = 0;
	for (int i = 0; i < proc_num; i++)
	{
		agg_disp[i] = start_disp;
		start_disp += recv_count[i];
	}
	int agg_ranks[out_file_num]; /* Aggregators Array*/
	MPI_Allgatherv(&rank, 1, MPI_INT, agg_ranks, recv_count, agg_disp, MPI_INT, comm);

	if (mode == 0) /* fixed-size mode */
	{
		unsigned long long proc_size = 0; /* the size (bytes) per process */
		int local_patch_size_array[patch_count]; /* array: the size of patches per process */
		for (int i = 0; i < patch_count; i++)
		{
			local_patch_size_array[i] = local_patch->patch[i]->buffer_size;
			proc_size += local_patch->patch[i]->buffer_size;
		}
		unsigned long long total_size = 0; /* the total size of all the processes */
		MPI_Allreduce(&proc_size, &total_size, 1, MPI_LONG_LONG, MPI_SUM, comm);
		unsigned long long avg_size_per_agg = total_size / out_file_num; /* the average size per aggregator */
		avg_size_per_agg += total_size % out_file_num;

		int patch_size_array[total_patch_num]; /* array: the size of patches */
		MPI_Allgatherv(local_patch_size_array, patch_count, MPI_INT, patch_size_array, patch_count_array, displs, MPI_INT, comm);

		unsigned long long agg_size_array[out_file_num]; /* the current size of each aggregator */
		MPI_Allgatherv(&proc_size, 1, MPI_LONG_LONG, agg_size_array, recv_count, agg_disp, MPI_LONG_LONG, comm);

		/* Assign the patches which owned by aggregators to themselves */
		for (int i = 0; i < out_file_num; i++)
		{
			int id = agg_ranks[i];
			for (int j = 0; j < patch_count_array[id]; j++)
				assign_patch_array[j + displs[id]] = id;
		}
		/* Assign the patches which owned by non-aggregators to aggregators */
		for (int i = 0; i < out_file_num; i++)
		{
			for (int j = 0; j < total_patch_num; j++)
			{
				if (assign_patch_array[j] == -1 && (agg_size_array[i] + patch_size_array[j]) <  avg_size_per_agg)
				{
					assign_patch_array[j] = agg_ranks[i];
					agg_size_array[i] += patch_size_array[j];
				}
			}
		}
		/* Assign remaining patches */
		int min_rank = 0;
		int min_index = 0;
		for (int i = 0; i < total_patch_num; i++)
		{
			if (assign_patch_array[i] == -1)
			{
				unsigned long long min_size = total_size;
				for (int j = 0; j < out_file_num; j++)
				{
					if (agg_size_array[j] < min_size)
					{
						min_size = agg_size_array[j];
						min_rank = agg_ranks[j];
						min_index = j;
					}
				}
				assign_patch_array[i] = min_rank;
				agg_size_array[min_index] += min_size;
			}
		}
	}
	else if (mode == 1)  /* fixed-patch-number mode */
	{
		/* Calculate the number of patches per aggregator */
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

		int agg_patch_num_array[out_file_num]; /* array: the number of patches per aggregator */
		MPI_Allgatherv(&agg_patch_num, 1, MPI_INT, agg_patch_num_array, recv_count, agg_disp, MPI_INT, comm);
		int recv_count_array[out_file_num]; /* array: the received number of patches per aggregator */
		int id = 0;
		/* Assign the patches which owned by aggregators to themselves */
		for (int i = 0; i < out_file_num; i++)
		{
			id = agg_ranks[i];
			recv_count_array[i] = agg_patch_num_array[i] - patch_count_array[id];
			for (int j = 0; j < patch_count_array[id]; j++)
				assign_patch_array[j + displs[id]] = id;
		}
		/* Assign the patches which owned by non-aggregators to aggregators */
		id = 0;
		int count = 0;
		int dis = recv_count_array[id];
		for (int i = 0; i < total_patch_num; i++)
		{
			if (assign_patch_array[i] == -1) /* the patches that are not assigned */
			{
				if (count < dis)
				{
					assign_patch_array[i] = agg_ranks[id];
					count++;
				}
				else
				{
					id++;
					assign_patch_array[i] = agg_ranks[id];
					count++;
					dis += recv_count_array[id];
				}
			}
		}
	}
	else if (mode == 2)
	{
		printf("mode: increasing order mode\n");
	}
	else if (mode == 3)
	{
		printf("mode: spatial domain mode\n");
	}
	else
		return MPR_err_unsupported_flags;
//
//	if (rank == 0)
//	{
//		for (int i = 0; i < total_patch_num; i++)
//		{
//			printf("%d\n", assign_patch_array[i]);
//		}
//	}


	return MPR_success;
}

static void decide_aggregator(MPR_file file)
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
		int id = rank / file->mpr->proc_num_per_node;
		if (rank % file->mpr->proc_num_per_node < file_assign_per_node[id])
			file->mpr->is_aggregator = 1;
	}
}
