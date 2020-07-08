/*
 * MPR_point.c
 *
 *  Created on: Jul 6, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_set_point(MPR_point point, uint64_t  x, uint64_t  y, uint64_t z)
{
  if (point == NULL)
    return MPR_err_point;

  point[0] = x;
  point[1] = y;
  point[2] = z;

  return MPR_success;
}

MPR_return_code MPR_get_point(uint64_t* x, uint64_t* y, uint64_t* z, MPR_point point)
{
  if (point == NULL)
    return MPR_err_point;

  *x = point[0];
  *y = point[1];
  *z = point[2];

  return MPR_success;
}
