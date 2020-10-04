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
static void wavelet_helper(unsigned char* buf, int step, int ng_step, int flag, int bytes, int* patch_box, char* type_name);


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
	int trans_num = log2(min); /* Calculate the the number of transforms */

//	if (rank == 0)
//	{
//		for (int j = 0; j < 64; j++)
//		{
//			float a;
//			memcpy(&a, &file->variable[0]->local_patch->patch[0]->buffer[j*sizeof(float)], sizeof(float));
//			printf("%f\n", a);
//		}
//	}

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch;
		int patch_count = local_patch->patch_count; /* The number of patches per process */

		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		for (int i = 0; i < patch_count; i++)
		{
			wavelet_transform(local_patch->patch[i]->buffer, file->mpr->patch_box, bytes, file->variable[v]->type_name, trans_num);
		}

	}


	printf("Wavelet transform start\n");
	return MPR_success;
}


// Wavelet transform
static void wavelet_transform(unsigned char* buffer, int* patch_box, int bytes, char* type_name, int trans_num)
{
  for (int i = 1; i < (trans_num + 1); i++)
  {
    int step = pow(2, i);
    int ng_step = step/2;

    // Calculate x-dir
    wavelet_helper(buffer, step, ng_step, 0, bytes, patch_box, type_name);
    // Calculate y-dir
    wavelet_helper(buffer, step, ng_step, 1, bytes, patch_box, type_name);
    // Calculate z-dir
    wavelet_helper(buffer, step, ng_step, 2, bytes, patch_box, type_name);
  }
}


// A wavelet helper
static void wavelet_helper(unsigned char* buf, int step, int ng_step, int flag, int bytes, int* patch_box, char* type_name)
{
  int si = ng_step; int sj = ng_step; int sk = ng_step;

  // Define the start position based on orientations (x, y, z)
  if (flag == 0)
      sj = step;
  if (flag == 1)
      si = step;
  if (flag == 2)
      sk = step;

  int neighbor_ind = 1;

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
	for (int i = 0; i < patch_box[1]; i+=si)
	{
	  for (int j = 0; j < patch_box[0]; j+=sj)
	  {
	    int index = k * patch_box[1] * patch_box[0] + i * patch_box[0] + j;
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
	      buf[index] = (c_data + c_neigb)/2;
	      buf[neighbor_ind] = buf[index] - c_neigb;
	    }
	    if (strcmp(type_name, MPR_DType.INT16) == 0 || strcmp(type_name, MPR_DType.INT16_GA) == 0 || strcmp(type_name, MPR_DType.INT16_RGB) == 0)
	    {
	      // Covert unsigned char to short
	      memcpy(&s_data, &buf[index * sizeof(short)], sizeof(short));
	      memcpy(&s_neigb, &buf[neighbor_ind * sizeof(float)], sizeof(short));
	      // Calculate wavelet coefficients
	      short avg = (s_data + s_neigb) / 2;
	      short dif = avg - s_neigb;
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
	      int avg = (i_data + i_neigb) / 2;
	      int dif = avg - i_neigb;
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
	      float avg = (f_data + f_neigb) / 2.0;
	      float dif = avg - f_neigb;
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
	      double avg = (d_data + d_neigb) / 2.0;
	      double dif = avg - d_neigb;
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
	      int64_t avg = (i64_data + i64_neigb) / 2.0;
	      int64_t dif = avg - i64_neigb;
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
	      uint64_t avg = (u64i_data + u64i_neigb) / 2.0;
	      uint64_t dif = avg - u64i_neigb;
	      // Replace buffer
	      memcpy(&buf[index * sizeof(uint64_t)], &avg, sizeof(uint64_t));
	      memcpy(&buf[neighbor_ind * sizeof(uint64_t)], &dif, sizeof(uint64_t));
	    }
	  }
	}
  }
}

#endif /* SRC_CORE_MPR_WAVELET_C_ */
