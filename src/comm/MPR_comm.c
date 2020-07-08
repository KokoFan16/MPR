/*
 * MPR_comm.c
 *
 *  Created on: Jul 6, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_create_access(MPR_access* access)
{
  *access = (MPR_access)malloc(sizeof (*(*access)));
  memset(*access, 0, sizeof (*(*access)));

  (*access)->comm = MPI_COMM_NULL;

  return MPR_success;
}

MPR_return_code MPR_set_mpi_access(MPR_access access, MPI_Comm comm)
{
  if (access == NULL)
    return MPR_err_access;

  access->comm = comm;

  return MPR_success;
}

MPR_return_code MPR_close_access(MPR_access access)
{
  if (access == NULL)
    return MPR_err_access;

  free(access);

  return MPR_success;
}
