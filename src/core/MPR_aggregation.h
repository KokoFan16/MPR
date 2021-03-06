/*
 * MPR_aggregation.h
 *
 *  Created on: Aug 7, 2020
 *      Author: kokofan
 */

#ifndef SRC_CORE_MPR_AGGREGATION_H_
#define SRC_CORE_MPR_AGGREGATION_H_

MPR_return_code MPR_aggregation_perform(MPR_file file, int svi, int evi);

MPR_return_code MPR_no_aggregation(MPR_file file, int svi, int evi);

#endif /* SRC_CORE_MPR_AGGREGATION_H_ */
