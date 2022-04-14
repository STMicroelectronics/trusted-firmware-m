/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_api.h>
#include <tfm_sp_log.h>

#include <stm32_copro.h>

void tfm_platform_hal_system_reset(void)
{
	/* Reset the system */
	NVIC_SystemReset();
}

enum tfm_platform_err_t tfm_platform_hal_ioctl(tfm_platform_ioctl_req_t request,
					       psa_invec  *in_vec,
					       psa_outvec *out_vec)
{
	switch(request) {
#ifdef STM32_M33TDCID
	case TFM_PLATFORM_IOCTL_COPRO_SERVICE:
		return stm32_copro_service(in_vec, out_vec);
#endif
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}
}
