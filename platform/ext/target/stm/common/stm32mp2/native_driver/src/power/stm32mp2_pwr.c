/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_pwr

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <device.h>
#include <debug.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <reset.h>

#include <stm32mp2_pwr.h>

#define _PWR_CPU_MIN 1
#define _PWR_CPU_MAX 2

struct stm32mp2_pwr_config {
	uintptr_t base;
	const struct reset_control rst_ctl_bck;
};

static struct stm32_pwr_platdata pdata;

__attribute__((weak))
int stm32_pwr_get_platdata(struct stm32_pwr_platdata *pdata)
{
	return 0;
}

static uint32_t _cpux_base(uint32_t cpu)
{
	uint32_t offset = _PWR_CPU1D1SR;

	if (cpu < _PWR_CPU_MIN || cpu > _PWR_CPU_MAX)
		return 0;

	offset += sizeof(uint32_t) * (cpu - _PWR_CPU_MIN);
	return pdata.base + offset;
}

static int _cpu_state(uint32_t cpu, uint32_t *state)
{
	uint32_t cpux_base;

	cpux_base = _cpux_base(cpu);
	if (!cpux_base) {
		IMSG("cpu:%d not valid");
		return -1;
	}

	*state = mmio_read_32(cpux_base);
	return 0;
}

enum c_state stm32_pwr_cpu_get_cstate(uint32_t cpu)
{
	uint32_t state;

	if (_cpu_state(cpu, &state))
		return CERR;

	return _FLD_GET(_PWR_CPUXDXSR_CSTATE, state);
}

enum d_state stm32_pwr_cpu_get_dstate(uint32_t cpu)
{
	uint32_t state;

	if (_cpu_state(cpu, &state))
		return DERR;

	return _FLD_GET(_PWR_CPUXDXSR_DSTATE, state);
}

int stm32mp2_pwr_init(const struct device *dev)
{
	const struct stm32mp2_pwr_config *drv_cfg = dev_get_config(dev);
	uint32_t bdcr;

	/*
	 * Disable the backup domain write protection.
	 * The protection is enable at each reset by hardware
	 * and must be disabled by software.
	 */
	mmio_setbits_32(drv_cfg->base + _PWR_BDCR1, _PWR_BDCR1_DBD3P);
	mmio_read32_poll_timeout(drv_cfg->base + _PWR_BDCR1,
				 bdcr, (bdcr &  _PWR_BDCR1_DBD3P), 0);

	/* Reset backup domain on cold boot cases */
	reset_control_reset(&drv_cfg->rst_ctl_bck);

	return 0;
}

/*
 * FIXME:
 * when we add supply domain managment, we need to create a power domain
 * framework.
 */

#define STM32_PWR_INIT(n)						\
									\
static const struct stm32mp2_pwr_config stm32mp2_pwr_cfg_##n = {	\
	.base = DT_INST_REG_ADDR(n),					\
	.rst_ctl_bck = DT_INST_RESET_CONTROL_GET(n),			\
};									\
									\
DEVICE_DT_INST_DEFINE(n, &stm32mp2_pwr_init,				\
		      NULL, &stm32mp2_pwr_cfg_##n,			\
		      PRE_CORE, 1, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_PWR_INIT)
