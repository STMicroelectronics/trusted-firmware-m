/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  __MMIOPOLL_H__
#define  __MMIOPOLL_H__

#include <errno.h>
#include <lib/mmio.h>
#include <lib/timeout.h>

/**
 * readx_poll_timeout - Periodically poll an address until a condition is met or a timeout occurs
 * @op: accessor function (takes @addr as its only argument)
 * @addr: Address to poll
 * @val: Variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @timeout_us: Timeout in us, 0 means never timeout
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @addr is stored in @val. Must not
 * be called from atomic context if sleep_us or timeout_us are used.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 */
#define mmio_readx_poll_timeout(op, addr, val, cond, timeout_us)	\
({ \
	uint64_t __timeout_us = (timeout_us); \
	uint64_t __timeout = timeout_init_us(__timeout_us); \
	for (;;) { \
		(val) = op(addr); \
		if (cond) \
			break; \
		if (__timeout_us && timeout_elapsed(__timeout)) { \
			(val) = op(addr); \
			break; \
		} \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define mmio_read8_poll_timeout(addr, val, cond, timeout_us) \
	mmio_readx_poll_timeout(mmio_read_8, addr, val, cond, timeout_us)

#define mmio_read16_poll_timeout(addr, val, cond, timeout_us) \
	mmio_readx_poll_timeout(mmio_read_16, addr, val, cond, timeout_us)

#define mmio_read32_poll_timeout(addr, val, cond, timeout_us) \
	mmio_readx_poll_timeout(mmio_read_32, addr, val, cond, timeout_us)

#define mmio_read64_poll_timeout(addr, val, cond, timeout_us) \
	mmio_readx_poll_timeout(mmio_read_64, addr, val, cond, timeout_us)
#endif   /* ----- #ifndef MMIOPOLL_H  ----- */
