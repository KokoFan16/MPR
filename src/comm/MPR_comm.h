/*
 * MPR_comm.h
 *
 *  Created on: Jul 6, 2020
 *      Author: kokofan
 */

#ifndef SRC_UTILS_MPR_COMM_H_
#define SRC_UTILS_MPR_COMM_H_

#include <mpi.h>

struct MPR_access_struct
{
  MPI_Comm comm;
};
typedef struct MPR_access_struct* MPR_access;

/// Call this before opening or creating a MPR_file.
MPR_return_code MPR_create_access(MPR_access* access);

MPR_return_code MPR_close_access(MPR_access access);

MPR_return_code MPR_set_mpi_access(MPR_access access, MPI_Comm comm);

#endif /* SRC_UTILS_MPR_COMM_H_ */
