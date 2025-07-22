/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 */

#ifndef _INCLUDE_ILOG2_H_
#define _INCLUDE_ILOG2_H_

/**
 * const_ilog2 - log base 2 of 32-bit constant unsigned value
 * @n: parameter
 *
 * Use this where sparse expects a true constant expression, e.g. for array
 * indices.
 */
#define const_ilog2(n)				\
(						\
	__builtin_constant_p(n) ? (		\
		(n) < 2 ? 0 :			\
		(n) & (1UL << 31) ? 31 :	\
		(n) & (1UL << 30) ? 30 :	\
		(n) & (1UL << 29) ? 29 :	\
		(n) & (1UL << 28) ? 28 :	\
		(n) & (1UL << 27) ? 27 :	\
		(n) & (1UL << 26) ? 26 :	\
		(n) & (1UL << 25) ? 25 :	\
		(n) & (1UL << 24) ? 24 :	\
		(n) & (1UL << 23) ? 23 :	\
		(n) & (1UL << 22) ? 22 :	\
		(n) & (1UL << 21) ? 21 :	\
		(n) & (1UL << 20) ? 20 :	\
		(n) & (1UL << 19) ? 19 :	\
		(n) & (1UL << 18) ? 18 :	\
		(n) & (1UL << 17) ? 17 :	\
		(n) & (1UL << 16) ? 16 :	\
		(n) & (1UL << 15) ? 15 :	\
		(n) & (1UL << 14) ? 14 :	\
		(n) & (1UL << 13) ? 13 :	\
		(n) & (1UL << 12) ? 12 :	\
		(n) & (1UL << 11) ? 11 :	\
		(n) & (1UL << 10) ? 10 :	\
		(n) & (1UL <<  9) ?  9 :	\
		(n) & (1UL <<  8) ?  8 :	\
		(n) & (1UL <<  7) ?  7 :	\
		(n) & (1UL <<  6) ?  6 :	\
		(n) & (1UL <<  5) ?  5 :	\
		(n) & (1UL <<  4) ?  4 :	\
		(n) & (1UL <<  3) ?  3 :	\
		(n) & (1UL <<  2) ?  2 :	\
		1) :				\
	-1)

#define ilog2(n)			\
(					\
	__builtin_constant_p(n) ?	\
	const_ilog2(n) :		\
	((n) < 2 ? 0 :			\
	 (31 - __builtin_clz(n)))	\
)

#endif /* _INCLUDE_ILOG2_H_ */
