/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  TFM_PARTITION_PM_H
#define  TFM_PARTITION_PM_H

psa_status_t tfm_pm_fw_init(void);
psa_status_t tfm_pm_suspend(const psa_msg_t *msg);
psa_status_t tfm_pm_power_off(void);

#endif /* TFM_PARTITION_PM_H */
