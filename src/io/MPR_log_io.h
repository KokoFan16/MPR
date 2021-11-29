/*
 * MPR_logs.h
 *
 *  Created on: Jan 20, 2021
 *      Author: kokofan
 */

#pragma once

#ifndef SRC_IO_MPR_LOG_IO_H_
#define SRC_IO_MPR_LOG_IO_H_

extern float* time_buffer;
extern long long int* size_buffer;

MPR_return_code MPR_timing_logs(MPR_file file, int svi, int evi);

MPR_return_code MPR_logs(MPR_file file, int svi, int evi);


#endif /* SRC_IO_MPR_LOG_IO_H_ */
