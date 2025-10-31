/*
 * Copyright (c) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <device.h>
#include <errno.h>
#include <iterable_sections.h>
#include <tfm_sp_log.h>
#include <stdint.h>
#include <utilities.h>

#include <pm/device.h>

TYPE_SECTION_START_EXTERN(const struct device *, pm_device_slots);

/* Number of devices successfully suspended. */
static size_t num_susp;

static const enum pm_device_state action_target_state[] = {
	[PM_DEVICE_ACTION_SUSPEND] = PM_DEVICE_STATE_SUSPENDED,
	[PM_DEVICE_ACTION_RESUME] = PM_DEVICE_STATE_ACTIVE,
};

int pm_device_action_run(const struct device *dev, enum pm_device_action action, uint32_t pm_hint)
{
	struct pm_device *pm = dev->pm;
	int err;

	if (pm == NULL)
		return -ENOSYS;

	if (pm->state == action_target_state[action])
		return -EALREADY;

	err = pm->action_cb(dev, action, pm_hint);
	if (err)
		return err;

	pm->state = action_target_state[action];

	return 0;
}

bool pm_suspend_devices(uint32_t pm_hint)
{
	const struct device *devs;
	size_t devc;

	devc = device_get_all(&devs);

	num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		/*
		 * Ignore uninitialized devices, busy devices, wake up sources, and
		 * devices with runtime PM enabled.
		 */
		if (!device_is_ready(dev))
			continue;

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND, pm_hint);
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERRFMT("Device %s suspend (hint: 0x%x) error: %d\n",
				   dev->name, pm_hint, ret);
			return false;
		}

		TYPE_SECTION_START(pm_device_slots)[num_susp] = dev;
		num_susp++;
	}

	return true;
}

void pm_resume_devices(uint32_t pm_hint)
{
	int ret, nb_err = 0;

	for (int i = (num_susp - 1); i >= 0; i--) {
		const struct device *dev = TYPE_SECTION_START(pm_device_slots)[i];

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME, pm_hint);

		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY))
			continue;
		else if (ret < 0) {
			LOG_ERRFMT("Device %s resume (hint: 0x%x) error: %d\n",
				   dev->name, pm_hint, ret);
			nb_err++;
		}
	}

	if (nb_err) {
		LOG_ERRFMT("resume devices fail\n");
		tfm_core_panic();
	}

	num_susp = 0;
}
