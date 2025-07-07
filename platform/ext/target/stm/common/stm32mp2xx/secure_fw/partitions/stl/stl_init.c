/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */
#include <stdbool.h>
#include <tfm_sp_log.h>
#include <tfm_hal_defs.h>
#include <uart_stdout.h>
#include <string.h>
#include <psa/framework_feature.h>
#include <psa/service.h>
#include <assert.h>
#include <psa_manifest/tfm_stl.h>
#include <psa/service.h>
#include <stl_user_api.h>
#include <tfm_stl_api.h>

static STL_Status_t init_test(STL_TmStatus_t *p_status, enum stl_module index)
{
	switch (index) {
	case ram_run:
		return  STL_SCH_InitRam(p_status);
	case flash_run:
		return  STL_SCH_InitFlash(p_status);
	default: return STL_KO;
	}
}
static STL_Status_t deinit_test(STL_TmStatus_t *p_status, enum stl_module index)
{
	switch (index) {
	case ram_run:
		return STL_SCH_DeInitRam(p_status);
	case flash_run:
		return STL_SCH_DeInitFlash(p_status);

	default: return STL_KO;
	}
}

static STL_Status_t run_test(STL_TmStatus_t *p_status, enum stl_module index)
{
	switch (index) {
	case cpu_tm1:
		return STL_SCH_RunCpuTM1(p_status);

	case cpu_tm1l :
		return STL_SCH_RunCpuTM1L(p_status);

	case cpu_tm2:
		return STL_SCH_RunCpuTM2(p_status);

	case cpu_tm3:
		return STL_SCH_RunCpuTM3(p_status);

	case cpu_tm4:
		return STL_SCH_RunCpuTM4(p_status);

	case cpu_tm5:
		return STL_SCH_RunCpuTM5(p_status);

	case cpu_tm6:
		return STL_SCH_RunCpuTM6(p_status);

	case cpu_tm7:
		return STL_SCH_RunCpuTM7(p_status);

	case cpu_tm7l:
		return STL_SCH_RunCpuTM7L(p_status);

	case cpu_tm8:
		return STL_SCH_RunCpuTM8(p_status);

	case cpu_tm9:
		return STL_SCH_RunCpuTM9(p_status);

	case cpu_tm10:
		return STL_SCH_RunCpuTM10(p_status);

	case cpu_tm11:
		return STL_SCH_RunCpuTM11(p_status);

	case ram_run:
		return STL_SCH_RunRamTM(p_status);

	case flash_run:
		return STL_SCH_RunFlashTM(p_status);

	default: return STL_KO;

	}
}
/* Deserialize function, pNext must be set */
static STL_Status_t stl_deserialize(tfm_stl_mem_cfg_t *p_cfg)
{
	uint32_t i;

	if (p_cfg->n_subset == 0)
		return STL_KO;

	p_cfg->mem_cfg.pSubset = &p_cfg->subset[0];
	p_cfg->subset[0].pNext = 0;

	for(i = 1; i < p_cfg->n_subset; i++) {
		p_cfg->subset[i-1].pNext = &p_cfg->subset[i];
		p_cfg->subset[i].pNext = 0;
	}

	return STL_OK;
}

/* Unique ram and flashconfig */
static tfm_stl_mem_cfg_t s_ram_cfg;
static tfm_stl_mem_cfg_t s_flash_cfg;

static STL_Status_t reset_test(STL_TmStatus_t *p_status,
			       enum stl_module index)
{
	switch (index) {
	case ram_run:
		s_ram_cfg.n_subset = 0;
		return STL_SCH_ResetRam(p_status);

	case flash_run:
		s_flash_cfg.n_subset = 0;
		return STL_SCH_ResetFlash(p_status);
	default: return STL_KO;
	}
}
/* Unique variable for artifail config*/
static STL_ArtifFailingConfig_t s_arti_fail_cfg;

static STL_Status_t config_test(STL_TmStatus_t *p_status, enum stl_module index,
				 const psa_msg_t *msg)
{
	size_t cfg_sz = msg->in_size[1];

	switch (index) {
	case ram_run:
		/* Avoid overwriting Config */
		if (s_ram_cfg.n_subset)
			return STL_KO;
		if (sizeof(s_ram_cfg) < cfg_sz)
			return STL_KO;
		psa_read(msg->handle, 1,&s_ram_cfg, cfg_sz);
		/* Deserialize */
		if (stl_deserialize(&s_ram_cfg) != STL_OK)
			return STL_KO;
		return STL_SCH_ConfigureRam(p_status, &s_ram_cfg.mem_cfg);

	case flash_run:
		/* Avoid overwriting Config */
		if (s_flash_cfg.n_subset)
			return STL_KO;
		if (sizeof(s_flash_cfg) < cfg_sz)
			return STL_KO;
		psa_read(msg->handle, 1, &s_flash_cfg, cfg_sz);
		/* Deserialize */
		if (stl_deserialize(&s_flash_cfg) != STL_OK)
			return STL_KO;
		return STL_SCH_ConfigureFlash(p_status, &s_flash_cfg.mem_cfg);

	default: return STL_KO;
	}
}
static STL_Status_t config_all_test(STL_TmListStatus_t *p_list_status,
				 const psa_msg_t *msg)
{
	/* Avoid overwriting Config */
	if (s_ram_cfg.n_subset)
		return STL_KO;
	if (sizeof(s_ram_cfg) != msg->in_size[2])
		return STL_KO;
	psa_read(msg->handle, 2, &s_ram_cfg, msg->in_size[2]);
	/* Deserialize */
	if (stl_deserialize(&s_ram_cfg) != STL_OK)
		return STL_KO;
	/* Avoid overwriting Config */
	if (s_flash_cfg.n_subset)
		return STL_KO;
	if (sizeof(s_flash_cfg) != msg->in_size[1])
		return STL_KO;
	psa_read(msg->handle, 1, &s_flash_cfg, msg->in_size[1]);
	/* Deserialize */
	if (stl_deserialize(&s_flash_cfg) != STL_OK)
		return STL_KO_DEF;

	return STL_SCH_ConfigureAllTM(p_list_status, &s_flash_cfg.mem_cfg,
				      &s_ram_cfg.mem_cfg);
}

psa_status_t tfm_stl_entry(void)
{
	return PSA_SUCCESS;
}

psa_status_t tfm_stl_service_sfn(const psa_msg_t *msg)
{
	size_t in_sz = msg->in_size[0];
	size_t out_sz = msg->out_size[0];
	size_t cfg_sz[2] = {msg->in_size[1], msg->in_size[2]};
	uint32_t in_buf[32] = {0};
	STL_TmStatus_t status = {0};
	STL_TmListStatus_t list_status = {0};
	STL_Status_t ret;

	if (in_sz >= sizeof(in_buf))
		return PSA_ERROR_NOT_SUPPORTED;

	psa_read(msg->handle, 0, in_buf, in_sz);

	if ((in_sz == 4) && (out_sz == 0)) {
		if (in_buf[0] == stl_init) {
			ret = STL_SCH_Init();
			if (ret == STL_OK) {
				return PSA_SUCCESS;
			}
		}
	}

	if ((in_sz == 8) && (out_sz == sizeof(status)) && (cfg_sz[0] == 0)
	    && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_run) {
			ret = run_test(&status, (STL_CpuTmxIndex_t)in_buf[1]);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0,&status, sizeof(status));
				return PSA_SUCCESS;
			}
		}
		if (in_buf[0] == stl_initmem) {
			ret = init_test(&status, (STL_CpuTmxIndex_t)in_buf[1]);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &status, sizeof(status));
				return PSA_SUCCESS;
			}
		}
		if (in_buf[0] == stl_deinit) {
			ret = deinit_test(&status, (STL_CpuTmxIndex_t)in_buf[1]);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &status, sizeof(status));
				return PSA_SUCCESS;
			}
		}

		if (in_buf[0] == stl_reset) {
			ret = reset_test(&status, (STL_CpuTmxIndex_t)in_buf[1]);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &status, sizeof(status));
				return PSA_SUCCESS;
			}

		}
	}

	if ((in_sz == 8) && (out_sz == sizeof(status)) && (cfg_sz[0] != 0 )
	    && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_cfg) {
			if (sizeof(s_ram_cfg) < cfg_sz[0])
				return PSA_ERROR_INVALID_ARGUMENT;
			ret = config_test(&status, (STL_CpuTmxIndex_t)in_buf[1], msg);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &status, sizeof(status));
				return PSA_SUCCESS;
			}
		}
	}
	if ((in_sz == 4) && (out_sz == sizeof(list_status)) && (cfg_sz[0] != 0)
	    && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_init_all) {
			STL_TmListEnable_t list_enable;

			if (sizeof(list_enable) != cfg_sz[0])
				return PSA_ERROR_INVALID_ARGUMENT;
			psa_read(msg->handle, 1, &list_enable, cfg_sz[0]);

			ret = STL_SCH_InitAllTM(&list_status, &list_enable);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &list_status, sizeof(list_status));
				return PSA_SUCCESS;
			}
		}
	}

	if ((in_sz == 4) && (out_sz == sizeof(list_status))
	    && (cfg_sz[0] != 0 ) && (cfg_sz[1] != 0)) {
		if (in_buf[0] == stl_cfg_all) {
		        ret = config_all_test(&list_status, msg);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &list_status, sizeof(list_status));
				return PSA_SUCCESS;
			}
		}
	}

	if ((in_sz == 4) && (out_sz == sizeof(list_status))
	    && (cfg_sz[0] == 0 ) && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_run_all) {
			ret = STL_SCH_RunAllTM(&list_status);
			if (ret == STL_OK) {
				psa_write(msg->handle, 0, &list_status, sizeof(list_status));
				return PSA_SUCCESS;
			}
		}
	}

	if ((in_sz == 4) && (out_sz == 0) && (cfg_sz[0] == 0) && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_stopfail) {
			ret = STL_SCH_StopArtifFailing();
			if (ret == STL_OK) {
				return PSA_SUCCESS;
			}
		}
	}
	if ((in_sz == 4) && (out_sz == 0) && (cfg_sz[0] != 0) && (cfg_sz[1] == 0)) {
		if (in_buf[0] == stl_startfail) {
			psa_read(msg->handle, 1, &s_arti_fail_cfg, cfg_sz[0]);
			ret = STL_SCH_StartArtifFailing(&s_arti_fail_cfg);
			if (ret == STL_OK) {
				return PSA_SUCCESS;
			}
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
