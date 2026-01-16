/*
 * Copyright (c) 2026, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_PLAT_FWU_BL2_H__
#define __TFM_PLAT_FWU_BL2_H__

#include <stdint.h>
#include <stddef.h>
#include "tfm_plat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initialize the platform resources used
 *        for TF-M FWU notifications. This
 *        function sets up any elements required
 *        to later notify the platform about
 *        firmware update events (for example
 *        error or erase notifications).
 *        It is typically called once during the
 *        boot process before any FWU-related
 *        notifications are issued.
 *
 * \return  This function does not return a value.
 *          Any failure in the initialization must
 *          be handled internally by the platform.
 */
void tfm_plat_bl2_notify_init(void);

/**
 * \brief Notify the platform that something
 *        went wrong during TF-M FWU. This
 *        function can be useful on asymmetric
 *        platforms where the firmware update
 *        (FWU) is handled by a different
 *        bootloader. It allows the platform to be
 *        informed that an error occurred and that
 *        a flash erase may be required.
 *
 * \return If the notification fails, the erase
 *         operation must still be performed.
 */
void tfm_plat_bl2_notify_erase(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_PLAT_FWU_BL2_H__ */
