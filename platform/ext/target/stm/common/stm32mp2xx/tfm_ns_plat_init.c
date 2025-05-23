/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <Driver_Common.h>
#include <plat_device.h>
#include <uart_stdout.h>
#include <init.h>
#include "cmsis.h"
#include "psa/client.h"
#include "tfm_plat_ns.h"
#include "tfm_ns_notif_api.h"
#include "tfm_scmi_api.h"
#include "psa_manifest/sid.h"
#include "ns_evt.h"
#include <dt-bindings/scmi/stm32mp2-agents.h>

int32_t tfm_ns_platform_init (void)
{
	sys_init_run_level(INIT_LEVEL_PRE_CORE);
	sys_init_run_level(INIT_LEVEL_CORE);

	if (stm32_platform_ns_init())
		return ARM_DRIVER_ERROR;

	stdio_init();

	sys_init_run_level(INIT_LEVEL_POST_CORE);

	return ARM_DRIVER_OK;
}
#if STM32_M33TDCID
#define Software_IRQ RESERVED_9
#define Software_IRQ_Handler RESERVED_9_IRQHandler
extern void tfm_ns_post_sem_sec_ctx(void);

extern  void tfm_ns_post_sem_sec_ctx(void);
void Software_IRQ_Handler(void)
{
	NVIC_ClearPendingIRQ(Software_IRQ);
	tfm_ns_post_sem_sec_ctx();
}

static uint32_t event_area[16+sizeof(struct ns_event_fifo)];

int32_t tfm_ns_platform_post_init(void)
{
	int err;

	NVIC_SetPriority(Software_IRQ, 1);
	NVIC_EnableIRQ(Software_IRQ);
	err = tfm_ns_notif_init(event_area, sizeof(event_area));

	return ARM_DRIVER_OK;
}

int32_t tfm_ns_platform_sec_ctx_call(void)
{
	uint32_t event;

	while(!tfm_ns_notif_get(&event)) {
		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_RSE_NS_EVT) == TFM_SP_IPCC_RSE_NS_EVT)
			psa_call(TFM_MBOX_SERVICE_HANDLE, TFM_MBOX_SERVICE_SID, NULL, 0, NULL, 0);

		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_SCMI_CA35_NS_EVT) == TFM_SP_IPCC_SCMI_CA35_NS_EVT)
			tfm_secure_scmi_req(STM32MP25_AGENT_ID_CA35);

		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_SCMI_CA35_BL31_NS_EVT)
		    == TFM_SP_IPCC_SCMI_CA35_BL31_NS_EVT)
			tfm_secure_scmi_req(STM32MP25_AGENT_ID_CA35_BL31);
	}
	return ARM_DRIVER_OK;
}
#endif
