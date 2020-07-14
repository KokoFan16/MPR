/*
 * MPR_restructure.h
 *
 *  Created on: Jul 8, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_RESTRUCTURE_H_
#define SRC_CORE_MPR_RESTRUCTURE_H_

MPR_return_code MPR_restructure_perform(MPR_file file, int start_var_index, int end_var_index);

MPR_return_code MPR_restructure_setup(MPR_file file, int start_var_index, int end_var_index);

MPR_return_code MPR_restructure(MPR_file file, int start_var_index, int end_var_index);

MPR_return_code MPR_restructure_cleanup(MPR_file file);

#endif /* SRC_CORE_MPR_RESTRUCTURE_H_ */
