/*
 * MPR_write_io.h
 *
 *  Created on: Oct 2, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_WRITE_IO_H_
#define SRC_IO_MPR_WRITE_IO_H_

MPR_return_code MPR_raw_write(MPR_file file, int svi, int evi);

MPR_return_code MPR_multi_res_write(MPR_file file, int svi, int evi);

MPR_return_code MPR_multi_pre_write(MPR_file file, int svi, int evi);

MPR_return_code MPR_write_data_out(MPR_file file, int svi, int evi);

MPR_return_code MPR_metadata_write_out(MPR_file file, int svi, int evi);

#endif /* SRC_IO_MPR_WRITE_IO_H_ */
