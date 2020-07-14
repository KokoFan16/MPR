/*
 * MPR_io.c
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#include "../MPR_inc.h"

MPR_return_code MPR_write(MPR_file file, int svi, int evi, int MODE)
{
	/* Set the default restructuring box (32x32x32) */
	if (MPR_set_rst_box_size(file, svi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}
	/* Create metadata layout (folder) */
	if (MPR_create_folder_structure(file, svi, evi) != MPR_success)
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

	if (MPR_restructure_perform(file, svi, evi))
	{
		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
		return MPR_err_file;
	}

//	if (MODE == MPR_RAW_IO)
//		ret = MPR_raw_write(file, svi, evi);
//
//	else if (MODE == MPR_MUL_RES_IO)
//		ret = MPR_multi_resolution_write(file, svi, evi);
//
//	else if (MODE == MPR_MUL_PRE_IO)
//		ret = MPR_multi_precision_write(file, svi, evi);
//
//	else if (MODE == MPR_MUL_RES_PRE_IO)
//		ret = MPR_multi_multi_resolution_precision_write(file, svi, evi);
//	else
//	{
//		fprintf(stderr, "[%s] [%d]\n", __FILE__, __LINE__);
//		return MPR_err_unsupported_flags;
//	}
//
//	if (ret != MPR_success)
//	{
//		fprintf(stderr, "File %s Line %d\n", __FILE__, __LINE__);
//		return MPR_err_file;
//	}

	return MPR_success;
}


/* Set the default restructuring box (32x32x32) */
MPR_return_code MPR_set_rst_box_size(MPR_file file, int svi)
{
	if ((file->restructured_patch->patch_size[0] == -1 || file->restructured_patch->patch_size[1] == -1 || file->restructured_patch->patch_size[2] == -1)
		|| (file->restructured_patch->patch_size[0] == 0 &&  file->restructured_patch->patch_size[1] == 0 && file->restructured_patch->patch_size[2] == 0))
	{
		fprintf(stderr,"Warning: restructuring box is not set, using default 32x32x32 size. [File %s Line %d]\n", __FILE__, __LINE__);
		file->restructured_patch->patch_size[0]=32;file->restructured_patch->patch_size[1]=32;file->restructured_patch->patch_size[2]=32;
	}

	return MPR_success;
}
