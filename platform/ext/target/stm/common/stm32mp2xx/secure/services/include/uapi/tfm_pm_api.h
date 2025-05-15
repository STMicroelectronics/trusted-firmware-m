/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  TFM_PM_API_H
#define  TFM_PM_API_H

#include <psa/client.h>

/* PM message types that distinguish PM services. */
#define TFM_PM_SUSPEND		1001
#define TFM_PM_POWER_OFF	1002
#define TFM_PM_RESET		1003

/**
 * enum suspend mode
 * @PM_STOP2:		device is powered off
 * @PM_LP_STOP2:	device is suspended; needs to be woken up
 *			to receive a message.
 * @PM_LPLV_STOP2:	device is started (temporary state before running)
 * @PM_STANDBY1:	device is up and running
 */
enum pm_suspend_mode_t {
	PM_STOP2 = 0,
	PM_LP_STOP2,
	PM_LPLV_STOP2,
	PM_STANDBY1,
};

struct tfm_pm_suspend_args_t {
	enum pm_suspend_mode_t mode;
};

/**
 * @brief Power Management suspend.
 *
 * @return Returns values as specified by the psa_status_t
 */
psa_status_t psa_pm_suspend(enum pm_suspend_mode_t mode);

/**
 * @brief  Power Management power off.
 *
 * @return Returns values as specified by the psa_status_t
 */
psa_status_t psa_pm_power_off(void);

#endif /* TFM_PM_API_H */
