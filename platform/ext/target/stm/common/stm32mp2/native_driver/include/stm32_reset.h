/*
 * Copyright (c) 2018-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __STM32_RESET_H__
#define __STM32_RESET_H__

#include <stdint.h>

/*
 * Assert target reset, if @to_us non null, wait until reset is asserted
 *
 * @reset_id: Reset controller ID
 * @to_us: Timeout in microsecond, or 0 if not waiting
 * Return 0 on success and -ETIMEDOUT if waiting and timeout expired
 */
int stm32_reset_assert_to(uint32_t reset_id, unsigned int to_us);

/*
 * Enable reset control for target resource
 *
 * @reset_id: Reset controller ID
 */
static inline void stm32_reset_set(uint32_t reset_id)
{
	(void)stm32_reset_assert_to(reset_id, 0);
}

/*
 * Deassert target reset, if @to_us non null, wait until reset is deasserted
 *
 * @reset_id: Reset controller ID
 * @to_us: Timeout in microsecond, or 0 if not waiting
 * Return 0 on success and -ETIMEDOUT if waiting and timeout expired
 */
int stm32_reset_deassert_to(uint32_t reset_id, unsigned int to_us);

/*
 * Release reset control for target resource
 *
 * @reset_id: Reset controller ID
 */
static inline void stm32_reset_release(uint32_t reset_id)
{
	(void)stm32_reset_deassert_to(reset_id, 0);
}

#endif /* __STM32_RESET_H__ */

