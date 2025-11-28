/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <debug.h>
#include <errno.h>
#include <string.h>

#include <cpus.h>
#include <device.h>
#include <remoteproc.h>

struct cpu_info {
	const char *name;
	enum ctrl_method_t method;
	const struct device *dev_ctrl;
	const struct cpu_ctrl_api *ctrl_api;
	bool enable_at_startup;
};

struct cpu_ctrl_api {
	int (*cpu_start)(struct cpu_info *info);
	int (*cpu_stop)(struct cpu_info *info);
	int (*cpu_status)(struct cpu_info *info);
	int (*cpu_set_rsc_tab)(struct cpu_info *info, uint32_t addr, uint32_t size);
};

static __unused int _remoteproc_cpu_start(struct cpu_info *info)
{
	if (info->method != ENABLE_METHOD_REMOTEPROC)
		return -EINVAL;

	return rproc_boot(info->dev_ctrl);
}

static __unused int _remoteproc_cpu_stop(struct cpu_info *info)
{
	if (info->method != ENABLE_METHOD_REMOTEPROC)
		return -EINVAL;

	return rproc_shutdown(info->dev_ctrl);
}

static __unused int _remoteproc_cpu_status(struct cpu_info *info)
{
	if (info->method != ENABLE_METHOD_REMOTEPROC)
		return -EINVAL;

	return rproc_status(info->dev_ctrl);
}

static __unused int _remoteproc_cpu_set_rsc_tab(struct cpu_info *info,
						uint32_t addr, uint32_t size)
{
	if (info->method != ENABLE_METHOD_REMOTEPROC)
		return -EINVAL;

	return rproc_set_rsc_tab(info->dev_ctrl, addr, size);
}

static __unused const struct cpu_ctrl_api ctrl_remoteproc = {
	.cpu_start = _remoteproc_cpu_start,
	.cpu_stop = _remoteproc_cpu_stop,
	.cpu_status = _remoteproc_cpu_status,
	.cpu_set_rsc_tab = _remoteproc_cpu_set_rsc_tab,
};

#define EN_METHODE_NONE(node_id)							\
	{										\
		.name = DEVICE_DT_NAME(node_id),					\
		.method = ENABLE_METHOD_NONE,						\
		.enable_at_startup = DT_NODE_HAS_STATUS_OKAY(node_id),			\
	}

#define EN_METHODE_INVAL(node_id)							\
	{										\
		.name = DEVICE_DT_NAME(node_id),					\
		.method = ENABLE_METHOD_INVAL,						\
		.enable_at_startup = DT_NODE_HAS_STATUS_OKAY(node_id),			\
	}

#define EN_METHODE_REMOTEPROC(node_id)							\
	{										\
		.name = DEVICE_DT_NAME(node_id),					\
		.method = ENABLE_METHOD_REMOTEPROC,					\
		.dev_ctrl = DEVICE_DT_GET_OR_NULL(DT_RPROCS_CTLR(node_id)),		\
		.ctrl_api = &ctrl_remoteproc,						\
		.enable_at_startup = DT_NODE_HAS_STATUS_OKAY(node_id),			\
	}

#define DEFINE_COPRO_STATE(node_id)							\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, enable_method),				\
		    (COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, enable_method, remoteproc), \
				 (EN_METHODE_REMOTEPROC(node_id)),			\
				 (EN_METHODE_INVAL(node_id)))),				\
		    (EN_METHODE_NONE(node_id)))

static struct cpu_info cpu_infos[] = {
	DT_FOREACH_CHILD_SEP(DT_PATH(cpus), DEFINE_COPRO_STATE, (,))
};

bool cpu_is_valid(uint32_t id)
{
	return id >= ARRAY_SIZE(cpu_infos) ? false : true;
}

bool cpu_is_enable_method(uint32_t id)
{
	struct cpu_info *info;

	if (!cpu_is_valid(id))
		return false;

	info = &cpu_infos[id];
	if (info->method == ENABLE_METHOD_NONE ||
	    info->method == ENABLE_METHOD_INVAL ||
	    !info->ctrl_api)
		return false;

	return true;
}

static int cpus_init(void)
{
	uint32_t i, err = 0;

	/*
	 * enable at statup if:
	 * - enable metode is defined
	 * - status is okay and is not yet starting
	 */
	for(i = 0; i < ARRAY_SIZE(cpu_infos); i++) {
		struct cpu_info *info = &cpu_infos[i];

		if (!cpu_is_enable_method(i))
			continue;

		if (info->enable_at_startup) {
			if (info->ctrl_api->cpu_status(info) != CPU_RUNNING) {
				err = info->ctrl_api->cpu_start(info);
				if (err)
					EMSG("cpu:%s err:%d\n", info->name, err);
			}
		}
	}

	return err;
}
SYS_INIT(cpus_init, REST, 20);

enum tfm_platform_err_t cpu_service_info(struct cpu_serv_info *service_info_out)
{
	service_info_out->nb_cpu = ARRAY_SIZE(cpu_infos);
	return TFM_PLATFORM_ERR_SUCCESS;
}

enum tfm_platform_err_t cpu_get_info(uint32_t id, struct cpu_info_res *cpu_info_out)
{
	struct cpu_info *cpu;

	if (!cpu_is_valid(id))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	cpu = &cpu_infos[id];

	if (cpu_is_enable_method(id))
		cpu_info_out->status = cpu->ctrl_api->cpu_status(cpu);
	else
		cpu_info_out->status = cpu->enable_at_startup ? CPU_RUNNING : CPU_OFFLINE;

	cpu_info_out->method = cpu->method;
	strncpy(cpu_info_out->name, cpu->name, CPU_MAX_NAME_LEN);

	return TFM_PLATFORM_ERR_SUCCESS;
}

enum tfm_platform_err_t cpu_send_cmd(uint32_t id, enum tfm_cpu_service_type_t type,
				     struct cpu_cmd_res *cpu_cmd_out)
{
	struct cpu_info *cpu;
	int err = -EINVAL;

	if (!cpu_is_valid(id))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	cpu = &cpu_infos[id];

	switch (type) {
	case TFM_CPU_SERVICE_TYPE_START:
		err = cpu->ctrl_api->cpu_start(cpu);
		break;
	case TFM_CPU_SERVICE_TYPE_STOP:
		err = cpu->ctrl_api->cpu_stop(cpu);
		break;
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}

	cpu_cmd_out->status = cpu->ctrl_api->cpu_status(cpu);
	if (err)
		return TFM_PLATFORM_ERR_SYSTEM_ERROR;

	return TFM_PLATFORM_ERR_SUCCESS;
}

enum tfm_platform_err_t cpu_set_rsc_table(uint32_t id, uint32_t addr, uint32_t size)
{
	struct cpu_info *cpu;
	int err = -EINVAL;

	if (!cpu_is_valid(id))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	cpu = &cpu_infos[id];

	err = cpu->ctrl_api->cpu_set_rsc_tab(cpu, addr, size);
	if (err)
		return TFM_PLATFORM_ERR_SYSTEM_ERROR;
	return TFM_PLATFORM_ERR_SUCCESS;
}

enum tfm_platform_err_t cpus_service(const psa_invec *in_vec, const psa_outvec *out_vec)
{
	struct tfm_cpu_service_args_t *args;
	struct tfm_cpu_service_out_t *out = NULL;

	if (!in_vec || in_vec->len != sizeof(struct tfm_cpu_service_args_t) ||
	    (out_vec != NULL && out_vec->len != sizeof(struct tfm_cpu_service_out_t)))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	args = (struct tfm_cpu_service_args_t *)in_vec->base;
	if (!args)
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	if (out_vec != NULL) {
		out = (struct tfm_cpu_service_out_t *)out_vec->base;
		if (!out)
			return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	switch (args->type) {
	case TFM_CPU_SERVICE_TYPE_SERV_INFO:
		return cpu_service_info(&out->service_info);
	case TFM_CPU_SERVICE_TYPE_INFO:
		return cpu_get_info(args->cpu.id, &out->cpu_info);
	case TFM_CPU_SERVICE_TYPE_START:
	case TFM_CPU_SERVICE_TYPE_STOP:
		return cpu_send_cmd(args->cpu.id, args->type, &out->cpu_cmd);
	case TFM_CPU_SERVICE_TYPE_SET_RSC_TAB:
		return cpu_set_rsc_table(args->rsc_tab.id, args->rsc_tab.addr,
					 args->rsc_tab.size);
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}

	return TFM_PLATFORM_ERR_SUCCESS;
}
