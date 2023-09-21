/*
 * Copyright (c) 2016-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UTILS_DEF_H
#define UTILS_DEF_H

#include <lib/utils_def_exp.h>
#include <stddef.h>

/* Compute the number of elements in the given array */
#define ARRAY_SIZE(a)				\
	(sizeof(a) / sizeof((a)[0]))

#define IS_POWER_OF_TWO(x) (((x) != 0U) && (((x) & ((x) - 1U)) == 0U))

#define SIZE_FROM_LOG2_WORDS(n)		(4 << (n))

#define BIT_32(nr)			(U(1) << (nr))
#define BIT_64(nr)			(ULL(1) << (nr))

#ifdef __aarch64__
#define BIT				BIT_64
#else
#define BIT				BIT_32
#endif

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_64(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#if defined(__LINKER__) || defined(__ASSEMBLER__)
#define GENMASK_32(h, l) \
	(((0xFFFFFFFF) << (l)) & (0xFFFFFFFF >> (32 - 1 - (h))))

#define GENMASK_64(h, l) \
	((~0 << (l)) & (~0 >> (64 - 1 - (h))))
#else
#define GENMASK_32(h, l) \
	(((~UINT32_C(0)) << (l)) & (~UINT32_C(0) >> (32 - 1 - (h))))

#define GENMASK_64(h, l) \
	(((~UINT64_C(0)) << (l)) & (~UINT64_C(0) >> (64 - 1 - (h))))
#endif

#ifdef __aarch64__
#define GENMASK				GENMASK_64
#else
#define GENMASK				GENMASK_32
#endif

/*
 * This variant of div_round_up can be used in macro definition but should not
 * be used in C code as the `div` parameter is evaluated twice.
 */
#define DIV_ROUND_UP_2EVAL(n, d)	(((n) + (d) - 1) / (d))

#define div_round_up(val, div) __extension__ ({	\
	__typeof__(div) _div = (div);		\
	((val) + _div - (__typeof__(div)) 1) / _div;		\
})

#define MIN(x, y) __extension__ ({	\
	__typeof__(x) _x = (x);		\
	__typeof__(y) _y = (y);		\
	(void)(&_x == &_y);		\
	_x < _y ? _x : _y;		\
})

#define MAX(x, y) __extension__ ({	\
	__typeof__(x) _x = (x);		\
	__typeof__(y) _y = (y);		\
	(void)(&_x == &_y);		\
	_x > _y ? _x : _y;		\
})

/*
 * The round_up() macro rounds up a value to the given boundary in a
 * type-agnostic yet type-safe manner. The boundary must be a power of two.
 * In other words, it computes the smallest multiple of boundary which is
 * greater than or equal to value.
 *
 * round_down() is similar but rounds the value down instead.
 */
#define round_boundary(value, boundary)		\
	((__typeof__(value))((boundary) - 1))

#define round_up(value, boundary)		\
	((((value) - 1) | round_boundary(value, boundary)) + 1)

#define round_down(value, boundary)		\
	((value) & ~round_boundary(value, boundary))

/*
 * Evaluates to 1 if (ptr + inc) overflows, 0 otherwise.
 * Both arguments must be unsigned pointer values (i.e. uintptr_t).
 */
#define check_uptr_overflow(_ptr, _inc)		\
	((_ptr) > (UINTPTR_MAX - (_inc)))

/*
 * Evaluates to 1 if (u32 + inc) overflows, 0 otherwise.
 * Both arguments must be 32-bit unsigned integers (i.e. effectively uint32_t).
 */
#define check_u32_overflow(_u32, _inc) \
	((_u32) > (UINT32_MAX - (_inc)))

/* Register size of the current architecture. */
#ifdef __aarch64__
#define REGSZ		U(8)
#else
#define REGSZ		U(4)
#endif

/*
 * Test for the current architecture version to be at least the version
 * expected.
 */
#define ARM_ARCH_AT_LEAST(_maj, _min) \
	((ARM_ARCH_MAJOR > (_maj)) || \
	 ((ARM_ARCH_MAJOR == (_maj)) && (ARM_ARCH_MINOR >= (_min))))

/*
 * Import an assembly or linker symbol as a C expression with the specified
 * type
 */
#define IMPORT_SYM(type, sym, name) \
	extern char sym[];\
	static const __attribute__((unused)) type name = (type) sym;

/*
 * When the symbol is used to hold a pointer, its alignment can be asserted
 * with this macro. For example, if there is a linker symbol that is going to
 * be used as a 64-bit pointer, the value of the linker symbol must also be
 * aligned to 64 bit. This macro makes sure this is the case.
 */
#define ASSERT_SYM_PTR_ALIGN(sym) assert(((size_t)(sym) % __alignof__(*(sym))) == 0)

#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

/* Compiler builtin of GCC >= 9 and planned in llvm */
#ifdef __HAVE_SPECULATION_SAFE_VALUE
# define SPECULATION_SAFE_VALUE(var) __builtin_speculation_safe_value(var)
#else
# define SPECULATION_SAFE_VALUE(var) var
#endif

#ifndef __maybe_unused
#define __maybe_unused                  __attribute__((__unused__))
#endif

#define __unused			__attribute__((__unused__))
#define __weak				__attribute__((weak))

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)

/* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })

#define _FLD_PREP(field, value) (((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _FLD_GET(field, value)  (((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))

#endif /* UTILS_DEF_H */
