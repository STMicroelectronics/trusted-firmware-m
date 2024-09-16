// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <firewall.h>
#include <remoteproc.h>
#include <reset.h>
#include <cmsis.h>

struct stm32_rproc_variant {
	int (*init_fn)(const struct device *dev);
	int (*start_fn)(const struct device *dev);
	int (*stop_fn)(const struct device *dev);
};

struct stm32_rproc_config {
	const struct reset_control rst_ctl;
	const struct firewall_spec *fwall;
	const int n_fwall;
};

struct stm32_rproc_data {
	struct rproc_spec rproc;
	const struct stm32_rproc_variant *variant;
};

/*
 * the cortexA is in WFI and all exti are masked.
 * so before send event we must:
 * - unmask cpu2_sev (exti1 64) of cortexA (C1)
 * - apply rif access on exti (exti driver)
 * - send event by exti software interrupt
 */
static __unused int stm32mp2_a35_init(const struct device *dev)
{
	/* workaround while waiting interrupt framework and exti driver */
	EXTI1->C1IMR3 |= 1;

	return 0;
}

static __unused int stm32mp2_a35_start(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	int err;

	err = reset_control_assert(&cfg->rst_ctl);
	if (!err)
		return err;

	EXTI1->SWIER3 |=1;

	return reset_control_deassert(&cfg->rst_ctl);
}

static __unused int stm32mp2_a35_stop(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);

	return reset_control_assert(&cfg->rst_ctl);
}

static struct rproc_spec *stm32_rproc_get(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);

	return &data->rproc;
}

int stm32_rproc_start(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->start_fn)
		return -ENOTSUP;

	return data->variant->start_fn(rproc->dev);
}

int stm32_rproc_stop(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->stop_fn)
		return -ENOTSUP;

	return data->variant->stop_fn(rproc->dev);
}

static __unused int stm32_rproc_init(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);
	uint32_t err = 0;

	if (data->variant->init_fn)
		err = data->variant->init_fn(dev);

	rproc_init(dev, &data->rproc);

	return err;
}

static __unused const struct  stm32_rproc_variant stm32mp2_a35_var = {
	.init_fn = stm32mp2_a35_init,
	.start_fn = stm32mp2_a35_start,
	.stop_fn = stm32mp2_a35_stop,
};

static struct remoteproc_driver_api stm32_rproc_api = {
	.get_rproc = stm32_rproc_get,
	.start = stm32_rproc_start,
	.stop = stm32_rproc_stop,
};

#define STM32_RPROC_INIT(n, name, _variant)				\
									\
DT_INST_ACCESS_CTRLS_DEFINE(n);						\
									\
static const struct stm32_rproc_config _##name##_cfg##n = {		\
	.rst_ctl = DT_INST_RESET_CONTROL_GET(n),			\
	.fwall = DT_INST_ACCESS_CTRLS_GET(n),				\
	.n_fwall = DT_INST_ACCESS_CTRLS_NUM(n),				\
};									\
									\
static struct stm32_rproc_data _##name##_data##n = {			\
	.variant = &_variant,						\
};									\
									\
DEVICE_DT_INST_DEFINE(n, &stm32_rproc_init,				\
		      &_##name##_data##n, &_##name##_cfg##n,		\
		      CORE, 9, &stm32_rproc_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp2_a35

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RPROC_INIT,
				  DT_DRV_COMPAT, stm32mp2_a35_var)
