/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  __PLAT_DEVICE_H__
#define  __PLAT_DEVICE_H__

int stm32_platform_s_init(void);
int stm32_platform_bl2_init(void);
int stm32_platform_ns_init(void);

#endif   /* ----- #ifndef __PLAT_DEVICE_H__ ----- */

