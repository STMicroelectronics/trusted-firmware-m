/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  __CLK_H__
#define  __CLK_H__

#include <stdbool.h>
#include <limits.h>

#define CLK_UNDEF	ULONG_MAX

typedef struct clk_ops {
	int (*enable)(unsigned long id);
	void (*disable)(unsigned long id);
	unsigned long (*get_rate)(unsigned long id);
        int (*get_parent)(unsigned long id);
	bool (*is_enabled)(unsigned long id);
} clk_ops_t;

void clk_enable(unsigned long id);
void clk_disable(unsigned long id);
unsigned long clk_get_rate(unsigned long id);
bool clk_is_enabled(unsigned long id);
int  clk_get_parent(unsigned long id);

void clk_register(const clk_ops_t *ops);

#endif   /* ----- #ifndef __CLK_H__  ----- */
