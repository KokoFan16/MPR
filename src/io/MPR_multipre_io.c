/*
 * multipre_io.c
 *
 *  Created on: Sep 23, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_multi_pre_write(MPR_file file, int svi, int evi)
{
	if (MPR_ZFP_compression_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	return MPR_success;
}
