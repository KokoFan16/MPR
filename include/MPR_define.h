/*
 * MPR_define.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef INCLUDE_MPR_DEFINE_H_
#define INCLUDE_MPR_DEFINE_H_

#define MPR_MAX_DIMENSIONS           3  /* Define maximum dimensions */
#define MPR_MAX_VARIABLE_COUNT     768  /* Maximum variable count */
#define MPR_FILE_PATH_LENGTH      1024  /* Define maximum length for file path */

#define MPR_MODE_CREATE              1  /* Create the file if it does not exist */
#define MPR_MODE_EXCL               64  /* Error creating a file that already exists */
#define MPR_MODE_RDONLY              2  /* ADIO_RDONLY */
#define MPR_MODE_WRONLY              4  /* ADIO_WRONLY  */
#define MPR_MODE_RDWR                8  /* ADIO_RDWR  */
#define MPR_MODE_DELETE_ON_CLOSE    16  /* ADIO_DELETE_ON_CLOSE */
#define MPR_MODE_UNIQUE_OPEN        32  /* ADIO_UNIQUE_OPEN */
#define MPR_MODE_APPEND            128  /* ADIO_APPEND */
#define MPR_MODE_SEQUENTIAL        256  /* ADIO_SEQUENTIAL */

#define MPR_NO_COMPRESSION           0  /* No compression */
#define MPR_ZFP_ACC                  1  /* Compression with ZFP accuracy mode */
#define MPR_ZFP_PRE                  2  /* Compression with ZFP precision mode */

#define MPR_CONT_FILE_SIZE_MODE      0  /* Control the size of out file */
#define MPR_CONT_PATCH_NUM_MODE      1  /* Control the number of patches of out file */

#define MPR_row_major               0  /* Data in buffer is in row order */
#define MPR_column_major            1  /* Data in buffer is in column order */

/* Enumerate file type (MPR_READ-Read only mode) (MPR_WRITE - Write only mode{) */
enum IO_READ_WRITE {MPR_READ, MPR_WRITE};

/* Enumerate  all the IO type */
enum MPR_io_type {
	MPR_RAW_IO = 0,                 /* Write data in row format */
	MPR_MUL_RES_IO = 1,           /* Write data with multiple resolution format (wavelet transform) */
	MPR_MUL_PRE_IO = 2,           /* Write data with multiple precision format (ZFP compression) */
	MPR_MUL_RES_PRE_IO = 3        /* Write data with multiple resolution and precision format */
};

typedef unsigned int MPR_data_layout;
typedef char MPR_data_type[512];

struct mpr_dtype {
    MPR_data_type INT8;
    MPR_data_type INT8_GA;
    MPR_data_type INT8_RGB;
    MPR_data_type INT8_RGBA;

    MPR_data_type UINT8;
    MPR_data_type UINT8_GA;
    MPR_data_type UINT8_RGB;
    MPR_data_type UINT8_RGBA;

    MPR_data_type INT16;
    MPR_data_type INT16_GA;
    MPR_data_type INT16_RGB;
    MPR_data_type INT16_RGBA;

    MPR_data_type UINT16;
    MPR_data_type UINT16_GA;
    MPR_data_type UINT16_RGB;
    MPR_data_type UINT16_RGBA;

    MPR_data_type INT32;
    MPR_data_type INT32_GA;
    MPR_data_type INT32_RGB;
    MPR_data_type INT32_RGBA;

    MPR_data_type UINT32;
    MPR_data_type UINT32_GA;
    MPR_data_type UINT32_RGB;
    MPR_data_type UINT32_RGBA;

    MPR_data_type INT64;
    MPR_data_type INT64_GA;
    MPR_data_type INT64_RGB;
    MPR_data_type INT64_RGBA;

    MPR_data_type UINT64;
    MPR_data_type UINT64_GA;
    MPR_data_type UINT64_RGB;
    MPR_data_type UINT64_RGBA;

    MPR_data_type FLOAT32;
    MPR_data_type FLOAT32_GA;
    MPR_data_type FLOAT32_RGB;
    MPR_data_type FLOAT32_RGBA;
    MPR_data_type FLOAT32_7STENCIL;
    MPR_data_type FLOAT32_9TENSOR;

    MPR_data_type FLOAT64;
    MPR_data_type FLOAT64_GA;
    MPR_data_type FLOAT64_RGB;
    MPR_data_type FLOAT64_RGBA;
    MPR_data_type FLOAT64_7STENCIL;
    MPR_data_type FLOAT64_9TENSOR;

    MPR_data_type INT64_9TENSOR;
    MPR_data_type INT32_9TENSOR;
};

extern struct mpr_dtype MPR_DType;



#endif /* INCLUDE_MPR_DEFINE_H_ */
