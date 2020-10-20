/*
 * MPR_read_io.h
 *
 *  Created on: Oct 19, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_READ_IO_H_
#define SRC_IO_MPR_READ_IO_H_

MPR_return_code MPR_raw_read(MPR_file file, int svi);

MPR_return_code MPR_multi_res_read(MPR_file file, int svi);

MPR_return_code MPR_multi_pre_read(MPR_file file, int svi);

MPR_return_code MPR_multi_pre_res_read(MPR_file file, int svi);

MPR_return_code MPR_read_data(MPR_file file, int svi);

#endif /* SRC_IO_MPR_READ_IO_H_ */
