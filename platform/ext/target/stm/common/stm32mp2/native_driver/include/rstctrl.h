/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  __RSTCTRL_H
#define  __RSTCTRL_H

struct rstctrl;

struct rstctrl_ops {
	/*
	 * Operators on reset control(s) exposed by a reset controller
	 *
	 * @assert_level: Assert reset level on control with a timeout hint
	 * @deassert_level: Deassert reset level on control with a timeout hint
	 *
	 * Operator functions @assert_level and @deassert_level use arguments:
	 * @rstctrl: Reset controller
	 * @to_us: Timeout in microseconds or RSTCTRL_NO_TIMEOUT, may be ignored
	 * by reset controller.
	 */
	int (*assert_level)(struct rstctrl *rstctrl, unsigned int to_us);
	int (*deassert_level)(struct rstctrl *rstctrl, unsigned int to_us);
};

/*
 * struct rstctrl - Instance of a control exposed by a reset controller
 * @ops: Operators of the reset controller
 */
struct rstctrl {
	const struct rstctrl_ops *ops;
};

/*
 * Platform driver may ignore the timeout hint according to their
 * capabilities. RSTCTRL_NO_TIMEOUT specifies no timeout hint.
 */
#define RSTCTRL_NO_TIMEOUT	0

/*
 * rstctrl_assert_to - Assert reset control possibly with timeout
 * rstctrl_assert - Assert reset control
 * rstctrl_deassert_to - Deassert reset control possibly with timeout
 * rstctrl_deassert - Deassert reset control
 *
 * @rstctrl: Reset controller
 * @to_us: Timeout in microseconds
 * Return an int: 0: no error else error code
 */
static inline int rstctrl_assert_to(struct rstctrl *rstctrl,
					   unsigned int to_us)
{
	return rstctrl->ops->assert_level(rstctrl, to_us);
}

static inline int rstctrl_assert(struct rstctrl *rstctrl)
{
	return rstctrl_assert_to(rstctrl, RSTCTRL_NO_TIMEOUT);
}

static inline int rstctrl_deassert_to(struct rstctrl *rstctrl,
					     unsigned int to_us)
{
	return rstctrl->ops->deassert_level(rstctrl, to_us);
}

static inline int rstctrl_deassert(struct rstctrl *rstctrl)
{
	return rstctrl_deassert_to(rstctrl, RSTCTRL_NO_TIMEOUT);
}

#endif   /* ----- #ifndef __RSTCTRL_H  ----- */
