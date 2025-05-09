/*
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "tfm_ns_notif_api.h"

int32_t tfm_ns_notif_init_s(void *area, size_t area_size);

struct  ns_event_fifo *p_ns_fifo;

int32_t tfm_ns_notif_init(void *area, size_t area_size)
{
	int32_t ret = tfm_ns_notif_init_s(area, area_size);

	if (!ret)
		p_ns_fifo = area;

	return ret;
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

	return 0;
}
