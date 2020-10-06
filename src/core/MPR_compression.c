
//#include <zfp.h>
#include <zfp.h>
#include "../MPR_inc.h"

static void calculate_res_level_box(int* res_box, int* patch_box, int level);

MPR_return_code MPR_ZFP_multi_res_compression_perform(MPR_file file, int svi, int evi)
{
	file->mpr->compression_type = 1; /* compression type (1: accuracy, 2: precision)*/
	file->mpr->compression_param = 0; /* compression parameter */

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		char* type_name = file->variable[v]->type_name;
		int local_patch_count = local_patch->patch_count;
		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		for (int p = 0; p < local_patch_count; p++)
		{
			int res_box[MPR_MAX_DIMENSIONS]; /* resolution box per each level */
			calculate_res_level_box(res_box, file->mpr->patch_box, file->mpr->wavelet_trans_num);

			int offset = 0; /* the offset for each sub-band */
			int comp_offset = 0;  /* the offset for each sun-band after compressed (should be smaller then offset)*/

			MPR_patch reg_patch = local_patch->patch[p];
			MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
			memset(output, 0, sizeof (*output)); /* Initialization */

			reg_patch->subbands_comp_size = malloc(reg_patch->subband_num * sizeof(int));
			int sid = 0;

			// Compressed DC component
			int size = res_box[0] * res_box[1] * res_box[2]; /* size of resolution box per level */
			unsigned char* dc_buf = malloc(size * bytes); /* buffer for DC component */
			memcpy(dc_buf, &reg_patch->buffer[offset], size * bytes);
			MPR_compress_3D_data(dc_buf, res_box[0], res_box[1], res_box[2], file->mpr->compression_type, file->mpr->compression_param, type_name, &output); /* ZFP Compression */
			free(dc_buf);
			memcpy(&reg_patch->buffer[comp_offset], output->p, output->compress_size);
			free(output->p);
			comp_offset += output->compress_size;
			offset += size * bytes;
			reg_patch->subbands_comp_size[sid++] = output->compress_size;

			for (int i = file->mpr->wavelet_trans_num; i > 0; i--)
			{
				calculate_res_level_box(res_box, file->mpr->patch_box, i);
				size = res_box[0] * res_box[1] * res_box[2];
				unsigned char* sub_buf = malloc(size * bytes);
				for (int j = 0; j < 7; j++) /* compresses 7 sub-bands for each level*/
				{
					memcpy(sub_buf, &reg_patch->buffer[offset], size * bytes); /* buffer for each sub-band*/
					offset += size * bytes;
					MPR_compress_3D_data(sub_buf, res_box[0], res_box[1], res_box[2], file->mpr->compression_type, file->mpr->compression_param, type_name, &output); /* ZFP Compression */
					memcpy(&reg_patch->buffer[comp_offset], output->p, output->compress_size); /* copy the compresses buffer to patch buffer */
					free(output->p); /* free compresses buffer */
					comp_offset += output->compress_size;
					reg_patch->subbands_comp_size[sid++] = output->compress_size;
				}
				free(sub_buf);
			}
			reg_patch->buffer = realloc(reg_patch->buffer, comp_offset); /* changed the size of patch buffer */
			reg_patch->patch_buffer_size = comp_offset; /* the total compressed size per patch */
		}
	}
	return MPR_success;
}

static void calculate_res_level_box(int* res_box, int* patch_box, int level)
{
	for (int i = 0; i < MPR_MAX_DIMENSIONS; i++)
	{
		res_box[i] = patch_box[i]/pow(2, level);
	}
}

MPR_return_code MPR_ZFP_compression_perform(MPR_file file, int svi, int evi)
{
	int rank = file->comm->simulation_rank; /* The rank of process */
	int procs_num = file->comm->simulation_nprocs; /* The number of processes */
	MPI_Comm comm = file->comm->simulation_comm; /* MPI Communicator */

	/* Patch size */
	int patch_x = file->mpr->patch_box[0];
	int patch_y = file->mpr->patch_box[1];
	int patch_z = file->mpr->patch_box[2];

	int patch_size = patch_x * patch_y * patch_z;

	file->mpr->compression_type = 1; /* compression type (1: accuracy, 2: precision)*/
	file->mpr->compression_param = 0; /* compression parameter */

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		char* type_name = file->variable[v]->type_name;
		int local_patch_count = local_patch->patch_count;

		for (int p = 0; p < local_patch_count; p++)
		{
			MPR_patch reg_patch = local_patch->patch[p];
			MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
			memset(output, 0, sizeof (*output)); /* Initialization */

			MPR_compress_3D_data(reg_patch->buffer, patch_x, patch_y, patch_z, file->mpr->compression_type, file->mpr->compression_param, type_name, &output);
			reg_patch->patch_buffer_size = output->compress_size;
			free(reg_patch->buffer);
			reg_patch->buffer = (unsigned char*)malloc(output->compress_size);
			memcpy(reg_patch->buffer, output->p, output->compress_size);
			free(output->p);
		}
	}
	return MPR_success;
}

// ZFP float compression
MPR_return_code MPR_compress_3D_data(unsigned char* buf, int dim_x, int dim_y, int dim_z, int flag, float param, char* type_name, MPR_zfp_compress* output)
{

	// ZFP data type according to PIDX data type
	zfp_type type = zfp_type_none;
	if (strcmp(type_name, MPR_DType.INT32) == 0 || strcmp(type_name, MPR_DType.INT32_GA) == 0 || strcmp(type_name, MPR_DType.INT32_RGB) == 0)
		type = zfp_type_int32;
	else if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
		type = zfp_type_float;
	else if (strcmp(type_name, MPR_DType.INT64) == 0 || strcmp(type_name, MPR_DType.INT64_GA) == 0 || strcmp(type_name, MPR_DType.INT64_RGB) == 0)
		type = zfp_type_int64;
	else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
		type = zfp_type_double;
	else
		printf("ERROR: ZFP doesn't support this type %s\n", type_name);

	zfp_field* field = zfp_field_3d(buf, type, dim_x, dim_y, dim_z);
	zfp_stream* zfp = zfp_stream_open(NULL);
	// Two compression modes
	if (flag == 1)
		zfp_stream_set_accuracy(zfp, param);
	else if (flag == 2)
		zfp_stream_set_precision(zfp, param);
	else
	{
		printf("ERROR: 1 means accuracy, and 2 means precision\n");
	}
	int max_compressed_bytes = zfp_stream_maximum_size(zfp, field);
	// ZFP pointer structure
	(*output)->p = (unsigned char*) malloc(max_compressed_bytes);
	bitstream* stream = stream_open((*output)->p, max_compressed_bytes);
	zfp_stream_set_bit_stream(zfp, stream);
	int compressed_bytes = zfp_compress(zfp, field);
	(*output)->compress_size = compressed_bytes; // Data size after compression
	if (compressed_bytes == 0)
		puts("ERROR: Something wrong happened during compression\n");
	zfp_field_free(field);
	zfp_stream_close(zfp);
	stream_close(stream);
	return MPR_success;
}
