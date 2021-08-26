/*
 * MPR_time_structs.h
 *
 *  Created on: Jul 4, 2020
 *      Author: kokofan
 */

#ifndef SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_
#define SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_

struct mpr_time_struct
{
	double total_start, total_end;                   /* The total time */
	double rst_start, rst_end;                       /* the time for restructuring phase */
	double wave_start, wave_end;                     /* the time for wavelet transform */
	double zfp_start, zfp_end;                       /* the time for zfp compression */
	double agg_start, agg_end;                       /* the time for aggregation phase */
	double wrt_data_start, wrt_data_end;             /* the time for write data out */

	double parse_bound_start, parse_bound_end;       /* the time for parsing bounding box time */
	double read_start, read_end;                     /* the time for reading data */

	/***************************** for visual system *****************************************/
	double crt_struc_start, crt_struc_end;
	double part_status_start, part_status_end;
	double part_gather_start, part_gather_end;
	double part_gather_basic_start, part_gather_basic_end;
	double part_gather_local_start, part_gather_local_end;
	double part_gather_comm_start, part_gather_comm_end;
	double part_cal_pcount_start, part_cal_pcount_end;
	double part_max_pshare_start, part_max_pshare_end;
	double part_assign_start, part_assign_end;
	double part_assign_mem_start, part_assign_mem_end;
	double part_assign_share_time;
	double part_assign_patch_time;
	double part_assign_update_time;
	double part_comm_start, part_comm_end;
	double part_comm_mem_start, part_comm_mem_end;
	double part_comm_recv_start, part_comm_recv_end;
	double part_comm_recv_pre_time;
	double part_comm_recv_calbox_time;
	double part_comm_recv_crtype_time;
	double part_comm_recv_req_time;
	double part_comm_send_start, part_comm_send_end;
	double part_comm_send_calbox_time;
	double part_comm_send_crtype_time;
	double part_comm_send_req_time;
	double part_comm_wait_start, part_comm_wait_end;

	double wave_pre_start, wave_pre_end;
	double wave_trans_time;
	double wave_org_time;

	double zfp_pre_start, zfp_pre_end;
	double zfp_comp_dc_time;
	double zfp_comp_bands_time;

	double agg_pre_start, agg_pre_end;
	double agg_gather_start, agg_gather_end;
	double agg_gather_local_start, agg_gather_local_end;
	double agg_gather_comm_start, agg_gather_comm_end;
	double agg_gather_filter_start, agg_gather_filter_end;
	double agg_calinfo_start, agg_calinfo_end;
	double agg_convert_z_start, agg_convert_z_end;
	double agg_assign_start, agg_assign_end;
	double agg_assign_act_start, agg_assign_act_end;
	double agg_assign_aggpik_start, agg_assign_aggpik_end;
	double agg_assign_calrecv_start, agg_assign_calrecv_end;
	double agg_comm_start, agg_comm_end;
	double agg_comm_pre_start, agg_comm_pre_end;
	double agg_comm_send_start, agg_comm_send_end;
	double agg_comm_recv_start, agg_comm_recv_end;
	double agg_comm_recv_act_time;
	double agg_comm_recv_bound_time;
	double agg_comm_recv_update_time;
	double agg_comm_wait_start, agg_comm_wait_end;
	double agg_clean_start, agg_clean_end;

	double wrt_meta_start, wrt_meta_end;
	double wrt_meta_basic_start, wrt_meta_basic_end;
	double wrt_meta_bound_start, wrt_meta_bound_end;
	double wrt_meta_file_start, wrt_meta_file_end;

	double wrt_file_start, wrt_file_end;

};

typedef struct mpr_time_struct* MPR_time;

#endif /* SRC_DATA_STRUCTS_MPR_TIME_STRUCTS_H_ */
