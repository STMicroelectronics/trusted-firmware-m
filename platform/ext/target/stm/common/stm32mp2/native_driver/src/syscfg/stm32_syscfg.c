/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>

#include <debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <stm32_syscfg.h>

/* BSEC REGISTER OFFSET */
#define _SYSCFG_SAFERSTCR		U(0x2018)

#define _SYSCFG_DEVICEID		U(0x6400)
#define _SYSCFG_VERR			U(0x7FF4)
#define _SYSCFG_IPIDR			U(0x7FF8)
#define _SYSCFG_SIDR			U(0x7FFC)

static struct stm32_syscfg_platdata pdata;

__attribute__((weak))
int stm32_syscfg_get_platdata(struct stm32_syscfg_platdata *pdata)
{
	return 0;
}

uint32_t stm32_syscfg_read(uint32_t offset)
{
	if (offset >= _SYSCFG_DEVICEID)
		return 0;

	return mmio_read_32(pdata.base + offset);
}

void stm32_syscfg_write(uint32_t offset, uint32_t val)
{
	if (offset >= _SYSCFG_DEVICEID)
		return;

	mmio_write_32(pdata.base + offset, val);
}

void stm32_syscfg_safe_rst(bool enable)
{
	uint32_t val = enable ? 0x1 : 0x0;

	stm32_syscfg_write(_SYSCFG_SAFERSTCR, val);
}

int stm32_syscfg_init(void)
{
	int err;

	err = stm32_syscfg_get_platdata(&pdata);
	if (err)
		return err;

	return 0;
}
