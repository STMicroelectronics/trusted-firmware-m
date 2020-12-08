/*
 * Copyright (c) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

static int stm32_reset_toggle(uint32_t id, unsigned int to_us, bool reset_status)
{
	return 0;
}

int stm32_reset_assert_to(uint32_t id, unsigned int to_us)
{
	return stm32_reset_toggle(id, to_us, true);
}

int stm32_reset_deassert_to(uint32_t id, unsigned int to_us)
{
	return stm32_reset_toggle(id, to_us, false);
}
