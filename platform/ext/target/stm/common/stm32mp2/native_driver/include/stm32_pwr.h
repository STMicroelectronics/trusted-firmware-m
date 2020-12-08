/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_PWR_H
#define STM32_PWR_H

#include <stddef.h>

struct stm32_pwr_platdata {
	uintptr_t base;
};

void stm32_pwr_backupd_wp(bool enable);
int stm32_pwr_init(void);

#endif /* STM32_PWR_H */
