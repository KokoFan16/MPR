/*
 * MPR_wavelet.c
 *
 *  Created on: Oct 3, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_WAVELET_C_
#define SRC_CORE_MPR_WAVELET_C_

#include "../MPR_inc.h"

static void wavelet_transform(unsigned char* buffer, int* patch_box, int bytes, char* type_name, int trans_num);
static void wavelet_helper(unsigned char* buf, int step, int flag, int bytes, int* patch_box, char* type_name, int mode);
static void MPR_wavelet_organization(unsigned char* buf, unsigned char* reg_buf, int* patch_box, int trans_num, int bytes, int mode, int end_level);
static void MPR_wavelet_reorg_helper(unsigned char* buf, unsigned char* reg_buf, int step, int* index, int sk, int sj, int si, int* patch_box, int bytes, int mode);

static void wavelet_decode_transform(unsigned char* buffer, int* patch_box, int bytes, char* type_name, int trans_num, int end_level);

MPR_return_code MPR_wavelet_transform_perform(MPR_file file, int svi, int evi)
{
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

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int patch_count = local_patch->patch_count; /* The number of patches per process */

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		for (int i = 0; i < patch_count; i++)
		{
			wavelet_transform(local_patch->patch[i]->buffer, file->mpr->patch_box, bytes, file->variable[v]->type_name, trans_num);
			unsigned char* reg_buffer = malloc(local_patch->patch[i]->patch_buffer_size);
			MPR_wavelet_organization(local_patch->patch[i]->buffer, reg_buffer, file->mpr->patch_box, trans_num, bytes, 0, 0);
			memcpy(local_patch->patch[i]->buffer, reg_buffer, local_patch->patch[i]->patch_buffer_size);
			free(reg_buffer);
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
		unsigned char* reg_buffer = malloc(patch_size);
		MPR_wavelet_organization(local_patch->patch[i]->buffer, reg_buffer, file->mpr->patch_box, file->mpr->wavelet_trans_num, bytes, 1, file->mpr->read_level);
		memcpy(local_patch->patch[i]->buffer, reg_buffer, patch_size);
		free(reg_buffer);
		wavelet_decode_transform(local_patch->patch[i]->buffer, file->mpr->patch_box, bytes, file->variable[svi]->type_name, file->mpr->wavelet_trans_num, file->mpr->read_level);
	}
	return MPR_success;
}

// Wavelet transform
static void wavelet_transform(unsigned char* buffer, int* patch_box, int bytes, char* type_name, int trans_num)
{
	for (int i = 0; i < trans_num; i++)
	{
		int step = pow(2, i+1);
		// Calculate x-dir
		wavelet_helper(buffer, step, 0, bytes, patch_box, type_name, 0);
		// Calculate y-dir
		wavelet_helper(buffer, step, 1, bytes, patch_box, type_name, 0);
		// Calculate z-dir
		wavelet_helper(buffer, step, 2, bytes, patch_box, type_name, 0);
	}
}

// Wavelet transform
static void wavelet_decode_transform(unsigned char* buffer, int* patch_box, int bytes, char* type_name, int trans_num, int end_level)
{
	for (int i = trans_num; i > end_level; i--)
	{
		int step = pow(2, i);
		// Calculate z-dir
		wavelet_helper(buffer, step, 2, bytes, patch_box, type_name, 1);
		// Calculate y-dir
		wavelet_helper(buffer, step, 1, bytes, patch_box, type_name, 1);
		// Calculate x-dir
		wavelet_helper(buffer, step, 0, bytes, patch_box, type_name, 1);
	}
}


// A wavelet helper
static void wavelet_helper(unsigned char* buf, int step, int flag, int bytes, int* patch_box, char* type_name, int mode)
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
	unsigned char c_data = 0;
	unsigned char c_neigb = 0;
	short s_data = 0;
	short s_neigb = 0;
	float f_data = 0;
	float f_neigb = 0;
	double d_data = 0;
	double d_neigb = 0;
	int i_data = 0;
	int i_neigb = 0;
	uint64_t u64i_data = 0;
	uint64_t u64i_neigb = 0;
	int64_t i64_data = 0;
	int64_t i64_neigb = 0;

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

				if (strcmp(type_name, MPR_DType.UINT8) == 0 || strcmp(type_name, MPR_DType.UINT8_GA) == 0 || strcmp(type_name, MPR_DType.UINT8_RGB) == 0)
				{
					c_data = buf[index];
					c_neigb = buf[neighbor_ind];
					// Calculate wavelet coefficients and replace in the buffer
					if (mode == 0)
					{
						buf[index] = (c_data + c_neigb) / 2.0;
						buf[neighbor_ind] = buf[index] - c_neigb;
					}
					if (mode == 1)
					{
						buf[index] = c_data + c_neigb;
						buf[neighbor_ind] = c_data - c_neigb;
					}
				}
				if (strcmp(type_name, MPR_DType.INT16) == 0 || strcmp(type_name, MPR_DType.INT16_GA) == 0 || strcmp(type_name, MPR_DType.INT16_RGB) == 0)
				{
					// Covert unsigned char to short
					memcpy(&s_data, &buf[index * sizeof(short)], sizeof(short));
					memcpy(&s_neigb, &buf[neighbor_ind * sizeof(float)], sizeof(short));
					// Calculate wavelet coefficients
					short avg = 0;
					short dif = 0;
					if (mode == 0)
					{
						avg = (s_data + s_neigb) / 2.0;
						dif = avg - s_neigb;
					}
					if (mode == 1)
					{
						avg = s_data + s_neigb;
						dif = s_data - s_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * sizeof(short)], &avg, sizeof(short));
					memcpy(&buf[neighbor_ind * sizeof(short)], &dif, sizeof(short));
				}
				if (strcmp(type_name, MPR_DType.INT32) == 0 || strcmp(type_name, MPR_DType.INT32_GA) == 0 || strcmp(type_name, MPR_DType.INT32_RGB) == 0)
				{
					// Covert unsigned char to int
					memcpy(&i_data, &buf[index * sizeof(int)], sizeof(int));
					memcpy(&i_neigb, &buf[neighbor_ind * sizeof(int)], sizeof(int));
					// Calculate wavelet coefficients
					int avg = 0;
					int dif = 0;
					if (mode == 0)
					{
						avg = (i_data + i_neigb) / 2.0;
						dif = avg - i_neigb;
					}
					if (mode == 1)
					{
						avg = i_data + i_neigb;
						dif = i_data - i_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * sizeof(int)], &avg, sizeof(int));
					memcpy(&buf[neighbor_ind * sizeof(int)], &dif, sizeof(int));
				}
				else if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
				{
					// Covert unsigned char to float
					memcpy(&f_data, &buf[index * sizeof(float)], sizeof(float));
					memcpy(&f_neigb, &buf[neighbor_ind * sizeof(float)], sizeof(float));
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
					memcpy(&buf[index * sizeof(float)], &avg, sizeof(float));
					memcpy(&buf[neighbor_ind * sizeof(float)], &dif, sizeof(float));
				}
				else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
				{
					// Covert unsigned char to double
					memcpy(&d_data, &buf[index * sizeof(double)], sizeof(double));
					memcpy(&d_neigb, &buf[neighbor_ind * sizeof(double)], sizeof(double));
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
					memcpy(&buf[index * sizeof(double)], &avg, sizeof(double));
					memcpy(&buf[neighbor_ind * sizeof(double)], &dif, sizeof(double));
				}
				else if (strcmp(type_name, MPR_DType.INT64) == 0 || strcmp(type_name, MPR_DType.INT64_GA) == 0 || strcmp(type_name, MPR_DType.INT64_RGB) == 0)
				{
					// Covert unsigned char to int64_t
					memcpy(&i64_data, &buf[index * sizeof(int64_t)], sizeof(int64_t));
					memcpy(&i64_neigb, &buf[neighbor_ind * sizeof(int64_t)], sizeof(int64_t));
					// Calculate wavelet coefficients
					int64_t avg = 0;
					int64_t dif = 0;
					if (mode == 0)
					{
						avg = (i64_data + i64_neigb) / 2.0;
						dif = avg - i64_neigb;
					}
					if (mode == 1)
					{
						avg = i64_data + i64_neigb;
						dif = i64_data - i64_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * sizeof(int64_t)], &avg, sizeof(int64_t));
					memcpy(&buf[neighbor_ind * sizeof(int64_t)], &dif, sizeof(int64_t));
				}
				else if (strcmp(type_name, MPR_DType.UINT64) == 0 || strcmp(type_name, MPR_DType.UINT64_GA) == 0 || strcmp(type_name, MPR_DType.UINT64_RGB) == 0)
				{
					// Covert unsigned char to uint64_t
					memcpy(&u64i_data, &buf[index * sizeof(uint64_t)], sizeof(uint64_t));
					memcpy(&u64i_neigb, &buf[neighbor_ind * sizeof(uint64_t)], sizeof(uint64_t));
					// Calculate wavelet coefficients
					uint64_t avg = 0;
					uint64_t dif = 0;
					if (mode == 0)
					{
						avg = (u64i_data + u64i_neigb) / 2.0;
						dif = avg - u64i_neigb;
					}
					if (mode == 1)
					{
						avg = u64i_data + u64i_neigb;
						dif = u64i_data - u64i_neigb;
					}
					// Replace buffer
					memcpy(&buf[index * sizeof(uint64_t)], &avg, sizeof(uint64_t));
					memcpy(&buf[neighbor_ind * sizeof(uint64_t)], &dif, sizeof(uint64_t));
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
