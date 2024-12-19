/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>

#include <tfm_platform_api.h>
#include <uapi/tfm_ioctl_api.h>

enum tfm_platform_err_t tfm_platform_cpu_service_info(struct cpu_serv_info *serv_info)
{
	struct tfm_cpu_service_args_t args;
	struct tfm_cpu_service_out_t out;
	enum tfm_platform_err_t ret;
	psa_outvec out_vec;
	psa_invec in_vec;

	in_vec.base = (const void *)&args;
	in_vec.len = sizeof(args);

	out_vec.base = (void *)&out;
	out_vec.len = sizeof(out);

	args.type = TFM_CPU_SERVICE_TYPE_SERV_INFO;

	ret = tfm_platform_ioctl(TFM_PLATFORM_IOCTL_CPU_SERVICE, &in_vec, &out_vec);
	memcpy(serv_info, &out.service_info, sizeof(struct cpu_serv_info));

	return ret;
}

enum tfm_platform_err_t tfm_platform_cpu_info(uint32_t cpu_id, struct cpu_info_res *cpu_info)
{
	struct tfm_cpu_service_args_t args;
	struct tfm_cpu_service_out_t out;
	enum tfm_platform_err_t ret;
	psa_outvec out_vec;
	psa_invec in_vec;

	in_vec.base = (const void *)&args;
	in_vec.len = sizeof(args);

	out_vec.base = (void *)&out;
	out_vec.len = sizeof(out);

	args.type = TFM_CPU_SERVICE_TYPE_INFO;
	args.cpu.id = cpu_id;

	ret = tfm_platform_ioctl(TFM_PLATFORM_IOCTL_CPU_SERVICE, &in_vec, &out_vec);
	memcpy(cpu_info, &out.cpu_info, sizeof(struct cpu_info_res));

	return ret;
}

enum tfm_platform_err_t tfm_platform_cpu_cmd(enum tfm_cpu_service_type_t type,
					     uint32_t cpu_id, int32_t *status)
{
	struct tfm_cpu_service_args_t args;
	struct tfm_cpu_service_out_t out;
	enum tfm_platform_err_t ret;
	psa_outvec out_vec;
	psa_invec in_vec;

	in_vec.base = (const void *)&args;
	in_vec.len = sizeof(args);

	out_vec.base = (void *)&out;
	out_vec.len = sizeof(out);

	args.type = type;
	args.cpu.id = cpu_id;

	ret = tfm_platform_ioctl(TFM_PLATFORM_IOCTL_CPU_SERVICE, &in_vec, &out_vec);
	*status = out.cpu_cmd.status;

	return ret;
}

enum tfm_platform_err_t tfm_platform_cpu_start(uint32_t cpu_id, int32_t *status)
{
	return tfm_platform_cpu_cmd(TFM_CPU_SERVICE_TYPE_START, cpu_id, status);
}

enum tfm_platform_err_t tfm_platform_cpu_stop(uint32_t cpu_id, int32_t *status)
{
	return tfm_platform_cpu_cmd(TFM_CPU_SERVICE_TYPE_STOP, cpu_id, status);
}
