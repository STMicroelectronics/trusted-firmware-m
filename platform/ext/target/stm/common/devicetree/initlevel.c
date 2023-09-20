/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <device.h>

extern const struct init_entry __init_start[];
extern const struct init_entry __init_EARLY_start[];
extern const struct init_entry __init_PRE_CORE_start[];
extern const struct init_entry __init_CORE_start[];
extern const struct init_entry __init_POST_CORE_start[];
extern const struct init_entry __init_REST_start[];
extern const struct init_entry __init_end[];

void sys_init_run_level(enum init_level level)
{
	static const struct init_entry *levels[] = {
		__init_EARLY_start,
		__init_PRE_CORE_start,
		__init_CORE_start,
		__init_POST_CORE_start,
		__init_REST_start,
		/* End marker */
		__init_end,
	};
	const struct init_entry *entry;

	for (entry = levels[level]; entry < levels[level + 1]; entry++) {
		const struct device *dev = entry->dev;

		/*
		 * If the init entry belongs to a device, run init function
		 * with its reference, otherwise it is a system init.
		 */
		if (dev == NULL) {
			(void)entry->init_fn.sys();
			continue;
		}

		/*
		 * Mark device initialized. If initialization
		 * failed, record the error condition.
		 */
		if (entry->init_fn.dev != NULL)
			dev->state->init_res = entry->init_fn.dev(dev);

		dev->state->initialized = true;
	}
}

bool device_is_ready(const struct device *dev)
{
	/*
	 * if an invalid device pointer is passed as argument, this call
	 * reports the `device` as not ready for usage.
	 */
	if (dev == NULL) {
		return false;
	}

	return dev->state->initialized && (dev->state->init_res == 0U);
}
