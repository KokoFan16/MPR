/*
 * MPR_restructure.h
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_RESTRUCTURE_H_
#define SRC_CORE_MPR_RESTRUCTURE_H_

MPR_return_code MPR_set_patch_box_size(MPR_file file, int svi);

MPR_return_code MPR_restructure_perform(MPR_file file, int start_var_index, int end_var_index);

MPR_return_code MPR_processing(MPR_file file, int svi, int evi);

MPR_return_code MPR_is_partition(MPR_file file, int svi, int evi);

#endif /* SRC_CORE_MPR_RESTRUCTURE_H_ */
