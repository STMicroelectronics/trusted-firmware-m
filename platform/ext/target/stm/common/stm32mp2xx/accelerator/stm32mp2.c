/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "crypto_hw.h"
#include "entropy.h"
#include "stm32mp2.h"

/*  interface for mbed-crypto */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
	(void)data;

	*olen = 0;

	if (entropy_get_entropy(NULL, output, len))
		return -1;

	*olen = len;

	return 0;
}

/*
 * \brief Initialize the stm crypto accelerator
 */

int crypto_hw_accelerator_init(void)
{
	return 0;
}

/*
 * \brief Deallocate the stm crypto accelerator
 */
int crypto_hw_accelerator_finish(void)
{
	return 0;
}

