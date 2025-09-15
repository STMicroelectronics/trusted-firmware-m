/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_PLAT_HW_KEYS_H__
#define __TFM_PLAT_HW_KEYS_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tfm_plat_hw_key_t {
	TFM_PLAT_HUK = 0,
	TFM_PLAT_BHK,
	TFM_PLAT_HW_KEY_LAST
};

#ifdef __cplusplus
}
#endif

#endif /* __TFM_PLAT_HW_KEYS_H__ */