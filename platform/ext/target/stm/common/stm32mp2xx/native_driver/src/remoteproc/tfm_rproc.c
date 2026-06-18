// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 */
#define DT_DRV_COMPAT tfm_rproc

#include <cmsis.h>
#include <debug.h>
#include <device.h>
#include <irq.h>
#include <lib/timeout.h>
#include <lib/utils_def.h>
#include <remoteproc.h>
#include <stddef.h>
#include <stdio.h>
#include <uapi/tfm_ioctl_api.h>

#define N_STATUS_RETRY 3

struct tfm_rproc_config {
	const struct irq_spec *ispec_wdog;
	uint32_t cpu_id;
};

struct tfm_rproc_data {
	struct rproc_spec rproc;
	int32_t active_status;
};

typedef enum tfm_platform_err_t (*cpu_cmd_fn_t)(uint32_t, int32_t *);

static int _tfm_rproc_get_cpu_info(uint32_t cpu_id, struct cpu_info_res *cpu_info)
{
	if (tfm_platform_cpu_info(cpu_id, cpu_info) != TFM_PLATFORM_ERR_SUCCESS)
		return -ENOTSUP;

	if (cpu_info->status < 0 || cpu_info->status >= CPU_LAST)
		return -ENOTSUP;

	return 0;
}

static int _tfm_rproc_cpu_cmd(const struct device *dev, cpu_cmd_fn_t cpu_fn,
			      uint32_t cpu_id, int status_expected)
{
	struct cpu_info_res cpu_info;
	enum tfm_platform_err_t tfm_err;
	int32_t status, err;

	if (!cpu_fn)
		return -EINVAL;

	err = _tfm_rproc_get_cpu_info(cpu_id, &cpu_info);
	if (err) {
		EMSG("%s: get cpu %d info fail: %d\r\n", dev->name, cpu_id, err);
		return err;
	}

	if (cpu_info.status == status_expected) {
		DMSG("%s: cpu %s is already %s\r\n", dev->name, cpu_info.name,
		     cpu_status_str[cpu_info.status]);
		return 0;
	}

	tfm_err = cpu_fn(cpu_id, &status);
	if (tfm_err != TFM_PLATFORM_ERR_SUCCESS) {
		EMSG("%s: cpu %s cmd fail: %d\r\n", dev->name, cpu_info.name, tfm_err);
		return -EIO;
	}

	return 0;
}

static int tfm_rproc_start(struct rproc_spec *rproc)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(rproc->dev);

	return _tfm_rproc_cpu_cmd(rproc->dev, tfm_platform_cpu_start,
				  dev_cfg->cpu_id, CPU_STARTED);
}

static int tfm_rproc_stop(struct rproc_spec *rproc)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(rproc->dev);

	return _tfm_rproc_cpu_cmd(rproc->dev, tfm_platform_cpu_stop,
				  dev_cfg->cpu_id, CPU_OFFLINE);
}

static bool tfm_rproc_is_running(struct rproc_spec *rproc)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(rproc->dev);
	struct cpu_info_res cpu_info;

	if (_tfm_rproc_get_cpu_info(dev_cfg->cpu_id, &cpu_info))
		return false;

	return cpu_info.status == CPU_RUNNING;
}

static int tfm_rproc_suspend(struct rproc_spec *rproc)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(rproc->dev);
	struct tfm_rproc_data *dev_data = dev_get_data(rproc->dev);
	struct cpu_info_res cpu_info;
	int err;

	err = _tfm_rproc_get_cpu_info(dev_cfg->cpu_id, &cpu_info);
	if (err) {
		EMSG("%s: get cpu %d info fail: %d\r\n",
		     rproc->dev->name, dev_cfg->cpu_id, err);
		return -ENOTSUP;
	}

	dev_data->active_status = cpu_info.status;

	return _tfm_rproc_cpu_cmd(rproc->dev, tfm_platform_cpu_suspend,
				  dev_cfg->cpu_id, CPU_SUSPENDED);
}

static int tfm_rproc_resume(struct rproc_spec *rproc)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(rproc->dev);
	struct tfm_rproc_data *dev_data = dev_get_data(rproc->dev);

	return _tfm_rproc_cpu_cmd(rproc->dev, tfm_platform_cpu_resume,
				  dev_cfg->cpu_id, dev_data->active_status);
}

static irqreturn_t __maybe_unused tfm_rproc_wdog(void *data)
{
	const struct device *dev = data;

	rproc_report_crash(dev, RPROC_WATCHDOG);

	return IRQ_HANDLED;
}

static struct rproc_spec *tfm_rproc_get(const struct device *dev)
{
	struct tfm_rproc_data *dev_data = dev_get_data(dev);

	return &dev_data->rproc;
}

static __unused int tfm_rproc_init(const struct device *dev)
{
	const struct tfm_rproc_config *dev_cfg = dev_get_config(dev);
	struct tfm_rproc_data *dev_data = dev_get_data(dev);
	int err = 0;

	if (dev_cfg->ispec_wdog) {
		err = interrupt_request(dev_cfg->ispec_wdog,
					(void *)dev, tfm_rproc_wdog, IRQF_NONE);
		if (err) {
			EMSG("%s: interrupt request fail: %d\r\n", dev->name, err);
			return err;
		}
	}

	rproc_init(dev, &dev_data->rproc);

	return 0;
}

static struct remoteproc_driver_api tfm_rproc_api = {
	.get_rproc = tfm_rproc_get,
	.start = tfm_rproc_start,
	.stop = tfm_rproc_stop,
	.is_running = tfm_rproc_is_running,
	.suspend = tfm_rproc_suspend,
	.resume = tfm_rproc_resume,
};

#define TFM_RPROC_INIT(n)							\
										\
DT_INST_IRQS_SPEC_DEFINE(n)							\
										\
static const struct tfm_rproc_config _tfm_rproc_cfg##n = {			\
	.ispec_wdog = DT_INST_IRQS_SPEC_GET_BY_NAME(n, wdog),			\
	.cpu_id = DT_INST_PROP(n, tfm_cpu_id),					\
};										\
										\
static struct tfm_rproc_data _tfm_rproc_data##n = {				\
};										\
										\
DEVICE_DT_INST_DEFINE(n, &tfm_rproc_init, NULL,					\
		      &_tfm_rproc_data##n, &_tfm_rproc_cfg##n,			\
		      CORE, 9, &tfm_rproc_api);

DT_INST_FOREACH_STATUS_OKAY(TFM_RPROC_INIT)
