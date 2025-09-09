// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 *
 * Author(s): Maxime Méré, <maxime.mere@foss.st.com> for STMicroelectronics.
 */
#ifndef  STM32_KEYSHARE_H
#define  STM32_KEYSHARE_H

#include <psa/client.h>
#include <tfm_platform_api.h>

enum tfm_platform_err_t stm32_keyshare_enable(const psa_invec *in_vec, const psa_outvec *out_vec);
enum tfm_platform_err_t stm32_keyshare_disable(const psa_invec *in_vec, const psa_outvec *out_vec);

#endif
