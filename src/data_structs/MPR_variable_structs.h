/*
 * MPR_variable_structs.h
 *
 *  Created on: Jul 3, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_VARIABLE_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_VARIABLE_STRUCTS_H_

#include "MPR_local_patch_structs.h"

struct mpr_variable_struct
{
	char var_name[MPR_FILE_PATH_LENGTH];         /* Variable name */
	int vps;                                     /* values per sample, Vector(3), scalar(1), or n */
	int bpv;                                     /* Number of bits each need */

	MPR_data_type type_name;                     /* Name of the type uint8, bob */
	MPR_data_layout data_layout;                 /* Row major or column major */

	MPR_local_patch local_patch;                 /* Local patches related information */
};
typedef struct mpr_variable_struct* MPR_variable;

#endif /* SRC_DATA_STRUCTS_MPR_VARIABLE_STRUCTS_H_ */
