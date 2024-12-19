/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */

#ifndef TFM_SCMI_H
#define TFM_SCMI_H
#define TFM_SCMI_OUT_OF_MEMORY -2
#define TFM_SCMI_INVAL_PARAM -1
#define TFM_SCMI_SUCCESS 0

int32_t scmi_server_initialize(void);
int32_t scmi_scpfw_cfg_early_init(void);
int32_t scmi_scpfw_cfg_init(void);

#endif /* TFM_SCMI_H */
