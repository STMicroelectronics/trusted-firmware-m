/*
 * Copyright (C) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef  _INCLUDE_STM32MP_DDR_DEBUG
#define  _INCLUDE_STM32MP_DDR_DEBUG

#include <stdio.h>
#include <stdint.h>
#include <debug.h>

#define DDR_LOG_LEVEL_OFF       0
#define DDR_LOG_LEVEL_ERROR     1
#define DDR_LOG_LEVEL_WARNING   2
#define DDR_LOG_LEVEL_INFO      3
#define DDR_LOG_LEVEL_DEBUG     4

#ifndef DDR_LOG_LEVEL
#define DDR_LOG_LEVEL DDR_LOG_LEVEL_ERROR
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_ERROR
# define DDR_ERROR(_fmt, ...) printf("[DDR ERR] " _fmt, ##__VA_ARGS__)
#else
# define DDR_ERROR(...) (void)0
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_WARNING
# define DDR_WARN(_fmt, ...) printf("[DDR WRN] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define DDR_WARN(...) (void)0
#endif

#if DDR_LOG_LEVEL >= STM32_LOG_LEVEL_INFO
# define DDR_INFO(_fmt, ...) printf("[DDR INF] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define DDR_INFO(...) (void)0
#endif

#if DDR_LOG_LEVEL >= STM32_LOG_LEVEL_DEBUG
# define DDR_VERBOSE(_fmt, ...) printf("[DDR DBG] " _fmt "\r\n", ##__VA_ARGS__)
#else
# define DDR_VERBOSE(...) (void)0
#endif

#endif   /* ----- #ifndef _INCLUDE_STM32MP_DDR_DEBUG  ----- */

