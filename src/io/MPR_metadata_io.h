/*
 * metadata_io.h
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_METADATA_IO_H_
#define SRC_IO_MPR_METADATA_IO_H_

MPR_return_code MPR_create_folder_structure(MPR_file file, int svi, int evi);

MPR_return_code MPR_basic_info_metadata_write_out(MPR_file file);

MPR_return_code MPR_out_file_metadata_write_out(MPR_file file, int svi, int evi);

#endif /* SRC_IO_MPR_METADATA_IO_H_ */
