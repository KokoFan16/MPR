/*
 * MPR_point.h
 *
 *  Created on: Jul 5, 2020
 *      Author: kokofan
 */

#ifndef SRC_UTILS_MPR_POINT_H_
#define SRC_UTILS_MPR_POINT_H_

#include <stdint.h>

typedef int MPR_point[MPR_MAX_DIMENSIONS];

typedef double MPR_physical_point[MPR_MAX_DIMENSIONS];

/// Utility functions to set or get the dimensions of an offset or a box (defined as points)
MPR_return_code MPR_set_point(MPR_point point, int x, int y, int z);
MPR_return_code MPR_get_point(int* x, int* y, int* z, MPR_point point);

#endif /* SRC_UTILS_MPR_POINT_H_ */
