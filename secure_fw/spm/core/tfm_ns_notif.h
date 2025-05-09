/*
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Definitions of  Non Secure Notification API Raised by Secure Partition to Non Secure
 */

#ifndef __TFM_NS_NOTIF_H__
#define __TFM_NS_NOTIF_H__

#include <stdint.h>
#include "cmsis_compiler.h"
#include "psa/client.h"

/*  handler to be called from a filh handler */
psa_status_t tfm_ns_notif_flih(uint32_t event);
psa_status_t ns_notif(uint32_t event);
int32_t tfm_ns_notification_init_s(void *area, size_t area_size);
psa_status_t tfm_ns_notif(uint32_t event);

#endif /* __TFM_NS_NOTIF_H__ */
