/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  __DELAY_H__
#define  __DELAY_H__

#include <cmsis.h>

void udelay(unsigned long usec);

static inline void mdelay(unsigned long msec)
{
	udelay(1000 * msec);
}

#endif   /* ----- #ifndef __DELAY_H__  ----- */
