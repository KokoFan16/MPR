/*
 * MPR_time_structs.h
 *
 *  Created on: Jul 4, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_

struct mpr_time_struct
{
	double total_start, total_end;                   /* The total time */
	double rst_start, rst_end;                       /* the time for restructuring phase */
	double wave_start, wave_end;                     /* the time for wavelet transform */
	double zfp_start, zfp_end;                       /* the time for zfp compression */
	double agg_start, agg_end;                       /* the time for aggregation phase */
	double wrt_data_start, wrt_data_end;             /* the time for write data out */
	double wrt_metadata_start, wrt_metadata_end;     /* the time for write metadata out */
};

typedef struct mpr_time_struct* MPR_time;

#endif /* SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_ */
