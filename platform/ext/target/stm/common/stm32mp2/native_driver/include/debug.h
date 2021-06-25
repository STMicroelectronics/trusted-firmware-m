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

#define STM32_LOG_LEVEL_OFF       0
#define STM32_LOG_LEVEL_ERROR     1
#define STM32_LOG_LEVEL_WARNING   2
#define STM32_LOG_LEVEL_INFO      3
#define STM32_LOG_LEVEL_DEBUG     4

#if STM32_LOG_LEVEL >= STM32_LOG_LEVEL_ERROR
# define EMSG(_fmt, ...) printf("[ERR] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define EMSG(...) (void)0
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_LEVEL_WARNING
# define WMSG(_fmt, ...) printf("[WRN] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define WMSG(...) (void)0
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_LEVEL_INFO
# define IMSG(_fmt, ...) printf("[INF] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define IMSG(...) (void)0
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_LEVEL_DEBUG
# define DMSG(_fmt, ...) printf("[DBG] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define DMSG(...) (void)0
#endif

/* for compatibilities with optee drivers */
#define MSG  (void)0
#define FMSG DMSG

/* for compatibilities with tfa drivers */
#define ERROR	EMSG
#define NOTICE	WMSG
#define WARN	WMSG
#define INFO	IMSG
#define VERBOSE DMSG

#define panic() while(1)

#endif /* DEBUG_H */

