/*
 * Copyright (c) 2021-2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP2_DDR_HELPERS_H
#define STM32MP2_DDR_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

#include <stm32mp2_ddr_regs.h>

enum stm32mp2_ddr_sr_mode {
	DDR_SR_MODE_INVALID = 0,
	DDR_SSR_MODE,
	DDR_HSR_MODE,
	DDR_ASR_MODE,
};

void ddr_activate_controller(struct stm32mp_ddrctl *ctl);
bool is_ddr_cid_filtering_enabled(void);
void ddr_enable_cid_filtering(void);
void ddr_disable_cid_filtering(void);
enum stm32mp2_ddr_sr_mode ddr_read_sr_mode(void);
void ddr_set_sr_mode(enum stm32mp2_ddr_sr_mode mode);

#endif /* STM32MP2_DDR_HELPERS_H */
