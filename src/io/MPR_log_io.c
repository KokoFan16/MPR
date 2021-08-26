/*
 * MPR_log_io.c
 *
 *  Created on: Jan 20, 2021
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_timing_logs(MPR_file file, int svi, int evi)
{
	int rank = file->comm->simulation_rank;
	int MODE = file->mpr->io_type;

	double total_time = file->time->total_end - file->time->total_start;
	double rst_time = file->time->rst_end - file->time->rst_start;
	double wave_time = file->time->wave_end - file->time->wave_start;
	double comp_time = file->time->zfp_end - file->time->zfp_start;

	if (file->flags == MPR_MODE_CREATE)
	{
		int time_count = 65;
		int offset = file->mpr->current_time_step * time_count;

		double agg_time = file->time->agg_end - file->time->agg_start;
		double io_time = file->time->wrt_data_end - file->time->wrt_data_start;

		time_buffer[offset] = file->mpr->current_time_step;
		time_buffer[offset + 1] = rank;
		time_buffer[offset + 2] = file->mpr->is_aggregator;
		time_buffer[offset + 3] = total_time;
		time_buffer[offset + 4] = file->time->crt_struc_end - file->time->crt_struc_start;
		time_buffer[offset + 5] = rst_time;
		time_buffer[offset + 6] = file->time->part_status_end - file->time->part_status_start;
		time_buffer[offset + 7] = file->time->part_gather_end - file->time->part_gather_start;
		time_buffer[offset + 8] = file->time->part_gather_basic_end - file->time->part_gather_basic_start;
		time_buffer[offset + 9] = file->time->part_gather_local_end - file->time->part_gather_local_start;
		time_buffer[offset + 10] = file->time->part_gather_comm_end - file->time->part_gather_comm_start;
		time_buffer[offset + 11] = file->time->part_cal_pcount_end - file->time->part_cal_pcount_start;
		time_buffer[offset + 12] = file->time->part_max_pshare_end - file->time->part_max_pshare_start;
		time_buffer[offset + 13] = file->time->part_assign_end - file->time->part_assign_start;
		time_buffer[offset + 14] = file->time->part_assign_mem_end - file->time->part_assign_mem_start;
		time_buffer[offset + 15] = file->time->part_assign_share_time;
		time_buffer[offset + 16] = file->time->part_assign_patch_time;
		time_buffer[offset + 17] = file->time->part_assign_update_time;
		time_buffer[offset + 18] = file->time->part_comm_end - file->time->part_comm_start;
		time_buffer[offset + 19] = file->time->part_comm_mem_end - file->time->part_comm_mem_start;
		time_buffer[offset + 20] = file->time->part_comm_recv_end - file->time->part_comm_recv_start;
		time_buffer[offset + 21] = file->time->part_comm_recv_pre_time;
		time_buffer[offset + 22] = file->time->part_comm_recv_calbox_time;
		time_buffer[offset + 23] = file->time->part_comm_recv_crtype_time;
		time_buffer[offset + 24] = file->time->part_comm_recv_req_time;
		time_buffer[offset + 25] = file->time->part_comm_send_end - file->time->part_comm_send_start;
		time_buffer[offset + 26] = file->time->part_comm_send_calbox_time;
		time_buffer[offset + 27] = file->time->part_comm_send_crtype_time;
		time_buffer[offset + 28] = file->time->part_comm_send_req_time;
		time_buffer[offset + 29] = file->time->part_comm_wait_end - file->time->part_comm_wait_start;
		time_buffer[offset + 30] = wave_time;
		time_buffer[offset + 31] = file->time->wave_pre_end - file->time->wave_pre_start;
		time_buffer[offset + 32] = file->time->wave_trans_time;
		time_buffer[offset + 33] = file->time->wave_org_time;
		time_buffer[offset + 34] = comp_time;
		time_buffer[offset + 35] = file->time->zfp_pre_end - file->time->zfp_pre_start;
		time_buffer[offset + 36] = file->time->zfp_comp_dc_time;
		time_buffer[offset + 37] = file->time->zfp_comp_bands_time;
		time_buffer[offset + 38] = agg_time;
		time_buffer[offset + 39] = file->time->agg_pre_end - file->time->agg_pre_start;
		time_buffer[offset + 40] = file->time->agg_gather_end - file->time->agg_gather_start;
		time_buffer[offset + 41] = file->time->agg_gather_local_end - file->time->agg_gather_local_start;
		time_buffer[offset + 42] = file->time->agg_gather_comm_end - file->time->agg_gather_comm_start;
		time_buffer[offset + 43] = file->time->agg_gather_filter_end - file->time->agg_gather_filter_start;
		time_buffer[offset + 44] = file->time->agg_calinfo_end - file->time->agg_calinfo_start;
		time_buffer[offset + 45] = file->time->agg_convert_z_end - file->time->agg_convert_z_start;
		time_buffer[offset + 46] = file->time->agg_assign_end - file->time->agg_assign_start;
		time_buffer[offset + 47] = file->time->agg_assign_act_end - file->time->agg_assign_act_start;
		time_buffer[offset + 48] = file->time->agg_assign_aggpik_end - file->time->agg_assign_aggpik_start;
		time_buffer[offset + 49] = file->time->agg_assign_calrecv_end - file->time->agg_assign_calrecv_start;
		time_buffer[offset + 50] = file->time->agg_comm_end - file->time->agg_comm_start;
		time_buffer[offset + 51] = file->time->agg_comm_pre_end - file->time->agg_comm_pre_start;
		time_buffer[offset + 52] = file->time->agg_comm_send_end - file->time->agg_comm_send_start;
		time_buffer[offset + 53] = file->time->agg_comm_recv_end - file->time->agg_comm_recv_start;
		time_buffer[offset + 54] = file->time->agg_comm_recv_act_time;
		time_buffer[offset + 55] = file->time->agg_comm_recv_bound_time;
		time_buffer[offset + 56] = file->time->agg_comm_recv_update_time;
		time_buffer[offset + 57] = file->time->agg_comm_wait_end - file->time->agg_comm_wait_start;
		time_buffer[offset + 58] = file->time->agg_clean_end - file->time->agg_clean_start;
		time_buffer[offset + 59] = io_time;
		time_buffer[offset + 60] = file->time->wrt_meta_end - file->time->wrt_meta_start;
		time_buffer[offset + 61] = file->time->wrt_meta_basic_end - file->time->wrt_meta_basic_start;
		time_buffer[offset + 62] = file->time->wrt_meta_bound_end - file->time->wrt_meta_bound_start;
		time_buffer[offset + 63] = file->time->wrt_meta_file_end - file->time->wrt_meta_file_start;
		time_buffer[offset + 64] = file->time->wrt_file_end - file->time->wrt_file_start;


		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
		{
			char* directory_path = malloc(512);
			memset(directory_path, 0, sizeof(*directory_path) * 512);
			strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

			char time_folder[512];
			sprintf(time_folder, "%s_times", directory_path);

			char time_log[512];
			sprintf(time_log, "%s/time_write_%s_log_%d", time_folder, directory_path, rank);
			free(directory_path);

			FILE* fp = fopen(time_log, "w"); /* open file */
			if (!fp) /* Check file handle */
				fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);

			for (int i = 0; i < file->mpr->last_tstep; i++)
			{
				fprintf(fp, "%d %d %d total-%f-none {crt_struc-%f-comp partition-%f-none {ck_stat-%f-comp gather-%f-none {basic-%f-comp local-%f-comp comm-%f-comm} "
						"cal_pcount-%f-comp max_pshare-%f-comp assign-%f-none {malloc-%f-none find_share-%f-comp apatch-%f-comp update-%f-comp} "
						"comm-%f-none {malloc-%f-none recv-%f-none {pre-%f-comp calbox-%f-comp crtype-%f-comm req-%f-comm} "
						"send-%f-none {calbox-%f-comp crtype-%f-comm req-%f-comm} wait-%f-comm}} "
						"wave-%f-none {pre-%f-comp trans-%f-comp org-%f-comp} "
						"comp-%f-none {pre-%f-comp dc-%f-comp subbands-%f-comp} "
						"agg-%f-none {pre-%f-comp, gather-%f-none {local-%f-comp comm-%f-comm, filter-%f-comm} calinfo-%f-comp convert_z-%f-comp "
						"assign-%f-none {act-%f-comp aggpic-%f-comp calrecv-%f-comp} comm-%f-none {pre-%f-comp send-%f-comm "
						"recv-%f-none {act-%f-comm calbound-%f-comp update-%f-none} wait-%f-comm} clean-%f-none} "
						" io-%f-none {meta-%f-none {basic-%f-io bound-%f-io file-%f-comp} file-%f-io}}\n",
						(int)time_buffer[i*time_count], (int)time_buffer[i*time_count+1], (int)time_buffer[i*time_count+2],
						time_buffer[i*time_count+3], time_buffer[i*time_count+4], time_buffer[i*time_count+5], time_buffer[i*time_count+6], time_buffer[i*time_count+7], time_buffer[i*time_count+8],
						time_buffer[i*time_count+9], time_buffer[i*time_count+10], time_buffer[i*time_count+11], time_buffer[i*time_count+12], time_buffer[i*time_count+13], time_buffer[i*time_count+14],
						time_buffer[i*time_count+15], time_buffer[i*time_count+16], time_buffer[i*time_count+17], time_buffer[i*time_count+18], time_buffer[i*time_count+19], time_buffer[i*time_count+20],
						time_buffer[i*time_count+21], time_buffer[i*time_count+22], time_buffer[i*time_count+23], time_buffer[i*time_count+24], time_buffer[i*time_count+25], time_buffer[i*time_count+26],
						time_buffer[i*time_count+27], time_buffer[i*time_count+28], time_buffer[i*time_count+29], time_buffer[i*time_count+30], time_buffer[i*time_count+31], time_buffer[i*time_count+32],
						time_buffer[i*time_count+33], time_buffer[i*time_count+34], time_buffer[i*time_count+35], time_buffer[i*time_count+36], time_buffer[i*time_count+37], time_buffer[i*time_count+38],
						time_buffer[i*time_count+39], time_buffer[i*time_count+40], time_buffer[i*time_count+41], time_buffer[i*time_count+42], time_buffer[i*time_count+43], time_buffer[i*time_count+44],
						time_buffer[i*time_count+45], time_buffer[i*time_count+46], time_buffer[i*time_count+47], time_buffer[i*time_count+48], time_buffer[i*time_count+49], time_buffer[i*time_count+50],
						time_buffer[i*time_count+51], time_buffer[i*time_count+52], time_buffer[i*time_count+53], time_buffer[i*time_count+54], time_buffer[i*time_count+55], time_buffer[i*time_count+56],
						time_buffer[i*time_count+57], time_buffer[i*time_count+58], time_buffer[i*time_count+59], time_buffer[i*time_count+60], time_buffer[i*time_count+61], time_buffer[i*time_count+62],
						time_buffer[i*time_count+63], time_buffer[i*time_count+64]);
			}

			fclose(fp);
		}


	}

	return MPR_success;
}

//MPR_return_code MPR_timing_logs(MPR_file file, int svi, int evi)
//{
//	int rank = file->comm->simulation_rank;
//	int MODE = file->mpr->io_type;
//
//	double total_time = file->time->total_end - file->time->total_start;
//	double rst_time = file->time->rst_end - file->time->rst_start;
//	double wave_time = file->time->wave_end - file->time->wave_start;
//	double comp_time = file->time->zfp_end - file->time->zfp_start;
//
//	if (file->flags == MPR_MODE_CREATE)
//	{
//		int time_count = 9;
//
//		double agg_time = file->time->agg_end - file->time->agg_start;
//		double wrt_data_time = file->time->wrt_data_end - file->time->wrt_data_start;
//
//		int offset = file->mpr->current_time_step * time_count;
//
//		if (file->mpr->is_aggregator == 1)
//		{
//			time_buffer[offset] = file->mpr->is_aggregator;
//			time_buffer[offset + 1] = file->mpr->current_time_step;
//			time_buffer[offset + 2] = rank;
//			time_buffer[offset + 3] = total_time;
//			time_buffer[offset + 4] = rst_time;
//			time_buffer[offset + 5] = wave_time;
//			time_buffer[offset + 6] = comp_time;
//			time_buffer[offset + 7] = agg_time;
//			time_buffer[offset + 8] = wrt_data_time;
//		}
//
//		double max_time = 0;
//		MPI_Allreduce(&total_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, file->comm->simulation_comm);
//		if (total_time == max_time)
//			printf("%d %d: [%f] >= [p %f w %f c %f a %f o %f]\n", file->mpr->current_time_step, rank, total_time, rst_time, wave_time, comp_time, agg_time, wrt_data_time);
//
//		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
//		{
//			int count = file->mpr->last_tstep * time_count;
//			float* total_time_buffer = NULL;
//			if (rank == 0)
//				total_time_buffer = malloc(count*sizeof(float)*file->comm->simulation_nprocs);
//
//			MPI_Gather(time_buffer, count, MPI_FLOAT, total_time_buffer, count, MPI_FLOAT, 0, file->comm->simulation_comm);
//
//			if (rank == 0)
//			{
//				char* directory_path = malloc(512);
//				memset(directory_path, 0, sizeof(*directory_path) * 512);
//				strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);
//
//				char time_log[512];
//				sprintf(time_log, "time_write_%s_log", directory_path);
//				free(directory_path);
//
//				FILE* fp = fopen(time_log, "w"); /* open file */
//				if (!fp) /* Check file handle */
//					fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);
//
//				int loop_count = file->comm->simulation_nprocs * file->mpr->last_tstep;
//				for (int i = 0; i < loop_count; i++)
//				{
//					if (total_time_buffer[i*time_count] == 1)
//						fprintf(fp, "%d %d: [%f] >= [p %f w %f c %f a %f o %f]\n", (int)total_time_buffer[i*time_count+1], (int)total_time_buffer[i*time_count+2], total_time_buffer[i*time_count+3], total_time_buffer[i*time_count+4], total_time_buffer[i*time_count+5], total_time_buffer[i*time_count+6], total_time_buffer[i*time_count+7], total_time_buffer[i*time_count+8]);
//				}
//
//				fclose(fp);
//			}
//			free(total_time_buffer);
//		}
//	}
//
//
//	if (file->flags == MPR_MODE_RDONLY)
//	{
////		time_buffer = malloc(6*sizeof(float));
//
//		double read_time = file->time->read_end - file->time->read_start;
//
//		int count = 7;
//		int offset = file->read_ite * count;
//
//
//		time_buffer[offset] = file->read_ite;
//		time_buffer[offset + 1] = rank;
//		time_buffer[offset + 2] = total_time;
//		time_buffer[offset + 3] = read_time;
//		time_buffer[offset + 4] = comp_time;
//		time_buffer[offset + 5] = wave_time;
//		time_buffer[offset + 6] = rst_time;
//
////		time_buffer[0] = rank;
////		time_buffer[1] = total_time;
////		time_buffer[2] = read_time;
////		time_buffer[3] = comp_time;
////		time_buffer[4] = wave_time;
////		time_buffer[5] = rst_time;
//
//		if (file->read_ite == 9)
//		{
//			float* total_time_buffer = NULL;
//			if (file->comm->simulation_rank == 0)
//				total_time_buffer = malloc(count*10*sizeof(float)*file->comm->simulation_nprocs);
//
//			MPI_Gather(time_buffer, count*10, MPI_FLOAT, total_time_buffer, count*10, MPI_FLOAT, 0, file->comm->simulation_comm);
//
//			if (rank == 0)
//			{
//				char time_log[512];
//				sprintf(time_log, "time_read_%s_log", file->outfile);
////				sprintf(time_log, "time_read_%s_log", file->mpr->filename);
//
//				FILE* fp = fopen(time_log, "w"); /* open file */
//				if (!fp) /* Check file handle */
//					fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);
//
//				int loop_count = file->comm->simulation_nprocs * 10;
//
//				for (int i = 0; i < loop_count; i++)
//					fprintf(fp,"%d %d: [%f] >= [read %f comp %f wave %f rst %f]\n", (int)total_time_buffer[i*count+0], (int)total_time_buffer[i*count+1], total_time_buffer[i*count+2], total_time_buffer[i*count+3], total_time_buffer[i*count+4], total_time_buffer[i*count+5], total_time_buffer[i*count+6]);
//
//				fclose(fp);
//			}
//
//			free(total_time_buffer);
////			free(time_buffer);
//		}
//	}
//
//	return MPR_success;
//}



MPR_return_code MPR_logs(MPR_file file, int svi, int evi)
{
	int rank = file->comm->simulation_rank;

	if (file->flags == MPR_MODE_CREATE)
	{
		if (file->mpr->current_time_step == (file->mpr->last_tstep -1))
		{
			int log_count = 5;
			MPR_local_patch local_patch = file->variable[0]->local_patch;

			int offset = file->mpr->current_time_step * log_count;

			if (file->mpr->is_aggregator == 1)
			{
				size_buffer[offset] = file->mpr->is_aggregator;
				size_buffer[offset + 1] = file->mpr->current_time_step;
				size_buffer[offset + 2] = rank;
				size_buffer[offset + 3] = local_patch->agg_patch_count;
				size_buffer[offset + 4] = local_patch->out_file_size;
			}

			int count = file->mpr->last_tstep * log_count;
			long long int* total_size_buffer = NULL;
			if (rank == 0)
				total_size_buffer = malloc(count*sizeof(long long int)*file->comm->simulation_nprocs);

			MPI_Gather(size_buffer, count, MPI_LONG_LONG_INT, total_size_buffer, count, MPI_LONG_LONG_INT, 0, file->comm->simulation_comm);

			if (rank == 0)
			{
				char* directory_path = malloc(512);
				memset(directory_path, 0, sizeof(*directory_path) * 512);
				strncpy(directory_path, file->mpr->filename, strlen(file->mpr->filename) - 4);

				char size_log[512];
				sprintf(size_log, "size_%s_log", directory_path);
				free(directory_path);

				FILE* fp = fopen(size_log, "w"); /* open file */
				if (!fp) /* Check file handle */
					fprintf(stderr, " [%s] [%d] mpr_dir is corrupt.\n", __FILE__, __LINE__);

				int loop_count = file->comm->simulation_nprocs * file->mpr->last_tstep;
				for (int i = 0; i < loop_count; i++)
				{
					if (total_size_buffer[i*log_count] == 1)
						fprintf(fp, "%lld %lld %lld %lld\n", total_size_buffer[i*log_count+1], total_size_buffer[i*log_count+2], total_size_buffer[i*log_count+3], total_size_buffer[i*log_count+4]);
				}

				fclose(fp);
			}
			free(total_size_buffer);
		}
	}

	return MPR_success;
}
