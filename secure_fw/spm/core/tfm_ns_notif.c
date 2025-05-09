/*
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "tfm_arch.h"
#include "tfm_nspm.h"
#include "ffm/psa_api.h"
#include "tfm_ns_notif.h"
#include "tfm_ns_notif_api.h"
#include "tfm_hal_defs.h"
#include "tfm_hal_platform.h"
#include "tfm_hal_isolation.h"
#include "mmio_defs.h"
#include "psa/error.h"
#include "spm.h"
#include "ns_evt.h"
#include "critical_section.h"
#include "current.h"

static uint32_t ns_evt_mask = 0;
uint32_t ns_evt_owned = 0;
struct  ns_event_fifo *p_ns_fifo;

__tz_c_veneer
int32_t tfm_ns_notif_init_s(void *area, size_t area_size)
{
	struct critical_section_t cs_signal = CRITICAL_SECTION_STATIC_INIT;
	uintptr_t boundary = (1 << HANDLE_ATTR_NS_POS) &
		HANDLE_ATTR_NS_MASK;
	uint32_t attr = TFM_HAL_ACCESS_READWRITE;
	bool ns_caller = tfm_spm_is_ns_caller();
	uint32_t ret;

	if (!ns_caller)
		return TFM_HAL_ERROR_INVALID_INPUT;

	ret = tfm_hal_memory_check(boundary, (uintptr_t)area,
				  area_size, attr);

	if (ret != TFM_HAL_SUCCESS)
		return ret;

	if (p_ns_fifo)
		return  TFM_HAL_ERROR_BAD_STATE;

	CRITICAL_SECTION_ENTER(cs_signal);
	/*  initialized  */
	p_ns_fifo = area;
	p_ns_fifo->len = (area_size - sizeof(*p_ns_fifo)) / sizeof(uint32_t);
	p_ns_fifo->write = 0;
	p_ns_fifo->read = 0;

	if (p_ns_fifo->len <= 1) {
		p_ns_fifo = NULL;
		CRITICAL_SECTION_LEAVE(cs_signal);
		return TFM_HAL_ERROR_INVALID_INPUT;
	}

	if (tfm_hal_notify_ns_init()) {
		p_ns_fifo = NULL;
		CRITICAL_SECTION_LEAVE(cs_signal);
		return TFM_HAL_ERROR_GENERIC;
	}
	CRITICAL_SECTION_LEAVE(cs_signal);
	return 0;
}

__tz_c_veneer
int32_t tfm_ns_notif_set_mask(uint32_t mask)
{
	ns_evt_mask = mask;
	return 0;
}

/* 1 element cannot be written   */
/*  fifo works as follow : rx = 0, tx = 0  noevent
                           rx = 0 , tx = 1  =>1 event to read
                           rx = size-1 tx = 0 => 1 event to read
			   rx = 0 , tx = size - 1, size event to read, fifo is full
			   rx = 1 , tx = size -1 , [1 to size -2] to read
			   rx = 1 , tx = 0 , fifo is full

*/
static psa_status_t ns_notif_do(uint32_t event, uint32_t owned_evt)
{
	struct critical_section_t cs_signal = CRITICAL_SECTION_STATIC_INIT;
	uint32_t next_write;

	if (!p_ns_fifo)
		return PSA_ERROR_NOT_SUPPORTED;

	if ((event & ~ns_evt_mask) != event)
		return PSA_ERROR_BAD_STATE;

	if ((event & owned_evt) != event)
		return PSA_ERROR_NOT_PERMITTED;

	CRITICAL_SECTION_ENTER(cs_signal);
	/*  compute next write value if possible */
	next_write = (p_ns_fifo->write + 1) >= p_ns_fifo->len ? 0 : p_ns_fifo->write + 1;

	if (next_write == p_ns_fifo->read) {
		CRITICAL_SECTION_LEAVE(cs_signal);
		/*  fifo full */
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	p_ns_fifo->event[p_ns_fifo->write] = event;
	p_ns_fifo->write = next_write;
	tfm_hal_raise_notify_ns();
	CRITICAL_SECTION_LEAVE(cs_signal);
	return PSA_SUCCESS;
}

psa_status_t ns_notif(uint32_t event)
{
	return ns_notif_do(event, ns_evt_owned);
}
