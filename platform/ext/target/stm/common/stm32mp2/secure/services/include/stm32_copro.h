/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  STM32_COPRO_H
#define  STM32_COPRO_H
#include <tfm_platform_system.h>

enum tfm_platform_err_t
stm32_copro_service(const psa_invec *in_vec, const psa_outvec *out_vec);

#endif /* STM32_COPRO_H */
