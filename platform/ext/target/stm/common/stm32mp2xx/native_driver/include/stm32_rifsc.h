/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_RIFSC_H
#define __STM32_RIFSC_H

#include <stdint.h>

int stm32_rifsc_get_access_by_id(const struct device *dev, uint32_t id);

#endif /* __STM32_RIFSC_H */
