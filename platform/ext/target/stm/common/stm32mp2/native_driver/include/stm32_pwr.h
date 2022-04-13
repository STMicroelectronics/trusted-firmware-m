/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_PWR_H
#define STM32_PWR_H

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

struct stm32_pwr_platdata {
	uintptr_t base;
};

enum c_state {
	CERR = INT_MIN,
	CRESET = 0,
	CRUN = 1,
	CSLEEP = 2,
	CSTOP = 3
};

enum d_state {
	DERR = INT_MIN,
	DRUN = 0,
	DSTOP1 = 1,
	DSTOP2 = 2,
	DSTOP3 = 3,
	DSTANDBY = 4,
};

enum c_state stm32_pwr_cpu_get_cstate(uint32_t cpu);
enum d_state stm32_pwr_cpu_get_dstate(uint32_t cpu);

void stm32_pwr_backupd_wp(bool enable);
int stm32_pwr_init(void);

#endif /* STM32_PWR_H */
