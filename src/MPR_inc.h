/*
 * MPR_inc.h
 *
 *  Created on: Jul 2, 2020
 *      Author: kokofan
 */

#ifndef SRC_MPR_INC_H_
#define SRC_MPR_INC_H_

#include "../include/MPR_define.h"
#include "utils/MPR_error_code.h"
#include "utils/MPR_point.h"
#include "comm/MPR_comm.h"
#include "data_structs/MPR_file_structs.h"
#include "io/MPR_io.h"
#include "io/MPR_metadata_io.h"
#include "io/MPR_raw_io.h"
#include "core/MPR_restructure.h"
#include "core/MPR_aggregation.h"
#include "core/MPR_compression.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "../external/zfp/include/zfp.h"


#endif /* SRC_MPR_INC_H_ */
