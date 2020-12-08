/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#ifdef TFM_ENV
#include <clk.h>
#else
#include <drivers/clk.h>
#endif

/*
 * The clk implementation
 */

static const clk_ops_t *ops;

void clk_enable(unsigned long id)
{
	assert(ops);
	assert(ops->enable != NULL);

	ops->enable(id);
}

void clk_disable(unsigned long id)
{
	assert(ops);
	assert(ops->disable != NULL);

	ops->disable(id);
}

unsigned long clk_get_rate(unsigned long id)
{
	assert(ops);
	assert(ops->get_rate != NULL);

	return ops->get_rate(id);
}

int  clk_get_parent(unsigned long id)
{
	assert(ops);
	assert(ops->get_parent != NULL);

	return ops->get_parent(id);
}

bool clk_is_enabled(unsigned long id)
{
	assert(ops);
	assert(ops->is_enabled != NULL);

	return ops->is_enabled(id);
}

/*
 * Initialize the clk. The fields in the provided clk
 * ops pointer must be valid.
 */

void clk_register(const clk_ops_t *ops_ptr)
{
	assert(ops_ptr != NULL  &&
			(ops_ptr->enable != NULL) &&
			(ops_ptr->disable != NULL) &&
			(ops_ptr->get_rate != NULL) &&
			(ops_ptr->get_parent != NULL) &&
			(ops_ptr->is_enabled != NULL));

	ops = ops_ptr;
}
