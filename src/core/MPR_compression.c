
//#include <zfp.h>
#include <zfp.h>
#include "../MPR_inc.h"

static void calculate_res_level_box(int* res_box, int* patch_box, int level);

MPR_return_code MPR_ZFP_multi_res_compression_perform(MPR_file file, int svi, int evi)
{
	Events e("ZFP");

	int subband_num = file->mpr->wavelet_trans_num * 7 + 1;

	for (int v = svi; v < evi; v++)
	{
//		file->time->zfp_pre_start = MPI_Wtime();
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		char* type_name = file->variable[v]->type_name;
		int local_patch_count = local_patch->patch_count;
		int bytes = file->variable[v]->vps * file->variable[v]->bpv/8; /* bytes per data */

		int data_type;
		{
			Events e("getDT");
		if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
			data_type = 0;
		else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
			data_type = 1;
		}
//		file->time->zfp_pre_end = MPI_Wtime();

//		file->time->zfp_comp_dc_time = 0;
//		file->time->zfp_comp_bands_time = 0;

		for (int p = 0; p < local_patch_count; p++)
		{
			double comp_dc_start = MPI_Wtime();

			int res_box[MPR_MAX_DIMENSIONS]; /* resolution box per each level */
			calculate_res_level_box(res_box, file->mpr->patch_box, file->mpr->wavelet_trans_num);

			int offset = 0; /* the offset for each sub-band */
			int comp_offset = 0;  /* the offset for each sun-band after compressed (should be smaller then offset)*/
			int sid = 0;
			int size = res_box[0] * res_box[1] * res_box[2]; /* size of resolution box per level */

			MPR_patch reg_patch = local_patch->patch[p];
			MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));

			{
				Events e("calDC", "comp", 2, p);

			memset(output, 0, sizeof (*output)); /* Initialization */
			reg_patch->subbands_comp_size = (int*)malloc(subband_num * sizeof(int));

			// Compressed DC component
			unsigned char* dc_buf = (unsigned char*)malloc(size * bytes); /* buffer for DC component */
			memcpy(dc_buf, &reg_patch->buffer[offset], size * bytes);
			MPR_compress_3D_data(dc_buf, res_box[0], res_box[1], res_box[2], file->mpr->compression_type, file->mpr->compression_param, data_type, &output); /* ZFP Compression */
			free(dc_buf);
			memcpy(&reg_patch->buffer[comp_offset], output->p, output->compress_size);
			free(output->p);
			comp_offset += output->compress_size;
			offset += size * bytes;
			reg_patch->subbands_comp_size[sid++] = output->compress_size;

			}

//			double comp_dc_end = MPI_Wtime();
//			file->time->zfp_comp_dc_time += comp_dc_end - comp_dc_start;

//			double comp_bands_start = MPI_Wtime();
			{
				Events e("calBands", "comp", 2, p);
			for (int i = file->mpr->wavelet_trans_num; i > 0; i--)
			{
				calculate_res_level_box(res_box, file->mpr->patch_box, i);
				size = res_box[0] * res_box[1] * res_box[2];
				unsigned char* sub_buf = (unsigned char*)malloc(size * bytes);
				for (int j = 0; j < 7; j++) /* compresses 7 sub-bands for each level*/
				{
					memcpy(sub_buf, &reg_patch->buffer[offset], size * bytes); /* buffer for each sub-band*/
					offset += size * bytes;
					MPR_compress_3D_data(sub_buf, res_box[0], res_box[1], res_box[2], file->mpr->compression_type, file->mpr->compression_param, data_type, &output); /* ZFP Compression */
					memcpy(&reg_patch->buffer[comp_offset], output->p, output->compress_size); /* copy the compresses buffer to patch buffer */
					free(output->p); /* free compresses buffer */
					comp_offset += output->compress_size;
					reg_patch->subbands_comp_size[sid++] = output->compress_size;
				}
				free(sub_buf);
			}
			reg_patch->buffer = (unsigned char*)realloc(reg_patch->buffer, comp_offset); /* changed the size of patch buffer */
			reg_patch->patch_buffer_size = comp_offset; /* the total compressed size per patch */
			free(output);
			}
//			double comp_bands_end = MPI_Wtime();
//			file->time->zfp_comp_bands_time += comp_bands_end - comp_bands_start;
		}

	}
	return MPR_success;
}


MPR_return_code MPR_ZFP_multi_res_decompression_perform(MPR_file file, int svi)
{
	int subband_num = file->mpr->wavelet_trans_num * 7 + 1;

	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */

	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */

	int read_box[MPR_MAX_DIMENSIONS];
	calculate_res_level_box(read_box, file->mpr->patch_box, file->mpr->read_level);
	int read_size = read_box[0] * read_box[1] * read_box[2] * bytes;

	char* type_name = file->variable[svi]->type_name;

	int data_type;
	if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
		data_type = 0;
	else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
		data_type = 1;

	for (int p = 0; p < local_patch->patch_count; p++)
	{
		MPR_patch reg_patch = local_patch->patch[p];
		MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
		memset(output, 0, sizeof (*output)); /* Initialization */

		unsigned char* tmp_buffer = (unsigned char*)malloc(read_size);

		int offset = 0; /* the offset for each sub-band */
		int decomp_offset = 0;  /* the decompressed offset for each sub-band*/

		int res_box[MPR_MAX_DIMENSIONS]; /* resolution box per each level */
		calculate_res_level_box(res_box, file->mpr->patch_box, file->mpr->wavelet_trans_num);

		int dc_size = res_box[0] * res_box[1] * res_box[2] * bytes;  /* size of resolution box per level */
		output->p = (unsigned char*) malloc(dc_size);

		int dc_comp_size = reg_patch->subbands_comp_size[0];
		unsigned char* dc_buf = (unsigned char*)malloc(dc_comp_size); /* buffer for DC component */
		memcpy(dc_buf, &reg_patch->buffer[offset], dc_comp_size);
		MPR_decompress_3D_data(dc_buf, dc_comp_size, res_box[0], res_box[1], res_box[2],
				file->mpr->compression_type, file->mpr->compression_param, data_type, &output);
		memcpy(&tmp_buffer[decomp_offset], output->p, dc_size);
		offset += dc_comp_size;
		decomp_offset += dc_size;
		free(output->p);
		free(dc_buf);

		for (int i = file->mpr->wavelet_trans_num; i > file->mpr->read_level; i--)
		{
			calculate_res_level_box(res_box, file->mpr->patch_box, i);
			int res_size = res_box[0] * res_box[1] * res_box[2] * bytes;
			output->p = (unsigned char*) malloc(res_size);

			int id = (file->mpr->wavelet_trans_num - i);
			for (int j = 0; j < 7; j++) /* compresses 7 sub-bands for each level*/
			{
				int sub_comp_size = reg_patch->subbands_comp_size[id * 7 + j + 1];
				unsigned char* sub_buf = (unsigned char*)malloc(sub_comp_size); /* buffer for DC component */
				memcpy(sub_buf, &reg_patch->buffer[offset], sub_comp_size);
				MPR_decompress_3D_data(sub_buf, sub_comp_size, res_box[0], res_box[1], res_box[2],
						file->mpr->compression_type, file->mpr->compression_param, data_type, &output);
				memcpy(&tmp_buffer[decomp_offset], output->p, res_size);
				offset += sub_comp_size;
				decomp_offset += res_size;
				free(sub_buf);
			}
			free(output->p);
		}

		int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2] * bytes;
		reg_patch->buffer = (unsigned char*)realloc(reg_patch->buffer, patch_size);
		memcpy(reg_patch->buffer, tmp_buffer, read_size);

		free(tmp_buffer);
		free(output);
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

	for (int v = svi; v < evi; v++)
	{
		MPR_local_patch local_patch = file->variable[v]->local_patch; /* Local patch pointer */
		char* type_name = file->variable[v]->type_name;
		int local_patch_count = local_patch->patch_count;

		int data_type;
		if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
			data_type = 0;
		else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
			data_type = 1;

		for (int p = 0; p < local_patch_count; p++)
		{
			MPR_patch reg_patch = local_patch->patch[p];
			MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
			memset(output, 0, sizeof (*output)); /* Initialization */

			MPR_compress_3D_data(reg_patch->buffer, patch_x, patch_y, patch_z, file->mpr->compression_type, file->mpr->compression_param, data_type, &output);
			reg_patch->patch_buffer_size = output->compress_size;
			free(reg_patch->buffer);
			reg_patch->buffer = (unsigned char*)malloc(output->compress_size);
			memcpy(reg_patch->buffer, output->p, output->compress_size);
			free(output->p);
		}
	}
	return MPR_success;
}

MPR_return_code MPR_ZFP_decompression_perform(MPR_file file, int svi)
{
	int rank = file->comm->simulation_rank; /* The rank of process */
	int procs_num = file->comm->simulation_nprocs; /* The number of processes */
	MPI_Comm comm = file->comm->simulation_comm; /* MPI Communicator */

	/* Patch size */
	int patch_x = file->mpr->patch_box[0];
	int patch_y = file->mpr->patch_box[1];
	int patch_z = file->mpr->patch_box[2];

	int bytes = file->variable[svi]->vps * file->variable[svi]->bpv/8; /* bytes per data */
	int patch_size = file->mpr->patch_box[0] * file->mpr->patch_box[1] * file->mpr->patch_box[2] * bytes;


	MPR_local_patch local_patch = file->variable[svi]->local_patch; /* Local patch pointer */
	char* type_name = file->variable[svi]->type_name;

	int data_type;
	if (strcmp(type_name, MPR_DType.FLOAT32) == 0 || strcmp(type_name, MPR_DType.FLOAT32_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT32_RGB) == 0)
		data_type = 0;
	else if (strcmp(type_name, MPR_DType.FLOAT64) == 0 || strcmp(type_name, MPR_DType.FLOAT64_GA) == 0 || strcmp(type_name, MPR_DType.FLOAT64_RGB) == 0)
		data_type = 1;

	for (int p = 0; p < local_patch->patch_count; p++)
	{
		MPR_patch reg_patch = local_patch->patch[p];
		MPR_zfp_compress output = (MPR_zfp_compress)malloc(sizeof(*output));
		memset(output, 0, sizeof (*output)); /* Initialization */
		output->p = (unsigned char*) malloc(patch_size);

		/* decompression */
		MPR_decompress_3D_data(reg_patch->buffer, reg_patch->patch_buffer_size, patch_x, patch_y, patch_z,
				file->mpr->compression_type, file->mpr->compression_param, data_type, &output);

		reg_patch->buffer = (unsigned char*)realloc(reg_patch->buffer, patch_size);
		memcpy(reg_patch->buffer, output->p, patch_size);
		free(output->p);
		free(output);
	}

	return MPR_success;
}

// ZFP float compression
MPR_return_code MPR_compress_3D_data(unsigned char* buf, int dim_x, int dim_y, int dim_z, int flag, float param, int data_type, MPR_zfp_compress* output)
{
	// ZFP data type according to PIDX data type
	zfp_type type = zfp_type_none;
	if (data_type == 0)
		type = zfp_type_float;
	else
		type = zfp_type_double;

	zfp_field* field = zfp_field_3d(buf, type, dim_x, dim_y, dim_z);
	zfp_stream* zfp = zfp_stream_open(NULL);
	// Two compression modes
	if (flag == 0)
		zfp_stream_set_accuracy(zfp, param);
	else if (flag == 1)
		zfp_stream_set_precision(zfp, param);
	else
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
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


MPR_return_code MPR_decompress_3D_data(unsigned char* buf, int size, int dim_x, int dim_y, int dim_z,
		int flag, float param, int data_type, MPR_zfp_compress* output)
{
	zfp_type type = zfp_type_none;
	if (data_type == 0)
		type = zfp_type_float;
	else
		type = zfp_type_double;

    zfp_field* field = zfp_field_3d((*output)->p, type, dim_x, dim_y, dim_z);
    zfp_stream* zfp = zfp_stream_open(NULL);
    if (flag == 0)
        zfp_stream_set_accuracy(zfp, param);
    else if (flag == 1)
        zfp_stream_set_precision(zfp, param);
    else
    {
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
    }
    bitstream* stream = stream_open(buf, size);
    zfp_stream_set_bit_stream(zfp, stream);
    if (!zfp_decompress(zfp, field)) {
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
    }
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    return MPR_success;
}
