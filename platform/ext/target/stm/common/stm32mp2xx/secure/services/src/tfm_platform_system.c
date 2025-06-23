/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <debug.h>
#include <reset.h>
#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_api.h>
#include <tfm_sp_log.h>

#include <cpus.h>
#include <wdt.h>

#if defined(STM32MP21xxxx)
#include <dt-bindings/reset/st,stm32mp21-rcc.h>
#else
#include <dt-bindings/reset/st,stm32mp25-rcc.h>
#endif

/* Resets the system with RCC when TDCID or locally. */
void tfm_platform_hal_system_reset(void)
{
	struct reset_control rstc;

	if (IS_ENABLED(STM32_M33TDCID)) {
		rstc.dev = DEVICE_DT_GET(DT_NODELABEL(rcc_reset));
		rstc.id = SYS_R;

		IMSG("System reset");
		reset_control_assert(&rstc);
	} else {
		IMSG("Cortex-M33 reset");
		NVIC_SystemReset();
	}
}
enum tfm_platform_err_t tfm_platform_hal_ioctl(tfm_platform_ioctl_req_t request,
					       psa_invec  *in_vec,
					       psa_outvec *out_vec)
{
	switch(request) {
#ifdef STM32_M33TDCID
	case TFM_PLATFORM_IOCTL_CPU_SERVICE:
		return cpus_service(in_vec, out_vec);
#endif
#ifdef TFM_PLATFORM_WDT_API
	case TFM_PLATFORM_IOCTL_WDT_SERVICE:
		return watchdog_service(in_vec, out_vec);
#endif
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}

	return TFM_PLATFORM_ERR_NOT_SUPPORTED;
}
