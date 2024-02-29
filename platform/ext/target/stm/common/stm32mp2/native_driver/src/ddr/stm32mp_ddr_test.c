/*
 * Copyright (C) 2022-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>
#include <stm32mp_ddr.h>
#include <stm32mp_ddr_test.h>
#include <lib/mmio.h>
#include <cmsis.h>

#define DDR_PATTERN	0xAAAAAAAAU
#define DDR_ANTIPATTERN	0x55555555U

/*******************************************************************************
 * This function tests a simple read/write access to the DDR.
 * Note that the previous content is restored after test.
 * Returns 0 if success, and address value else.
 ******************************************************************************/
uintptr_t stm32mp_ddr_test_rw_access(struct stm32mp_ddr_size *info)
{
	uint32_t saved_value = mmio_read_32(info->base);

	mmio_write_32(info->base, DDR_PATTERN);

	if (mmio_read_32(info->base) != DDR_PATTERN) {
		return info->base;
	}

	mmio_write_32(info->base, saved_value);

	return 0UL;
}

/*******************************************************************************
 * This function tests the DDR data bus wiring.
 * This is inspired from the Data Bus Test algorithm written by Michael Barr
 * in "Programming Embedded Systems in C and C++" book.
 * resources.oreilly.com/examples/9781565923546/blob/master/Chapter6/
 * File: memtest.c - This source code belongs to Public Domain.
 * Returns 0 if success, and address value else.
 ******************************************************************************/
uintptr_t stm32mp_ddr_test_data_bus(struct stm32mp_ddr_size *info)
{
	uint32_t pattern;

	for (pattern = 1U; pattern != 0U; pattern <<= 1U) {
		mmio_write_32(info->base, pattern);

		if (mmio_read_32(info->base) != pattern) {
			return info->base;
		}
	}

	return 0UL;
}

/*******************************************************************************
 * This function tests the DDR address bus wiring.
 * This is inspired from the Data Bus Test algorithm written by Michael Barr
 * in "Programming Embedded Systems in C and C++" book.
 * resources.oreilly.com/examples/9781565923546/blob/master/Chapter6/
 * File: memtest.c - This source code belongs to Public Domain.
 * size: size in bytes of the DDR memory device.
 * Returns 0 if success, and address value else.
 ******************************************************************************/
uintptr_t stm32mp_ddr_test_addr_bus(struct stm32mp_ddr_size *info)
{
	size_t addressmask = info->size - 1U;
	size_t offset;
	size_t testoffset = 0U;

	/* Write the default pattern at each of the power-of-two offsets. */
	for (offset = sizeof(uint32_t); (offset & addressmask) != 0U;
	     offset <<= 1U) {
		mmio_write_32(info->base + offset, DDR_PATTERN);
	}

	/* Check for address bits stuck high. */
	mmio_write_32(info->base + testoffset, DDR_ANTIPATTERN);

	for (offset = sizeof(uint32_t); (offset & addressmask) != 0U;
	     offset <<= 1U) {
		if (mmio_read_32(info->base + offset) != DDR_PATTERN) {
			return info->base + offset;
		}
	}

	mmio_write_32(info->base + testoffset, DDR_PATTERN);

	/* Check for address bits stuck low or shorted. */
	for (testoffset = sizeof(uint32_t); (testoffset & addressmask) != 0U;
	     testoffset <<= 1U) {
		mmio_write_32(info->base + testoffset, DDR_ANTIPATTERN);

		if (mmio_read_32(info->base) != DDR_PATTERN) {
			return info->base;
		}

		for (offset = sizeof(uint32_t); (offset & addressmask) != 0U;
		     offset <<= 1) {
			if ((mmio_read_32(info->base + offset) != DDR_PATTERN) &&
			    (offset != testoffset)) {
				return info->base + offset;
			}
		}

		mmio_write_32(info->base + testoffset, DDR_PATTERN);
	}

	return 0UL;
}

/*******************************************************************************
 * This function checks the DDR size. It has to be run with Data Cache off.
 * This test is run before data have been put in DDR, and is only done for
 * cold boot. The DDR data can then be overwritten, and it is not useful to
 * restore its content.
 * Returns DDR computed size.
 ******************************************************************************/
size_t stm32mp_ddr_check_size(struct stm32mp_ddr_size *info, size_t size)
{
	size_t offset = sizeof(uint32_t);

	mmio_write_32(info->base, DDR_PATTERN);

	while (offset < size) {
		mmio_write_32(info->base + offset, DDR_ANTIPATTERN);
		__DSB();

		if (mmio_read_32(info->base) != DDR_PATTERN) {
			break;
		}

		offset <<= 1U;
	}

	return offset;
}
