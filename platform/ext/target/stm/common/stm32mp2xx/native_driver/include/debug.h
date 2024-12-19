/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  DEBUG_H
#define  DEBUG_H

#include <lib/utils_def.h>
#include <stdio.h>
#include <stdint.h>

#if STM32_NSEC
#include <tfm_log.h>

#define EMSG(_fmt, ...)    LOG_MSG("[ERR] "_fmt, ##__VA_ARGS__)
#define WMSG(_fmt, ...)    LOG_MSG("[WAR] "_fmt, ##__VA_ARGS__)
#define IMSG(_fmt, ...)    LOG_MSG("[INF] "_fmt, ##__VA_ARGS__)
#define DMSG(_fmt, ...)    LOG_MSG("[DBG] "_fmt, ##__VA_ARGS__)

#elif STM32_SEC
#include <tfm_log.h>

/* map on tfm_log library */
#define EMSG(...)       ERROR(__VA_ARGS__)
#define WMSG(...)       WARN(__VA_ARGS__)
#define IMSG(...)       INFO(__VA_ARGS__)
#define DMSG(...)       VERBOSE(__VA_ARGS__)

#elif STM32_BL2
#include <bootutil/bootutil_log.h>

#define EMSG(...)       BOOT_LOG_ERR(__VA_ARGS__)
#define WMSG(...)       BOOT_LOG_WRN(__VA_ARGS__)
#define IMSG(...)       BOOT_LOG_INF(__VA_ARGS__)
#define DMSG(...)       BOOT_LOG_DBG(__VA_ARGS__)

#else
#error "debug not supported in this component"
#endif

#ifndef ERROR
#define ERROR   EMSG
#endif

#ifndef WARN
#define WARN    WMSG
#endif

#ifndef INFO
#define INFO    IMSG
#endif

#ifndef VERBOSE
#define VERBOSE DMSG
#endif

#define panic() while(1)

#define _ASSERT(_test)				\
	do {					\
		if (!(_test))			\
			while(1);		\
	} while(false)
#endif /* DEBUG_H */

