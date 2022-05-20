/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <uapi/tfm_ioctl_api.h>
#include <plat_device.h>

#include <stm32_copro.h>
#include <stm32_pwr.h>
#include <rstctrl.h>

#ifndef COPRO_RSTCTRL
#error "COPRO_RSTCTRL must be defined for co-processor"
#endif

#ifndef COPRO_ID
#error "COPRO_ID must be defined for co-processor"
#endif

#define COPRO_TIMEOUT_US 1000

static int _copro_power_up(void)
{
	return 0;
}

enum stm32_copro_state_t stm32_copro_get_state(void)
{
	if (stm32_pwr_cpu_get_dstate(COPRO_ID) == DSTANDBY)
		return COPRO_OFF;

	if (stm32_pwr_cpu_get_cstate(COPRO_ID) == CRESET)
		return COPRO_RESET;

	/*
	 * running: the cpu power is up, the copro is running
	 * or could be trig on event
	 */
	return COPRO_RUNNING;
}

enum stm32_copro_state_t
stm32_copro_set_state(enum stm32_copro_state_t state)
{
	if (state != COPRO_RUNNING && state != COPRO_RESET)
		return COPRO_ERR;

	if (stm32_copro_get_state() == COPRO_OFF)
		if (_copro_power_up())
			return COPRO_ERR;

	if (state == COPRO_RESET)
		rstctrl_assert_to(COPRO_RSTCTRL, COPRO_TIMEOUT_US);
	else
		rstctrl_deassert_to(COPRO_RSTCTRL, COPRO_TIMEOUT_US);

	return stm32_copro_get_state();
}

enum tfm_platform_err_t
stm32_copro_service(const psa_invec *in_vec, const psa_outvec *out_vec)
{
	struct tfm_copro_service_args_t *args;
	struct tfm_copro_service_out_t *out;

	if (in_vec->len != sizeof(struct tfm_copro_service_args_t) ||
	    out_vec->len != sizeof(struct tfm_copro_service_out_t))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	args = (struct tfm_copro_service_args_t *)in_vec->base;
	out = (struct tfm_copro_service_out_t *)out_vec->base;
	out->state = COPRO_ERR;

	switch (args->type) {
	case TFM_COPRO_SERVICE_TYPE_GET:
		out->state = stm32_copro_get_state();
		break;

	case TFM_COPRO_SERVICE_TYPE_SET:
		out->state = stm32_copro_set_state(args->state);
		break;

	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
		break;
	}

	return TFM_PLATFORM_ERR_SUCCESS;
}
