// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * inspired by the linux remoteproc framework
 */
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <errno.h>
#include <lib/utils_def.h>

#include <device.h>
#include <remoteproc.h>
#include <cpus.h>

static struct rproc_spec *_is_valid_rproc(const struct device *dev)
{
	const struct remoteproc_driver_api *api;

	if(!device_is_ready(dev))
		return NULL;

	api = dev->api;

	if (!api || !api->get_rproc)
		return NULL;

	return api->get_rproc(dev);
}

void rproc_init(const struct device *dev, struct rproc_spec *rproc)
{
	rproc->dev = dev;
	rproc->state = CPU_OFFLINE;
	atomic_init(&rproc->use_count, 0);
}

/*
 * remoteproc can manage several user.
 * Use an atomic counter to start on first
 * and stop on the last.
 * Warning: state and boot/shutdown is not protected by lock
 */
int rproc_boot(const struct device *dev)
{
	const struct remoteproc_driver_api *api;
	struct rproc_spec *rproc = _is_valid_rproc(dev);
	int err;

	if (!rproc)
		return -ENODEV;

	api = rproc->dev->api;

	if (rproc->state == CPU_CRASHED)
		return -EINVAL;

	/* skip the boot process if rproc is already powered up */
	if (atomic_fetch_add(&rproc->use_count, 1) > 1)
		return 0;

	err = api->start(rproc);
	if (err) {
		EMSG("can't start rproc: %d", err);
		atomic_fetch_sub(&rproc->use_count, 1);
		return err;
	}

	rproc->state = CPU_RUNNING;

	return 0;
}

int rproc_shutdown(const struct device *dev)
{
	const struct remoteproc_driver_api *api;
	struct rproc_spec *rproc = _is_valid_rproc(dev);
	int err;

	if (!rproc)
		return -ENODEV;

	api = rproc->dev->api;

	if (rproc->state == CPU_OFFLINE)
		return 0;

	/* skip the boot process if rproc is already powered dow */
	if (!atomic_fetch_sub(&rproc->use_count, 1))
		return 0;

	err = api->stop(rproc);
	if (err) {
		EMSG("can't stop rproc: %d", err);
		atomic_fetch_add(&rproc->use_count, 1);
		return err;
	}

	rproc->state = CPU_OFFLINE;

	return 0;
}

int rproc_status(const struct device *dev)
{
	struct rproc_spec *rproc = _is_valid_rproc(dev);

	if (!rproc)
		return -EINVAL;

	return rproc->state;
}
