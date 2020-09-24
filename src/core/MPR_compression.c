
//#include <zfp.h>
#include <zfp.h>
#include "../MPR_inc.h"

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

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		char* type_name = file->variable[v]->type_name;
		int local_patch_count = local_patch->patch_count;
		int bits = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		for (int p = 0; p < local_patch_count; p++)
		{
			MPR_patch reg_patch = local_patch->patch[p];
			MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
			memset(output, 0, sizeof (*output)); /* Initialization */

			MPR_compress_3D_data(reg_patch->buffer, patch_x, patch_y, patch_z, 0, 10, type_name, &output);
			reg_patch->patch_buffer_size = output->compress_size;
			printf("%d: %d, %d\n", rank, reg_patch->global_id, output->compress_size);
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
	if (flag == 0)
		zfp_stream_set_accuracy(zfp, param);
	else if (flag == 1)
		zfp_stream_set_precision(zfp, param);
	else
	{
		printf("ERROR: O means accuracy, and 1 means precision\n");
	}
	size_t max_compressed_bytes = zfp_stream_maximum_size(zfp, field);
	// ZFP pointer structure
	(*output)->p = (unsigned char*) malloc(max_compressed_bytes);
	bitstream* stream = stream_open((*output)->p, max_compressed_bytes);
	zfp_stream_set_bit_stream(zfp, stream);
	size_t compressed_bytes = zfp_compress(zfp, field);
	(*output)->compress_size = compressed_bytes; // Data size after compression
	if (compressed_bytes == 0)
		puts("ERROR: Something wrong happened during compression\n");
	zfp_field_free(field);
	zfp_stream_close(zfp);
	stream_close(stream);
	return MPR_success;
}
