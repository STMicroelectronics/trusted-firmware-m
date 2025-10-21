/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <Driver_Common.h>
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
#include <stdio.h>

/*  SCMI Protocol Define */
#define _PROT(A)  (((A) >> 10) & 0xff)
#define _TOKEN(A) (((A) >> 18) & 0x3ff)
#define _MSG_ID(A)((A) & 0xff)
#define SYS_POWER 0x12
#define _MSG(PROT, ID)  ((PROT) << 10 | (ID))
#define SYS_POWER 0x12
#define SYS_POWER_STATE_NOTIFIER (0x0)
#define SYS_POWER_STATE_NOTIFY (0x5)
#define ENABLE_NOTIFY (0x1)
#define GRACEFUL (0x1)

int32_t tfm_ns_platform_init (void)
{
	sys_init_run_level(INIT_LEVEL_PRE_CORE);
	sys_init_run_level(INIT_LEVEL_CORE);

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
	uint32_t in_buf[2];
	uint32_t out_buf[2];
	size_t out_sz, in_sz;
	int ret;

	NVIC_SetPriority(Software_IRQ, 1);
	NVIC_EnableIRQ(Software_IRQ);
	err = tfm_ns_notif_init(event_area, sizeof(event_area));

	/* Active scmi system power notification */
	in_buf[0] = _MSG(SYS_POWER, SYS_POWER_STATE_NOTIFY);
	in_buf[1] = ENABLE_NOTIFY;
	in_sz = sizeof(in_buf);
	out_sz = sizeof(out_buf);
	ret = tfm_scmi_req(in_buf, in_sz, out_buf, out_sz);
	if ((ret != PSA_SCMI_ERR_SUCCESS) || (out_buf[1]))
		printf("%s scmi power notitification not activated\r\n", __func__);

	return ARM_DRIVER_OK;
}

int32_t tfm_ns_platform_sec_ctx_call(void)
{
	uint32_t event;
	uint32_t tmp[32];
	psa_status_t ret;

	while (!tfm_ns_notif_get(&event)) {
		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_RSE_NS_EVT) == TFM_SP_IPCC_RSE_NS_EVT)
			psa_call(TFM_MBOX_SERVICE_HANDLE, TFM_MBOX_SERVICE_SID, NULL, 0, NULL, 0);

		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_SCMI_CA35_NS_EVT) ==
		    TFM_SP_IPCC_SCMI_CA35_NS_EVT) {
			tfm_secure_scmi_req(STM32MP25_AGENT_ID_CA35);
		}
		if (tfm_ns_notif_get_pending(TFM_SP_IPCC_SCMI_CA35_BL31_NS_EVT)
		    == TFM_SP_IPCC_SCMI_CA35_BL31_NS_EVT) {
			tfm_secure_scmi_req(STM32MP25_AGENT_ID_CA35_BL31);
		}
		/* handle the scmi system power notification */
		while ((ret = tfm_secure_scmi_get_notif(tmp, sizeof(tmp))) !=
		       PSA_SCMI_ERR_BUFFER_EMPTY){
			if ((_PROT(tmp[0]) == SYS_POWER) &&
			    (_MSG_ID(tmp[0]) == SYS_POWER_STATE_NOTIFIER))
				tfm_sys_power_state_notifier(tmp[1], tmp[2] & GRACEFUL, tmp[3]);
			else
				printf("Unexpected Notif %x %x %x \r\n", _PROT(tmp[0]),
				       _MSG_ID(tmp[0]), _TOKEN(tmp[0]));

			if (ret == PSA_SCMI_ERR_BUFFER_OVERFLOW)
				printf("SCMI Notif overflow\r\n");
		}
	}

	return ARM_DRIVER_OK;
}
#endif
