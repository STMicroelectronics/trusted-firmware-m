/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_PLAT_NS_H__
#define __TFM_PLAT_NS_H__

/**
 * \brief Platform peripherals and devices initialization.
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
*/

int32_t tfm_ns_platform_init(void);

/**
 * \brief Coprocessor initialization.
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
*/
int32_t tfm_ns_cp_init(void);

/**
 * \brief platform post init for drivers relying on operating system
 * function
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
*/
int32_t tfm_ns_platform_post_init(void);

/**
 * \brief ns platform function for executing platform specific secure service
 * in a dedicated non secure context
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
*/
int32_t tfm_ns_platform_sec_ctx_call(void);

#endif /* __TFM_PLAT_NS_H__ */
