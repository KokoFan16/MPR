/*
 * MPR_error_code.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef INCLUDE_MPR_ERROR_CODE_H_
#define INCLUDE_MPR_ERROR_CODE_H_


/////////////////////////////////////////////////
// ERROR CODES
/////////////////////////////////////////////////

typedef int MPR_return_code;

extern MPR_return_code MPR_success;
extern MPR_return_code MPR_err_id;
extern MPR_return_code MPR_err_unsupported_flags;
extern MPR_return_code MPR_err_file_exists;
extern MPR_return_code MPR_err_name;
extern MPR_return_code MPR_err_box;
extern MPR_return_code MPR_err_file;
extern MPR_return_code MPR_err_time;
extern MPR_return_code MPR_err_block;
extern MPR_return_code MPR_err_comm;
extern MPR_return_code MPR_err_count;
extern MPR_return_code MPR_err_size;
extern MPR_return_code MPR_err_offset;
extern MPR_return_code MPR_err_type;
extern MPR_return_code MPR_err_variable;
extern MPR_return_code MPR_err_not_implemented;
extern MPR_return_code MPR_err_point;
extern MPR_return_code MPR_err_access;
extern MPR_return_code MPR_err_mpi;
extern MPR_return_code MPR_err_rst;
extern MPR_return_code MPR_err_chunk;
extern MPR_return_code MPR_err_compress;
extern MPR_return_code MPR_err_hz;
extern MPR_return_code MPR_err_agg;
extern MPR_return_code MPR_err_io;
extern MPR_return_code MPR_err_unsupported_compression_type;
extern MPR_return_code MPR_err_close;
extern MPR_return_code MPR_err_flush;
extern MPR_return_code MPR_err_header;
extern MPR_return_code MPR_err_wavelet;
extern MPR_return_code MPR_err_metadata;
extern MPR_return_code MPR_err_res;


#endif /* INCLUDE_MPR_ERROR_CODE_H_ */
