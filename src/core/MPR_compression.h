/*
 * MPR_compression.h
 *
 *  Created on: Sep 23, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_COMPRESSION_H_
#define SRC_CORE_MPR_COMPRESSION_H_

// Structure of ZFP pointer
struct MPR_zfp_compress_pointer
{
  unsigned char *p;
  int compress_size;
};
typedef struct MPR_zfp_compress_pointer* MPR_zfp_compress;

MPR_return_code MPR_ZFP_compression_perform(MPR_file file, int svi, int evi);

MPR_zfp_compress MPR_compress_3D_float(unsigned char* buf, int dim_x, int dim_y, int dim_z, int flag, float param, char* type_name);

#endif /* SRC_CORE_MPR_COMPRESSION_H_ */
