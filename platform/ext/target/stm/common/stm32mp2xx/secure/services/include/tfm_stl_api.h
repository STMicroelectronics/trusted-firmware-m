// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 */
#ifndef TFM_STL_API_H
#define TFM_STL_API_H
#include <psa/client.h>
#include <stl_user_api.h>

enum stl_func{
	stl_init,/* To initalise the scheduler :
		    param void,
		    return void */
	stl_run, /* To run Test Module
		    param STL_TmStatus_t,
		    return STL_Status_t*/
	stl_initmem, /* To initialise Flash test - only for single test
			param STL_TmStatus_t,
			return STL_Status_t*/
	stl_cfg, /* To configure Flash/RAM subsets test - only for single test
		    params STL_TmStatus_t, STL_MemConfig_t,
		    return STL_Status_t */
	stl_reset, /* To reset FLASH/RAM subsets - only for single test or multiple test
		      params STL_TmStatus_t,
		      return STL_Status_t */
	stl_deinit,/* To de-initialise FLASH/RAM test - only for single test
		      param STL_TmStatus_t,
		      return STL_Status_t */
	stl_init_all, /* To initialise all tests - only for multiple test
			 param STL_TmListStatus_t, STL_TmListEnable_t,
			 return STL_Status_t */
	stl_cfg_all, /* To configure all tests - only for multiple test
			param STL_TmListStatus_t, STL_MemConfig_t(FLASH), STL_MemConfig_t(RAM)
			return STL_Status */
	stl_run_all, /* To run all tests - only for multiple test
			param STL_TmListStatus_t,
			return STL_Status */
	stl_startfail, /* To set artificial failing configuration and start
			  param STL_ArtifFailingConfig_t,
			  return STL_Status_t */
	stl_stopfail, /* To stop artificial failing
			 param void,
			 return STL_Status_t */
};

/* User API functions prototypes */
enum stl_module {
	cpu_tm1 = STL_CPU_TM1_IDX,
	cpu_tm1l,
	cpu_tm2,
	cpu_tm3,
	cpu_tm4,
	cpu_tm5,
	cpu_tm6,
	cpu_tm7,
	cpu_tm7l,
	cpu_tm8,
	cpu_tm9,
	cpu_tm10,
	cpu_tm11,
	flash_run,
	ram_run,
};
/* Number customizeable at build */
#define MAX_MEM_SUBSET 100
/* structure to serialize MemConfig to pass to Secure */
typedef struct
{
	/* Fix me for safety in os with multiple threads, since a single
	 * structure is allocated for RAM, write access must be guarded to prevent
	 * multiple threads from a Config request */
	uint32_t n_subset; /* number of subset */
	STL_MemConfig_t mem_cfg;
	STL_MemSubset_t subset[MAX_MEM_SUBSET]; /* Pointer to the Flash or RAM subsets to test */
} tfm_stl_mem_cfg_t;

psa_status_t tfm_stl_req(void *req, size_t req_len, void *rsp, size_t rsp_len);

#endif /* TFM_SCMI_API_H */
