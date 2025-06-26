/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <psa/service.h>
#include <uapi/tfm_adac_api.h>
#include "adac.h"

static bool is_service_enabled;

static psa_status_t adac_service(const psa_msg_t *msg)
{
	return adac_service_request();
}

/**
 * \brief The ADAC partition's entry function.
 */
psa_status_t tfm_adac_init(void)
{
	return adac_sp_init(&is_service_enabled);
}

psa_status_t tfm_adac_service_sfn(const psa_msg_t *msg)
{
	if (!is_service_enabled)
		return PSA_ERROR_NOT_PERMITTED;

	/* Process the message type */
	switch (msg->type)
	{
	case PSA_IPC_CALL:
		return adac_service(msg);
	default:
		/* Invalid message type */
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
