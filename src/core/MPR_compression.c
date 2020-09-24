
//#include <zfp.h>
#include <zfp.h>
#include "../MPR_inc.h"

MPR_return_code MPR_ZFP_compression_perform(MPR_file file, int svi, int evi)
{
	return MPR_success;
}

// ZFP float compression
MPR_zfp_compress MPR_compress_3D_float(unsigned char* buf, int dim_x, int dim_y, int dim_z, int flag, float param, char* type_name)
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
	printf("ERROR: ZFP cannot handle type %s\n", type_name);

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
  MPR_zfp_compress output;
  output->p = (unsigned char*) malloc(max_compressed_bytes);
  bitstream* stream = stream_open(output->p, max_compressed_bytes);
  zfp_stream_set_bit_stream(zfp, stream);
  size_t compressed_bytes = zfp_compress(zfp, field);
  output->compress_size = compressed_bytes; // Data size after compression
  if (compressed_bytes == 0)
	puts("ERROR: Something wrong happened during compression\n");
  zfp_field_free(field);
  zfp_stream_close(zfp);
  stream_close(stream);
  return output;
}
