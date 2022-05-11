/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_BSEC3_H
#define STM32_BSEC3_H

#include <stddef.h>
#include <stdbool.h>

#define DBG_FULL 0xFFF

struct stm32_bsec_platdata {
	uintptr_t base;
};

void stm32_bsec_write_debug_conf(uint32_t val);
int stm32_bsec_init(void);

#endif /* STM32_BSEC3_H */
