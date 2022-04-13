/*
 * Copyright (c) 2018-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __STM32_RESET_H__
#define __STM32_RESET_H__

#include <stdint.h>
#include <rstctrl.h>

struct stm32_reset_platdata {
	uintptr_t base;
};

/* Exposed rstctrl instance */
struct stm32_rstline {
	unsigned int id;
	struct rstctrl rstctrl;
};

int stm32_reset_init(void);
int stm32_reset_get_platdata(struct stm32_reset_platdata *pdata);

#define STM32_RSTCTRL_REF(__id) (struct rstctrl *) &(stm32_rstline_##__id.rstctrl)

#define DECLARE_STM32_RSTLINE(__id)					\
	extern const struct stm32_rstline stm32_rstline_##__id

#define DEFINE_STM32_RSTLINE(__id, __ops)				\
	const struct stm32_rstline stm32_rstline_##__id = {		\
		.id = __id,						\
		.rstctrl = {						\
			.ops = __ops					\
		},							\
	}

#endif /* __STM32_RESET_H__ */
