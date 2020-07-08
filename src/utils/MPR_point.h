/*
 * MPR_point.h
 *
 *  Created on: Jul 5, 2020
 *      Author: kokofan
 */

#ifndef SRC_UTILS_MPR_POINT_H_
#define SRC_UTILS_MPR_POINT_H_

#include <stdint.h>

typedef uint64_t MPR_point[MPR_MAX_DIMENSIONS];

typedef double MPR_physical_point[MPR_MAX_DIMENSIONS];

/// Utility functions to set or get the dimensions of an offset or a box (defined as points)
MPR_return_code MPR_set_point(MPR_point point, uint64_t  x,uint64_t  y,uint64_t  z);
MPR_return_code MPR_get_point(uint64_t* x,uint64_t* y,uint64_t* z, MPR_point point);

MPR_return_code MPR_get_physical_point(double* x, double* y, double* z, MPR_physical_point point);
MPR_return_code MPR_set_physical_point(MPR_physical_point point, double  x, double  y, double  z);


#endif /* SRC_UTILS_MPR_POINT_H_ */
