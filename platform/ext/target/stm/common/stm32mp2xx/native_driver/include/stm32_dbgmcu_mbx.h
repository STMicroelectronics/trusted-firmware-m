/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * Copyright (c) 2025, STMicroelectronics
 */
#ifndef __STM32_DBGMCU_MBX_H
#define __STM32_DBGMCU_MBX_H

#include <stddef.h>
#include <stdint.h>
#include <lib/timeout.h>

/* Timeout for mailbox read and write */
#define MAILBOX_TIMEOUT_1S_IN_MS MSEC_PER_SEC

/*
 * Read a value from DBGMCU_DBG_AUTH_HOST register.
 * @value: pointer to value read from register
 * @timeout_ms: timeout in milliseconds. If timeout is null, DBG_AUTH_HOST is
 *              read without waiting for DBG_AUTH_ACK.HOST_ACK. Useful in case
 *              of adac request detection where DBG_AUTH_HOST is first read by
 *              early request or interrupt management.
 */
int stm32_dbgmcu_mbx_read_auth_host(uint32_t *value, uint32_t timeout_ms);

/*
 * Write a value in DBGMCU_DBG_AUTH_DEV register.
 * @value: value to write in the register
 * @timeout_ms: timeout in milliseconds
 */
int stm32_dbgmcu_mbx_write_auth_dev(uint32_t value, uint32_t timeout_ms);

#endif /* __STM32_DBGMCU_MBX_H */
