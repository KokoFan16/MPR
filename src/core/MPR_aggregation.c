/*
 * MPR_aggregation.c
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

static int calZOrder(int x, int y, int z);

MPR_return_code MPR_aggregation_perform(MPR_file file, int svi, int evi)
{
	int proc_num = file->comm->simulation_nprocs;  /* The number of processes */
	int rank = file->comm->simulation_rank; /* The rank of each process */
	MPI_Comm comm = file->comm->simulation_comm; /* The MPI communicator */
	int total_patch_num = file->mpr->total_patches_num; /* The number of total patches */

	int max_pcount = total_patch_num / proc_num;
	if (total_patch_num % proc_num > 0) max_pcount += 1;

	if (file->mpr->agg_version == 0) // patch-level
	{
		for (int v  = svi; v < evi; v++)
		{
//			file->time->agg_gather_start = MPI_Wtime();

			MPR_local_patch local_patch = file->variable[v]->local_patch;
			int patch_count = local_patch->patch_count; /* the number of patches per process */
			int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

			/*************************** Gather required information *************************/
			double gather_start = MPI_Wtime();
			int* global_subband_sizes = NULL;
			int* local_subband_sizes = NULL;
			int* subband_sizes = NULL;
			int subbands_num = 0;

			int *patch_size_id, *patch_sizes, *patch_ranks;

			double start = MPI_Wtime();
			CALI_MARK_BEGIN("gather");
			call_count += 1;
			double end = MPI_Wtime();
			cali_cost += (end - start);
//			{
//				Events e("gather", "comm");

			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
			{
				subbands_num = file->mpr->wavelet_trans_num * 7 + 1;
				global_subband_sizes = (int*)malloc(max_pcount * proc_num * subbands_num * sizeof(int));
				local_subband_sizes = (int*)malloc(max_pcount * subbands_num * sizeof(int));
				subband_sizes = (int*)malloc(total_patch_num * subbands_num * sizeof(int));
			}
			local_patch->proc_size = 0;
			int local_patch_size_id_rank[max_pcount * 3]; /* local information: size, id, own_rank per patch */
			memset(local_patch_size_id_rank, -1, max_pcount * 3 * sizeof(int));
			for (int i = 0; i < patch_count; i++)
			{
				local_patch_size_id_rank[i * 3] = local_patch->patch[i]->global_id;
				local_patch_size_id_rank[i * 3 + 1] = local_patch->patch[i]->patch_buffer_size;
				local_patch_size_id_rank[i * 3 + 2] = rank;
				if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
					memcpy(&local_subband_sizes[i*subbands_num], local_patch->patch[i]->subbands_comp_size, subbands_num*sizeof(int));
				local_patch->proc_size += local_patch->patch[i]->patch_buffer_size; /* print only */
			}

			patch_size_id = (int*)malloc(max_pcount * proc_num * 3 * sizeof(int));
			MPI_Allgather(local_patch_size_id_rank, max_pcount * 3, MPI_INT, patch_size_id, max_pcount * 3, MPI_INT, comm);

			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				MPI_Allgather(local_subband_sizes, max_pcount * subbands_num, MPI_INT, global_subband_sizes, max_pcount * subbands_num, MPI_INT, comm);
			free(local_subband_sizes);

			patch_sizes = (int*)malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the size of patch i */
			patch_ranks = (int*)malloc(total_patch_num * sizeof(int)); 	/* A array in which element i is the owned rank of patch i */
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

//			}

			start = MPI_Wtime();
			CALI_MARK_END("gather");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			file->time->agg_gather_end = MPI_Wtime();

			/******************************************************************************/
//			file->time->agg_calinfo_start = MPI_Wtime();

			long long int total_size = 0; /* The total size of all the patches across all the processes */
			int patch_count_xyz[MPR_MAX_DIMENSIONS]; /* patch count in each dimension */

			start = MPI_Wtime();
			CALI_MARK_BEGIN("calInfo");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			{
//				Events e("calInfo", "comp");
			for (int i = 0; i < total_patch_num; i++)
				total_size += patch_sizes[i];
			local_patch->compression_ratio = total_size / bytes;

			for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
			{
				patch_count_xyz[i] = ceil((float)file->mpr->global_box[i] / file->mpr->patch_box[i]);
				local_patch->compression_ratio /= file->mpr->global_box[i];
			}

			start = MPI_Wtime();
			CALI_MARK_END("calInfo");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			file->time->agg_calinfo_end = MPI_Wtime();

			/****************** Convert to z-order ********************/

			start = MPI_Wtime();
			CALI_MARK_BEGIN("cvrZ");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

//			file->time->agg_convert_z_start = MPI_Wtime();
			int patch_count_power2 = 0;  /* z-order count */
			int* patch_sizes_zorder = NULL;
			int* patch_ids_zorder = NULL;

			int next_2_power_xyz[MPR_MAX_DIMENSIONS]; /* (e.g., 3x3x3 -> 4x4x4)*/
			int max_d = 0;

//			{
//				Events e("cvrZ", "comp");

			for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
			{
				if (patch_count_xyz[i] > max_d)
					max_d = patch_count_xyz[i];
			}
			patch_count_power2 = pow(pow(2, ceil(log2(max_d))), 3); /* 27 -> 64 */
			/* Reorder the patch id array with z-order curve */
			patch_sizes_zorder = (int*)malloc(patch_count_power2 * sizeof(int)); /* patch size with z-order */
			memset(patch_sizes_zorder, 0, patch_count_power2 * sizeof(int));
			patch_ids_zorder = (int*)malloc(patch_count_power2 * sizeof(int));  /* patch id with z-order */
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

			start = MPI_Wtime();
			CALI_MARK_END("cvrZ");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			file->time->agg_convert_z_end = MPI_Wtime();
			/**********************************************************/

			/************************* Assign patches **************************/
//			file->time->agg_assign_start = MPI_Wtime();

			int patch_assign_array[total_patch_num];
			memset(patch_assign_array, -1, total_patch_num * sizeof(int));
			long long int agg_size = 0; /* the size of aggregator */
			long long int agg_sizes[file->mpr->out_file_num]; /* the current size of aggregators */
			memset(agg_sizes, 0, file->mpr->out_file_num * sizeof(long long int));
			int cur_agg_count = 0;

			start = MPI_Wtime();
			CALI_MARK_BEGIN("assign");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

//			{
//				Events e("assign", "comp");

			if (file->mpr->is_fixed_file_size == 0) /* fixed number of patches per file mode */
			{
				int avg_patch_num = ceil((float)total_patch_num / file->mpr->out_file_num); /* average patches count per file */

				int pcount = 0;
				for (int i = 0; i < patch_count_power2; i++)
				{
					if (patch_ids_zorder[i] > -1)
					{
						if (pcount == ((cur_agg_count + 1) * avg_patch_num))
							cur_agg_count++;
						patch_assign_array[patch_ids_zorder[i]] = cur_agg_count;
						agg_sizes[cur_agg_count] += patch_sizes_zorder[i];
						pcount++;
					}
				}

			}
			else
			{
				long long int average_file_size = total_size / file->mpr->out_file_num; /* The idea average file size*/
				int pcount = 0;

				while (pcount < patch_count_power2 && cur_agg_count < file->mpr->out_file_num)
				{
					if (patch_ids_zorder[pcount] > -1)
					{
						if (agg_sizes[cur_agg_count] >= average_file_size)
						{
							total_size -= agg_sizes[cur_agg_count];
							cur_agg_count++;
							average_file_size = total_size / (file->mpr->out_file_num - cur_agg_count);
						}
						patch_assign_array[patch_ids_zorder[pcount]] = cur_agg_count;
						agg_sizes[cur_agg_count] += patch_sizes_zorder[pcount];
					}
					pcount++;
				}

			}
			start = MPI_Wtime();
			CALI_MARK_END("assign");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			file->time->agg_assign_end = MPI_Wtime();

//			file->time->agg_comm_pre_start = MPI_Wtime();

			start = MPI_Wtime();
			CALI_MARK_BEGIN("commPre");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);


			int recv_array[total_patch_num]; /* Local receive array per process */
			int recv_num = 0;  /* number of received number of patches per aggregator */
			int agg_ranks[file->mpr->out_file_num]; /* AGG Array */
			int gap = proc_num / file->mpr->out_file_num;
			int cagg = 0;
//			{
//				Events e("commPre", "comp");
			file->mpr->out_file_num = cur_agg_count + 1;
			for (int i = 0; i < proc_num; i+= gap)
			{
				if (cagg < file->mpr->out_file_num)
				{
					agg_ranks[cagg++] = i;
					if (rank == i)
						file->mpr->is_aggregator = 1;
				}
				else
					break;
			}

			for (int i = 0; i < total_patch_num; i++)
			{
				if (rank == agg_ranks[patch_assign_array[i]])
					recv_array[recv_num++] = i;
			}
			local_patch->agg_patch_count = recv_num;

			/**********************************************************************/
			/* calculate total size per aggregator */
			for (int i = 0; i < file->mpr->out_file_num; i++)
			{
				if (rank == agg_ranks[i])
					agg_size = agg_sizes[i];
			}

			local_patch->agg_patch_id_array = (int*)malloc(recv_num * sizeof(int));
			local_patch->agg_patch_disps = (int*)malloc(recv_num * sizeof(int));
			local_patch->agg_patch_size = (int*)malloc(recv_num * sizeof(int));
			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				local_patch->agg_subbands_size = (int*)malloc(recv_num * subbands_num * sizeof(int));

			start = MPI_Wtime();
			CALI_MARK_END("commPre");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

//			}
//			file->time->agg_comm_pre_end = MPI_Wtime();

			/********************** Point-to-point communication **********************/
//			file->time->agg_comm_send_start = MPI_Wtime();
			start = MPI_Wtime();
			CALI_MARK_BEGIN("send");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

			local_patch->buffer = (unsigned char*)malloc(agg_size); /* reuse the local buffer per variable */
			local_patch->out_file_size = agg_size;

			int comm_count = patch_count + recv_num; /* the maximum transform times */
			MPI_Request* req = (MPI_Request*)malloc(comm_count * sizeof(MPI_Request));
			MPI_Status* stat = (MPI_Status*)malloc(comm_count * sizeof(MPI_Status));
			int req_id = 0;
			int offset = 0;

//			{
//				Events e("send", "comm");
			for (int i = 0; i < patch_count; i++)
			{
				int id = local_patch->patch[i]->global_id;
				int buffer_size = local_patch->patch[i]->patch_buffer_size;
				MPI_Isend(local_patch->patch[i]->buffer, buffer_size, MPI_BYTE, agg_ranks[patch_assign_array[id]], id, comm, &req[req_id]);
				req_id++;
			}
			start = MPI_Wtime();
			CALI_MARK_END("send");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			file->time->agg_comm_send_end = MPI_Wtime();

//			file->time->agg_comm_recv_start = MPI_Wtime();

			start = MPI_Wtime();
			CALI_MARK_BEGIN("recv");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

			/* Recv data */
			int max_xyz[MPR_MAX_DIMENSIONS] = {0, 0, 0};
			int min_xyz[MPR_MAX_DIMENSIONS] = {INT_MAX, INT_MAX, INT_MAX};

			file->time->agg_comm_recv_act_time = 0;
			file->time->agg_comm_recv_bound_time = 0;

//			{
//				Events e("recv", "null");

			for (int i = 0; i < recv_num; i++)
			{
				start = MPI_Wtime();
				CALI_MARK_BEGIN("calBound");
				call_count += 1;
				end = MPI_Wtime();
				cali_cost += (end - start);
//				{
//					Events e("calBound", "comp", 0, 2, i);
				int z = recv_array[i] / (patch_count_xyz[0] * patch_count_xyz[1]);
				int y = (recv_array[i] - (z * patch_count_xyz[0] * patch_count_xyz[1])) / patch_count_xyz[0];
				int x = recv_array[i] - z * patch_count_xyz[0] * patch_count_xyz[1] - y * patch_count_xyz[0];
				if (z < min_xyz[2]) min_xyz[2] = z;
				if (z > max_xyz[2]) max_xyz[2] = z;
				if (y < min_xyz[1]) min_xyz[1] = y;
				if (y > max_xyz[1]) max_xyz[1] = y;
				if (x < min_xyz[0]) min_xyz[0] = x;
				if (x > max_xyz[0]) max_xyz[0] = x;
//				}

				start = MPI_Wtime();
				CALI_MARK_END("calBound");
				call_count += 1;
				end = MPI_Wtime();
				cali_cost += (end - start);
//				double recv_bound_end = MPI_Wtime();
//				if (rank == 0) { printf("%d, %d, %f\n", rank, i, (recv_bound_end - recv_bound_start)); }
//				file->time->agg_comm_recv_bound_time += recv_bound_end - recv_bound_start;

				start = MPI_Wtime();
				CALI_MARK_BEGIN("comm");
				call_count += 1;
				end = MPI_Wtime();
				cali_cost += (end - start);

//				double recv_act_start = MPI_Wtime();
//				{
//					Events e("comm", "comm", 0, 2, i);
				MPI_Irecv(&local_patch->buffer[offset], patch_sizes[recv_array[i]], MPI_BYTE, patch_ranks[recv_array[i]], recv_array[i], comm, &req[req_id]);

				local_patch->agg_patch_id_array[i] = recv_array[i];
				local_patch->agg_patch_disps[i] = offset;
				local_patch->agg_patch_size[i] = patch_sizes[recv_array[i]];

				if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
					memcpy(&local_patch->agg_subbands_size[i*subbands_num], &subband_sizes[recv_array[i]*subbands_num], subbands_num*sizeof(int));
				offset += patch_sizes[recv_array[i]];

				req_id++;

				start = MPI_Wtime();
				CALI_MARK_END("comm");
				call_count += 1;
				end = MPI_Wtime();
				cali_cost += (end - start);
//				double recv_act_end = MPI_Wtime();
//				file->time->agg_comm_recv_act_time += recv_act_end - recv_act_start;
//				}
			}
			start = MPI_Wtime();
			CALI_MARK_END("recv");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			file->time->agg_comm_recv_end = MPI_Wtime();

//			file->time->agg_comm_wait_start = MPI_Wtime();
//			{
//				Events e("wait", "comm");

			start = MPI_Wtime();
			CALI_MARK_BEGIN("wait");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

			MPI_Waitall(req_id, req, stat);

			start = MPI_Wtime();
			CALI_MARK_END("wait");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

//			file->time->agg_comm_wait_end = MPI_Wtime();

			free(req);
			free(stat);
			free(patch_ranks);
			free(patch_sizes);
			free(subband_sizes);
//			}

//			file->time->agg_bound_start = MPI_Wtime();
//			{
//				Events e("boundBox", "null");

			start = MPI_Wtime();
			CALI_MARK_BEGIN("boundBox");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);

			for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
			{
				local_patch->bounding_box[i] = min_xyz[i];
				local_patch->bounding_box[i + MPR_MAX_DIMENSIONS] = max_xyz[i] + 1;
			}

			start = MPI_Wtime();
			CALI_MARK_END("boundBox");
			call_count += 1;
			end = MPI_Wtime();
			cali_cost += (end - start);
//			file->time->agg_bound_end = MPI_Wtime();
//			}


//			if (file->mpr->is_aggregator == 1)
//				printf("Aggregation %d: total %f [ gather %f z %f assign %f comm %f ] \n", rank, (total_end - total_start), (gather_end - gather_start), (convert_z_end - convert_z_start),
//						(assign_end - assign_start), (comm_end - comm_start));
		}
	}
	else // process-level
	{
		for (int v  = svi; v < evi; v++)
		{
			double gather_start = MPI_Wtime();
			MPR_local_patch local_patch = file->variable[v]->local_patch;
			int patch_count = local_patch->patch_count; /* the number of patches per process */

			int* global_subband_sizes = NULL;
			int* local_subband_sizes = NULL;
			int* subband_sizes = NULL;
			int subbands_num = 0;
			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
			{
				subbands_num = file->mpr->wavelet_trans_num * 7 + 1;
				global_subband_sizes = (int*)malloc(max_pcount * proc_num * subbands_num * sizeof(int));
				local_subband_sizes = (int*)malloc(max_pcount * subbands_num * sizeof(int));
				subband_sizes = (int*)malloc(total_patch_num * subbands_num * sizeof(int));
			}

			int proc_size = 0;
			int local_patch_ids[max_pcount];
			for (int i = 0; i < patch_count; i++)
			{
				proc_size += local_patch->patch[i]->patch_buffer_size;
				local_patch_ids[i] = local_patch->patch[i]->global_id;
				if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
					memcpy(&local_subband_sizes[i*subbands_num], local_patch->patch[i]->subbands_comp_size, subbands_num*sizeof(int));
			}

			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				MPI_Allgather(local_subband_sizes, max_pcount * subbands_num, MPI_INT, global_subband_sizes, max_pcount * subbands_num, MPI_INT, comm);

			int procs_sizes[proc_num];
			MPI_Allgather(&proc_size, 1, MPI_INT, procs_sizes, 1, MPI_INT, comm);

			int procs_pcounts[proc_num];
			MPI_Allgather(&patch_count, 1, MPI_INT, procs_pcounts, 1, MPI_INT, comm);

			int* global_patch_ids = (int*)malloc(max_pcount*proc_num*sizeof(int));
			MPI_Allgather(&local_patch_ids, max_pcount, MPI_INT, global_patch_ids, max_pcount, MPI_INT, comm);

			free(global_patch_ids);
			free(global_subband_sizes);
			free(local_subband_sizes);
			free(subband_sizes);

			double gather_end = MPI_Wtime();
			double gather_time = gather_end - gather_start;

			double assign_start = MPI_Wtime();
			long long int total_size = 0;
			for (int i = 0; i < proc_num; i++)
				total_size += procs_sizes[i];

			int agg_sizes[file->mpr->out_file_num];
			memset(agg_sizes, 0, file->mpr->out_file_num*sizeof(int));

			int agg_ranks[file->mpr->out_file_num];
			agg_ranks[0] = 0;
			if (rank == 0)
				file->mpr->is_aggregator = 1;

			int pocs_assign_array[proc_num];
			memset(pocs_assign_array, 0, proc_num*sizeof(int));

			if (file->mpr->is_fixed_file_size == 0) /* fixed number of processes per file mode */
			{
				int avg_count = ceil((float)proc_num/file->mpr->out_file_num);
				for (int i = 0; i < proc_num; i++)
				{
					pocs_assign_array[i] = i / avg_count;
					agg_sizes[i / avg_count] += procs_sizes[i];
				}
			}
			else
			{
				int pcount = 0;
				int cur_agg_count = 0;
				int avg_size = total_size / file->mpr->out_file_num;
				while (pcount < proc_num && cur_agg_count < file->mpr->out_file_num)
				{
					if (agg_sizes[cur_agg_count] >= avg_size)
					{
						total_size -= agg_sizes[cur_agg_count];
						cur_agg_count++;
						avg_size = total_size / (file->mpr->out_file_num - cur_agg_count);
						agg_ranks[cur_agg_count] = pcount;
						if (rank == pcount)
							file->mpr->is_aggregator = 1;
					}
					pocs_assign_array[pcount] = cur_agg_count;
					agg_sizes[cur_agg_count] += procs_sizes[pcount];
					pcount++;
				}
			}

			double assign_end = MPI_Wtime();
			double assign_time = assign_end - assign_start;

			double flat_start = MPI_Wtime();
			unsigned char* flat_buffer = (unsigned char*)malloc(proc_size);
			int poffset = 0;
			for (int p = 0; p < patch_count; p++)
			{
				memcpy(&flat_buffer[poffset], local_patch->patch[p]->buffer,local_patch->patch[p]->patch_buffer_size);
				poffset += local_patch->patch[p]->patch_buffer_size;
			}
			double flat_end = MPI_Wtime();
			double flat_time = flat_end - flat_start;

			double cre_comm_start = MPI_Wtime();
			MPI_Comm agg_comm;
			MPI_Comm_split(comm, pocs_assign_array[rank], rank, &agg_comm);
			double cre_comm_end = MPI_Wtime();
			double cre_comm_time = cre_comm_end - cre_comm_start;

			double pre_start = MPI_Wtime();
			int recv_ranks[proc_num];
			int recv_sizes[proc_num];
			int recv_displs[proc_num];
			int recv_count = 0;
			int roffset = 0;
			int recv_pcount = 0;
			for (int i = 0; i < proc_num; i++)
			{
				if (rank == agg_ranks[pocs_assign_array[i]])
				{
					recv_ranks[recv_count] = i;
					recv_sizes[recv_count] = procs_sizes[i];
					recv_displs[recv_count] = roffset;
					roffset += procs_sizes[i];
					recv_pcount += procs_pcounts[i];
					recv_count++;
				}
			}

			int agg_size = 0;
			for (int i = 0; i < file->mpr->out_file_num; i++)
			{
				if (rank == agg_ranks[i])
					agg_size = agg_sizes[i];
			}
			double pre_end = MPI_Wtime();
			double pre_time = pre_end - pre_start;

			double comm_start = MPI_Wtime();
			local_patch->buffer = (unsigned char*)malloc(agg_size); /* reuse the local buffer per variable */
			local_patch->out_file_size = agg_size;
			MPI_Gatherv(flat_buffer, proc_size, MPI_BYTE, local_patch->buffer, recv_sizes, recv_displs, MPI_BYTE, 0, agg_comm);

			free(flat_buffer);
			MPI_Comm_free(&agg_comm);
			double comm_end = MPI_Wtime();
			double comm_time = comm_end - comm_start;

			local_patch->agg_patch_id_array = (int*)malloc(recv_pcount * sizeof(int));
			local_patch->agg_patch_disps = (int*)malloc(recv_pcount * sizeof(int));
			local_patch->agg_patch_size = (int*)malloc(recv_pcount * sizeof(int));
			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				local_patch->agg_subbands_size = (int*)malloc(recv_pcount * subbands_num * sizeof(int));

			for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
			{
				local_patch->bounding_box[i] = file->mpr->patch_box[i];
				local_patch->bounding_box[i + MPR_MAX_DIMENSIONS] = file->mpr->local_box[i] + 1;
			}

			if (file->mpr->is_aggregator == 1)
				printf("Aggregation %d: [ gather %f assign %f flat %f cre_comm %f pre %f comm %f ] \n", rank, gather_time, assign_time, flat_time, cre_comm_time, pre_time,
					comm_time);
		}
	}
	return MPR_success;
}


/* Do not perform aggregation, just copy information */
MPR_return_code MPR_no_aggregation(MPR_file file, int svi, int evi)
{
	file->mpr->is_aggregator = 1;
	file->mpr->out_file_num = file->comm->simulation_nprocs;

	for (int v  = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;

		local_patch->agg_patch_count = local_patch->patch_count;

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		int local_size = 0;
		for (int i = 0; i < local_patch->patch_count; i++)
			local_size += local_patch->patch[i]->patch_buffer_size;
		local_patch->out_file_size = local_size;
//		local_patch->proc_size = local_size;

		local_patch->buffer = (unsigned char*)malloc(local_size);
		local_patch->agg_patch_id_array = (int*)malloc(local_patch->patch_count * sizeof(int));
		local_patch->agg_patch_disps = (int*)malloc(local_patch->patch_count * sizeof(int));
		local_patch->agg_patch_size = (int*)malloc(local_patch->patch_count * sizeof(int));

		local_patch->agg_subbands_size = NULL;
		int subbands_num = 0;
		if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
		{
			subbands_num = file->mpr->wavelet_trans_num * 7 + 1;
			local_patch->agg_subbands_size = (int*)malloc(local_patch->patch_count * subbands_num * sizeof(int));
		}

		int patch_count_xyz[MPR_MAX_DIMENSIONS]; /* patch count in each dimension */
		for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
			patch_count_xyz[i] = ceil((float)file->mpr->global_box[i] / file->mpr->patch_box[i]);


		int max_xyz[MPR_MAX_DIMENSIONS] = {0, 0, 0};
		int min_xyz[MPR_MAX_DIMENSIONS] = {INT_MAX, INT_MAX, INT_MAX};

		int offset = 0;
		for (int i = 0; i < local_patch->patch_count; i++)
		{
			int pid = local_patch->patch[i]->global_id;
			int z = pid / (patch_count_xyz[0] * patch_count_xyz[1]);
			int y = (pid - (z * patch_count_xyz[0] * patch_count_xyz[1])) / patch_count_xyz[0];
			int x = pid - z * patch_count_xyz[0] * patch_count_xyz[1] - y * patch_count_xyz[0];
			if (z < min_xyz[2]) min_xyz[2] = z;
			if (z > max_xyz[2]) max_xyz[2] = z;
			if (y < min_xyz[1]) min_xyz[1] = y;
			if (y > max_xyz[1]) max_xyz[1] = y;
			if (x < min_xyz[0]) min_xyz[0] = x;
			if (x > max_xyz[0]) max_xyz[0] = x;

			local_patch->agg_patch_id_array[i] = pid;
			local_patch->agg_patch_disps[i] = offset;
			local_patch->agg_patch_size[i] = local_patch->patch[i]->patch_buffer_size;
			memcpy(&local_patch->buffer[offset], local_patch->patch[i]->buffer, local_patch->patch[i]->patch_buffer_size);
			offset += local_patch->patch[i]->patch_buffer_size;
			if (file->mpr->io_type == MPR_MUL_RES_PRE_IO)
				memcpy(&local_patch->agg_subbands_size[i*subbands_num], local_patch->patch[i]->subbands_comp_size, subbands_num*sizeof(int));
		}

		for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
		{
			local_patch->bounding_box[i] = min_xyz[i];
			local_patch->bounding_box[i + MPR_MAX_DIMENSIONS] = max_xyz[i] + 1;
		}
	}

	return MPR_success;
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
