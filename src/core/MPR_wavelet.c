/*
 * MPR_wavelet.c
 *
 *  Created on: Oct 3, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_WAVELET_C_
#define SRC_CORE_MPR_WAVELET_C_

#include "../MPR_inc.h"

static void wavelet_transform(unsigned char* buffer, int* patch_box, char* type_name, int trans_num);
static void wavelet_helper(unsigned char* buf, int step, int flag, int data_type, int* patch_box, int mode);
static void MPR_wavelet_organization(unsigned char* buf, unsigned char* reg_buf, int* patch_box, int trans_num, int bytes, int mode, int end_level);
static void MPR_wavelet_reorg_helper(unsigned char* buf, unsigned char* reg_buf, int step, int* index, int sk, int sj, int si, int* patch_box, int bytes, int mode);

static void wavelet_decode_transform(unsigned char* buffer, int* patch_box, char* type_name, int trans_num, int end_level);

MPR_return_code MPR_wavelet_transform_perform(MPR_file file, int svi, int evi)
{
//	Events e("wave", "null");

	int rank = file->comm->simulation_rank; /* The rank of process */
	int procs_num = file->comm->simulation_nprocs; /* The number of processes */
	MPI_Comm comm = file->comm->simulation_comm; /* MPI Communicator */

	int min = file->mpr->patch_box[0]; /* Get the minimum size of all dimensions */
	for (int i = 1; i < MPR_MAX_DIMENSIONS; i++)
	{
		if (file->mpr->patch_box[i] < min)
			min = file->mpr->patch_box[i];
	}
	int trans_num = log2(min) - 2; /* Calculate the the number of transforms */
	file->mpr->wavelet_trans_num = trans_num;

//	file->time->wave_trans_time = 0;
//	file->time->wave_org_time = 0;
	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int patch_count = local_patch->patch_count; /* The number of patches per process */

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		for (int i = 0; i < patch_count; i++)
		{
//			double trans_start = MPI_Wtime();
//			{
//				Events e("trans", "comp", 0, 2, i);
			double start = MPI_Wtime();
			CALI_MARK_BEGIN("trans");
			double end = MPI_Wtime();
			cali_cost += (end - start);

			wavelet_transform(local_patch->patch[i]->buffer, file->mpr->patch_box, file->variable[v]->type_name, trans_num);

			start = MPI_Wtime();
			CALI_MARK_END("trans");
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			double trans_end = MPI_Wtime();
//			file->time->wave_trans_time += trans_end - trans_start;

//			double org_start = MPI_Wtime();
//			{
//				Events e("organ", "comp", 0, 2, i);
			start = MPI_Wtime();
			CALI_MARK_BEGIN("organ");
			end = MPI_Wtime();
			cali_cost += (end - start);

			unsigned char* reg_buffer = (unsigned char*)malloc(local_patch->patch[i]->patch_buffer_size);
			MPR_wavelet_organization(local_patch->patch[i]->buffer, reg_buffer, file->mpr->patch_box, trans_num, bytes, 0, 0);
			memcpy(local_patch->patch[i]->buffer, reg_buffer, local_patch->patch[i]->patch_buffer_size);
			free(reg_buffer);

			start = MPI_Wtime();
			CALI_MARK_END("organ");
			end = MPI_Wtime();
			cali_cost += (end - start);
//			}
//			double org_end = MPI_Wtime();
//			file->time->wave_org_time += org_end - org_start;
		}
	}

	return MPR_success;
}


/* Decode wavalet transform */
MPR_return_code MPR_wavelet_decode_perform(MPR_file file, int svi)
{
	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */

	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */
	int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2] * bytes;

	for (int i = 0; i < local_patch->patch_count; i++)
	{
		unsigned char* reg_buffer = (unsigned char*)malloc(patch_size);
		MPR_wavelet_organization(local_patch->patch[i]->buffer, reg_buffer, file->mpr->patch_box, file->mpr->wavelet_trans_num, bytes, 1, file->mpr->read_level);
		memcpy(local_patch->patch[i]->buffer, reg_buffer, patch_size);
		free(reg_buffer);
		wavelet_decode_transform(local_patch->patch[i]->buffer, file->mpr->patch_box, file->variable[svi]->type_name, file->mpr->wavelet_trans_num, file->mpr->read_level);
	}
	return MPR_success;
}

// Wavelet transform
static void wavelet_transform(unsigned char* buffer, int* patch_box, char* type_name, int trans_num)
{
	int data_type;
	if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
		data_type = 0;
	else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
		data_type = 1;
	else
	{
		printf("ERROR: Unsupported data type %s\n", type_name);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	for (int i = 0; i < trans_num; i++)
	{
		int step = pow(2, i+1);
		// Calculate x-dir
		wavelet_helper(buffer, step, 0, data_type, patch_box, 0);
		// Calculate y-dir
		wavelet_helper(buffer, step, 1, data_type, patch_box, 0);
		// Calculate z-dir
		wavelet_helper(buffer, step, 2, data_type, patch_box, 0);
	}
}

// Wavelet transform
static void wavelet_decode_transform(unsigned char* buffer, int* patch_box, char* type_name, int trans_num, int end_level)
{
	int data_type;
	if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
		data_type = 0;
	else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
		data_type = 1;

	for (int i = trans_num; i > end_level; i--)
	{
		int step = pow(2, i);
		// Calculate z-dir
		wavelet_helper(buffer, step, 2, data_type, patch_box, 1);
		// Calculate y-dir
		wavelet_helper(buffer, step, 1, data_type, patch_box, 1);
		// Calculate x-dir
		wavelet_helper(buffer, step, 0, data_type, patch_box, 1);
	}
}


// A wavelet helper
static void wavelet_helper(unsigned char* buf, int step, int flag, int data_type, int* patch_box, int mode)
{
	int ng_step = step/2;
	int si = ng_step, sj = ng_step, sk = ng_step;

	// Define the start position based on orientations (x, y, z)
	if (flag == 0)
	  si = step;
	if (flag == 1)
	  sj = step;
	if (flag == 2)
	  sk = step;

	int neighbor_ind = 0;

	// data types
	float f_data = 0;
	float f_neigb = 0;
	double d_data = 0;
	double d_neigb = 0;

	for (int k = 0; k < patch_box[2]; k+=sk)
	{
		for (int j = 0; j < patch_box[1]; j+=sj)
		{
			for (int i = 0; i < patch_box[0]; i+=si)
			{
				int index = k * patch_box[1] * patch_box[0] + j * patch_box[0] + i;

				// Define the neighbor position based on orientations (x, y, z)
				if (flag == 0)
				  neighbor_ind = index + ng_step;
				if (flag == 1)
				  neighbor_ind = index + ng_step * patch_box[0];
				if (flag == 2)
				  neighbor_ind = index + ng_step * patch_box[1] * patch_box[0];

				if (data_type == 0)
				{
					// Covert unsigned char to float
					memcpy(&f_data, &buf[index * 4], 4);
					memcpy(&f_neigb, &buf[neighbor_ind * 4], 4);
					// Calculate wavelet coefficients
					float avg = 0;
					float dif = 0;
					if (mode == 0)
					{
						avg = (f_data + f_neigb) / 2.0;
						dif = avg - f_neigb;
					}
					if (mode == 1)
					{
						avg = f_data + f_neigb;
						dif = f_data - f_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * 4], &avg, 4);
					memcpy(&buf[neighbor_ind * 4], &dif, 4);
				}
				else if (data_type == 1)
				{
					// Covert unsigned char to double
					memcpy(&d_data, &buf[index * 8], 8);
					memcpy(&d_neigb, &buf[neighbor_ind * 8], 8);
					// Calculate wavelet coefficients
					double avg = 0;
					double dif = 0;
					if (mode == 0)
					{
						avg = (d_data + d_neigb) / 2.0;
						dif = avg - d_neigb;
					}
					if (mode == 1)
					{
						avg = d_data + d_neigb;
						dif = d_data - d_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * 8], &avg, 8);
					memcpy(&buf[neighbor_ind * 8], &dif, 8);
				}
			}
		}
	}
}


static void MPR_wavelet_organization(unsigned char* buf, unsigned char* reg_buf, int* patch_box, int trans_num, int bytes, int mode, int end_level)
{
	int index = 0;

	int step = pow(2, trans_num);
	MPR_wavelet_reorg_helper(buf, reg_buf, step, &index, 0, 0, 0, patch_box, bytes, mode);

	for (int i = trans_num; i > end_level; i--)
	{
		step = pow(2, i);
		int n_step[2] = {0, step/2};
		// Define the start points for HHL, HLL, HLH ...
		for(int k = 0; k < 2; k++)
		{
			for(int j = 0; j < 2; j++)
			{
				for(int i = 0; i < 2; i++)
				{
					if (i == 0 && j == 0 && k == 0)
						continue;
					else
						MPR_wavelet_reorg_helper(buf, reg_buf, step, &index, n_step[k], n_step[j], n_step[i], patch_box, bytes, mode);
				}
			}
		}
	}
}

// A organization helper for wavelet inplace buffer
static void MPR_wavelet_reorg_helper(unsigned char* buf, unsigned char* reg_buf, int step, int* index, int sk, int sj, int si, int* patch_box, int bytes, int mode)
{
	for (int k = sk; k < patch_box[2]; k += step)
	{
		for (int j = sj; j < patch_box[1]; j += step)
		{
			for (int i = si; i < patch_box[0]; i += step)
			{
				int position = k * patch_box[1] * patch_box[0] + j * patch_box[0] + i;
				if (mode == 0) /* write mode */
					memcpy(&reg_buf[(*index) * bytes], &buf[position * bytes], bytes);
				else if (mode == 1) /* read mode*/
					memcpy(&reg_buf[position * bytes], &buf[(*index) * bytes], bytes);
				(*index)++;
			}
		}
	}
}


#endif /* SRC_CORE_MPR_WAVELET_C_ */
