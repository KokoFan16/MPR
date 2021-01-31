/*
 * MPR_logs.h
 *
 *  Created on: Jan 20, 2021
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_LOG_IO_H_
#define SRC_IO_MPR_LOG_IO_H_

float* time_buffer;
long long int* size_buffer;

MPR_return_code MPR_timing_logs(MPR_file file, int svi, int evi);

MPR_return_code MPR_logs(MPR_file file, int svi, int evi);


#endif /* SRC_IO_MPR_LOG_IO_H_ */
