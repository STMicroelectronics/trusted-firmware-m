/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <psa/client.h>
#include <psa_manifest/sid.h>
#include <tfm_stl_api.h>
#include <stl_user_api.h>

psa_status_t tfm_stl_req(void *req, size_t req_len, void *rsp, size_t rsp_len)
{
	psa_status_t status;
	psa_outvec out_vec;
	psa_invec in_vec;

	in_vec.base = (const void *)req;
	in_vec.len = req_len;

	out_vec.base = (void *)rsp;
	out_vec.len = rsp_len;

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, &in_vec, 1,
			  &out_vec, 1);

	return status;
}

STL_Status_t STL_SCH_Init(void)
{
	uint32_t req[1] = {stl_init};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), NULL, 0);
	return ret == PSA_SUCCESS ? STL_OK_DEF : STL_KO_DEF;
}

static STL_Status_t  run_cpu_tmx(STL_TmStatus_t *p_status,
				 enum stl_module index)
{
	uint32_t req[2] = {stl_run, index};

	psa_status_t ret = tfm_stl_req(&req, sizeof(req),
				       p_status, sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK_DEF : STL_KO_DEF;
}

STL_Status_t STL_SCH_RunCpuTM1(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm1);
}

STL_Status_t STL_SCH_RunCpuTM1L(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm1l);
}

STL_Status_t STL_SCH_RunCpuTM2(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm2);
}

STL_Status_t STL_SCH_RunCpuTM3(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm3);
}

STL_Status_t STL_SCH_RunCpuTM4(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm4);
}

STL_Status_t STL_SCH_RunCpuTM5(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm5);
}

STL_Status_t STL_SCH_RunCpuTM6(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm6);
}

STL_Status_t STL_SCH_RunCpuTM7(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm7);
}

STL_Status_t STL_SCH_RunCpuTM7L(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm7l);
}

STL_Status_t STL_SCH_RunCpuTM8(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm8);
}

STL_Status_t STL_SCH_RunCpuTM9(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm9);
}

STL_Status_t STL_SCH_RunCpuTM10(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm10);
}

STL_Status_t STL_SCH_RunCpuTM11(STL_TmStatus_t *p_status)
{
	return run_cpu_tmx(p_status, cpu_tm11);
}

STL_Status_t STL_SCH_InitRam(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_initmem, ram_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_InitFlash(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_initmem, flash_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

static STL_Status_t stl_serialize(STL_MemConfig_t *p_cfg,
				  tfm_stl_mem_cfg_t *p_tfm_cfg)
{
	STL_MemSubset_t *p_subset=p_cfg->pSubset;

	if (p_tfm_cfg->n_subset != 0)
		return STL_KO;
	memcpy(&p_tfm_cfg->mem_cfg, p_cfg, sizeof(p_tfm_cfg->mem_cfg));
	while (p_subset)
	{
		memcpy(&p_tfm_cfg->subset[p_tfm_cfg->n_subset], p_subset,
		       sizeof(STL_MemSubset_t));
		p_tfm_cfg->n_subset++;
		p_subset = p_subset->pNext;
		if ((p_subset) && (p_tfm_cfg->n_subset >= MAX_MEM_SUBSET))
		{
			p_tfm_cfg->n_subset = 0;
			return STL_KO;
		}
	}

	return STL_OK;
}

/* Single ram and flash config*/
static tfm_stl_mem_cfg_t ns_ram_cfg;
static tfm_stl_mem_cfg_t ns_flash_cfg;

STL_Status_t STL_SCH_ConfigureRam(STL_TmStatus_t *p_status,
				  STL_MemConfig_t *p_ram_cfg)
{
	uint32_t req[2] = {stl_cfg, ram_run};
	psa_status_t status;
	psa_outvec out_vec;
	psa_invec in_vec[2];

	if (stl_serialize(p_ram_cfg, &ns_ram_cfg) != STL_OK)
		return STL_KO;

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);
	in_vec[1].base = &ns_ram_cfg;
	in_vec[1].len = sizeof(ns_ram_cfg);

	out_vec.base = (void *)p_status;
	out_vec.len = sizeof(*p_status);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 2,
			  &out_vec, 1);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}
STL_Status_t STL_SCH_ConfigureFlash(STL_TmStatus_t *p_status,
				    STL_MemConfig_t *p_flash_cfg)
{
	uint32_t req[2] = {stl_cfg, flash_run};
	psa_status_t status;
	psa_outvec out_vec;
	psa_invec in_vec[2];

	if (stl_serialize(p_flash_cfg, &ns_flash_cfg) != STL_OK)
		return STL_KO;

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);
	in_vec[1].base = &ns_flash_cfg;
	in_vec[1].len = sizeof(ns_flash_cfg);

	out_vec.base = (void *)p_status;
	out_vec.len = sizeof(*p_status);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 2,
			  &out_vec, 1);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_RunRamTM(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_run, ram_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_ResetRam(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_reset, ram_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	if (ret == PSA_SUCCESS) {
		ns_ram_cfg.n_subset = 0;
	}

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_DeInitRam(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_deinit, ram_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_StartArtifFailing(const STL_ArtifFailingConfig_t *p_art_fail_cfg)
{
	uint32_t req[1] = {stl_startfail};
	psa_status_t status;
	psa_invec in_vec[2];

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);
	in_vec[1].base = p_art_fail_cfg;
	in_vec[1].len = sizeof(*p_art_fail_cfg);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 2,
			  NULL, 0);

	return status == PSA_SUCCESS ? STL_OK: STL_KO;
}

STL_Status_t STL_SCH_StopArtifFailing(void)
{
	uint32_t req[1] = {stl_stopfail};
	psa_status_t status;
	psa_invec in_vec[1];

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 1,
			  NULL, 0);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_DeInitFlash(STL_TmStatus_t *p_status)
{
	uint32_t req[2] = {stl_deinit, flash_run};
	psa_status_t ret = tfm_stl_req(&req, sizeof(req), p_status,
				       sizeof(*p_status));

	return ret == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_InitAllTM(STL_TmListStatus_t *p_list_status,
			       STL_TmListEnable_t *p_list_enable)
{
	uint32_t req[1] = {stl_init_all};
	psa_status_t status;
	psa_invec in_vec[2];
	psa_outvec out_vec;

	out_vec.base = (void *)p_list_status;
	out_vec.len = sizeof(*p_list_status);

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);
	in_vec[1].base = p_list_enable;
	in_vec[1].len = sizeof(*p_list_enable);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 2,
			  &out_vec, 1);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_ConfigureAllTM(STL_TmListStatus_t *p_list_status,
				    STL_MemConfig_t *p_flash_cfg,
				    STL_MemConfig_t *p_ram_cfg)
{
	uint32_t req[1] = {stl_cfg_all};
	psa_status_t status;
	psa_invec in_vec[3];
	psa_outvec out_vec;

	out_vec.base = (void *)p_list_status;
	out_vec.len = sizeof(*p_list_status);

	if (stl_serialize(p_flash_cfg, &ns_flash_cfg) != STL_OK)
		return STL_KO;
	if (stl_serialize(p_ram_cfg, &ns_ram_cfg) != STL_OK)
		return STL_KO;

	in_vec[0].base = (const void *)req;
	in_vec[0].len = sizeof(req);
	in_vec[1].base = &ns_flash_cfg;
	in_vec[1].len = sizeof(ns_flash_cfg);
	in_vec[2].base = &ns_ram_cfg;
	in_vec[2].len = sizeof(ns_ram_cfg);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, in_vec, 3,
			  &out_vec, 1);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}

STL_Status_t STL_SCH_RunAllTM(STL_TmListStatus_t *p_list_status)
{
	uint32_t req[1] = {stl_run_all};
	psa_status_t status;
	psa_invec in_vec;
	psa_outvec out_vec;

	out_vec.base = (void *)p_list_status;
	out_vec.len = sizeof(*p_list_status);

	in_vec.base = (const void *)req;
	in_vec.len = sizeof(req);

	status = psa_call(TFM_STL_SERVICE_HANDLE, PSA_IPC_CALL, &in_vec, 1,
			  &out_vec, 1);

	return status == PSA_SUCCESS ? STL_OK : STL_KO;
}
