/*
 * MPR_comm_structs.h
 *
 *  Created on: Jul 6, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_COMM_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_COMM_STRUCTS_H_

#include <mpi.h>

struct mpr_comm_struct
{
	MPI_Comm simulation_comm;     /// Communicator passed by the application (simulation)
	int simulation_rank;          /// rank of a process within global_comm
	int simulation_nprocs;        /// number of processes in global_comm
};
typedef struct mpr_comm_struct* MPR_comm;

#endif /* SRC_DATA_STRUCTS_MPR_COMM_STRUCTS_H_ */
