/*
 * Copyright (c) 2017-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __TFM_NS_NOTIF_API_H__
#define __TFM_NS_NOTIF_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tfm_veneers.h"

struct  ns_event_fifo {
	uint32_t write;
	uint32_t read;
	uint32_t len;
	uint32_t event[];
};

/**
 * \brief NS interface to initialize notification from secure to non secure
 *
 * \details This function porvide a non secure memory area used as a fifo
 *          to provide secure event to non secure
 *
 * \return Returns 0 in case of sucessfull initialisation
 *	   else return error code
 */
int32_t tfm_ns_notif_init(void *area, size_t area_size);

/**
 * \brief NS interface to get notification sent by secure to non secure
 *
 *
 * \return Returns 0 when an event is present
 *	   else return error code
 */
int32_t tfm_ns_notif_get(uint32_t *event);

/**
 * \brief NS interface to mask/umask notification sent by secure to non secure
 *
 * \return Returns 0 on success
 * 	   else return error code
 */
int32_t tfm_ns_notif_set_mask(uint32_t mask);

/**
 * \brief NS interface function to retrieve events in ns variable
 * maintaining event posted by secure
 * \return Returns  0 on success
 *	   else return error code
 */
uint32_t tfm_ns_notif_get_pending(uint32_t val);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_NS_NOTF_API_H__ */
