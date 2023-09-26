/*
 * Copyright (C) 2023, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <reset.h>

static inline int _status(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = dev->api;

	if (api->status == NULL)
		return -ENOSYS;

	return api->status(dev, id);
}

int reset_control_status(struct reset_control *rstc)
{
	if (!rstc || !rstc->dev)
		return -ENODEV;

	return _status(rstc->dev, rstc->id);
}

static inline int _assert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = dev->api;

	if (api->assert_level == NULL)
		return -ENOSYS;

	return api->assert_level(dev, id);
}

int reset_control_assert(struct reset_control *rstc)
{
	if (!rstc || !rstc->dev)
		return -ENODEV;

	return _assert(rstc->dev, rstc->id);
}

static inline int _deassert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = dev->api;

	if (api->deassert_level == NULL)
		return -ENOSYS;

	return api->deassert_level(dev, id);
}

int reset_control_deassert(struct reset_control *rstc)
{
	if (!rstc || !rstc->dev)
		return -ENODEV;

	return _deassert(rstc->dev, rstc->id);
}

static inline int _reset(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = dev->api;

	if (api->reset == NULL) {
		return -ENOSYS;
	}

	return api->reset(dev, id);
}

int reset_control_reset(const struct reset_control *rstc)
{
	if (!rstc || !rstc->dev)
		return -ENODEV;

	return _reset(rstc->dev, rstc->id);
}
