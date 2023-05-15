/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <stm32_bsec3.h>

static struct stm32_bsec_platdata pdata;

/* BSEC REGISTER OFFSET */
#define _BSEC_FVR			U(0x000)
#define _BSEC_SR			U(0xE40)
#define _BSEC_OTPCR			U(0xC04)
#define _BSEC_WDR			U(0xC08)
#define _BSEC_OTPSR			U(0xE44)
#define _BSEC_LOCKR			U(0xE10)
#define _BSEC_DENR			U(0xE20)
#define _BSEC_SFSR			U(0x940)
#define _BSEC_OTPVLDR			U(0x8C0)
#define _BSEC_SPLOCK			U(0x800)
#define _BSEC_SWLOCK			U(0x840)
#define _BSEC_SRLOCK			U(0x880)
#define _BSEC_SCRATCHR0			U(0xE00)
#define _BSEC_SCRATCHR1			U(0xE04)
#define _BSEC_SCRATCHR2			U(0xE08)
#define _BSEC_SCRATCHR3			U(0xE0C)
#define _BSEC_JTAGINR			U(0xE14)
#define _BSEC_JTAGOUTR			U(0xE18)
#define _BSEC_UNMAPR			U(0xE24)
#define _BSEC_WRCR			U(0xF00)
#define _BSEC_HWCFGR			U(0xFF0)
#define _BSEC_VERR			U(0xFF4)
#define _BSEC_IPIDR			U(0xFF8)
#define _BSEC_SIDR			U(0xFFC)

/* BSEC_LOCKR register fields */
#define _BSEC_LOCKR_GWLOCK_MASK		BIT(0)

/* BSEC_DENR register fields */
#define _BSEC_DENR_ALL_MSK		GENMASK(15, 0)

#define _BSEC_DENR_KEY			0xDEB60000

static struct stm32_bsec_platdata pdata;

__attribute__((weak))
int stm32_bsec_get_platdata(struct stm32_bsec_platdata *pdata)
{
	return -ENODEV;
}

static void bsec_lock(void)
{
	/* Not yet available */
	return;
}

static void bsec_unlock(void)
{
	/* Not yet available */
	return;
}

static bool is_bsec_write_locked(void)
{
	return (mmio_read_32(pdata.base + _BSEC_LOCKR) &
		_BSEC_LOCKR_GWLOCK_MASK) != 0U;
}

void stm32_bsec_write_debug_conf(uint32_t val)
{
	uint32_t masked_val = val & _BSEC_DENR_ALL_MSK;

	if (is_bsec_write_locked() == true) {
		panic();
	}

	bsec_lock();
	mmio_write_32(pdata.base + _BSEC_DENR, _BSEC_DENR_KEY | masked_val);
	bsec_unlock();
}

int stm32_bsec_init(void)
{
	int err;

	err = stm32_bsec_get_platdata(&pdata);
	if (err)
		return err;

	return 0;
}
