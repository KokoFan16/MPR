/*
 * raw_io.h
 *
 *  Created on: Aug 6, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_RAW_IO_H_
#define SRC_IO_MPR_RAW_IO_H_

MPR_return_code MPR_raw_write(MPR_file file, int svi, int evi);

MPR_return_code MPR_raw_write_data_out(MPR_file file, int svi, int evi);

MPR_return_code MPR_metadata_raw_write_out(MPR_file file, int svi, int evi);

#endif /* SRC_IO_MPR_RAW_IO_H_ */
