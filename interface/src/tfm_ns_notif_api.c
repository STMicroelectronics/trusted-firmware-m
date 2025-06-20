/*
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "tfm_ns_notif_api.h"
#include "lib/utils_def.h"
#include "cmsis.h"

int32_t tfm_ns_notif_init_s(void *area, size_t area_size);

struct  ns_event_fifo *p_ns_fifo;
static volatile uint32_t pending_ns_event = 0;

static uint32_t atomic_set(volatile uint32_t *valuePtr, uint32_t val)
{
	COMPILER_BARRIER();
	uint32_t newValue;
	do
	{
		newValue = __LDREXW(valuePtr) | val;
	} while (__STREXW(newValue, valuePtr));
	COMPILER_BARRIER();
	return newValue;
}

static uint32_t atomic_clr(volatile uint32_t *valuePtr, uint32_t val)
{
	COMPILER_BARRIER();
	uint32_t value;
	uint32_t new_val;
	do {
		value = __LDREXW(valuePtr);
		new_val = value & ~val;
	} while (__STREXW(new_val, valuePtr));
	COMPILER_BARRIER();
	return value & val;
}

int32_t tfm_ns_notif_init(void *area, size_t area_size)
{
	int32_t ret = tfm_ns_notif_init_s(area, area_size);

	if (!ret)
		p_ns_fifo = area;

	return ret;
}

__attribute__((weak))
void tfm_ns_notif_listener(uint32_t event)
{
	 atomic_set(&pending_ns_event, event);
}

__attribute__((weak))
uint32_t tfm_ns_notif_get_pending(uint32_t val)
{
	/*  return val from pending_mask  and clear val in an
	 *  atomic way*/
	return atomic_clr(&pending_ns_event, val);
}

int32_t tfm_ns_notif_get(uint32_t *event)
{
	uint32_t cur_read;
	uint32_t next_read;

	if (!p_ns_fifo)
		return -1;

	cur_read = p_ns_fifo->read;
	next_read = (p_ns_fifo->read + 1) >= p_ns_fifo->len ? 0 : p_ns_fifo->read + 1;

	if (p_ns_fifo->write == p_ns_fifo->read)
		/*  fifo empty */
		return -2;

	p_ns_fifo->read = next_read;
	*event = p_ns_fifo->event[cur_read];
	tfm_ns_notif_listener(*event);

	return 0;
}
