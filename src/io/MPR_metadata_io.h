/*
 * metadata_io.h
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_METADATA_IO_H_
#define SRC_IO_MPR_METADATA_IO_H_

MPR_return_code MPR_create_folder_structure(MPR_file file, int svi, int evi, int ite);

MPR_return_code MPR_metadata_write_out(MPR_file file, int svi, int evi, int ite);

MPR_return_code MPR_basic_info_metadata_write_out(MPR_file file);

MPR_return_code MPR_bounding_box_metadata_write_out(MPR_file file, int svi, int evi);

MPR_return_code MPR_file_metadata_write_out(MPR_file file, int svi, int evi, int ite);

MPR_return_code MPR_basic_metatda_parse(char* file_name, MPR_file* file);

MPR_return_code MPR_bounding_box_metatda_parse(char* file_name, MPR_file file);

MPR_return_code MPR_file_related_metadata_parse(char* file_name, MPR_file file, int var_id, int* patches_offset, int* patches_size, int* patches_subbands);

#endif /* SRC_IO_MPR_METADATA_IO_H_ */
