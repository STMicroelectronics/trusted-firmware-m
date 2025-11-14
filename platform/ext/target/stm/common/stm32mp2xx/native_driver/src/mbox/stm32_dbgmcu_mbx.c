// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 * Author(s): Amelie Delaunay, <amelie.delaunay@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32mp21_dbgmcu_mbx

#include <device.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "single-instance st_stm32mp21_dbgmcu_mbx node must be present");

#include <string.h>

#include <clk.h>
#include <cmsis.h>
#include <debug.h>
#include <lib/mmiopoll.h>
#include <stm32_dbgmcu_mbx.h>
#include <target_cfg.h>

#include <psa/error.h>
#include <uapi/tfm_adac_api.h>

/* DBGMCU mailbox offset register */
#define DBGMCU_DBG_AUTH_HOST U(0x0)
#define DBGMCU_DBG_AUTH_DEV U(0x4)
#define DBGMCU_DBG_AUTH_ACK U(0x8)

/* DBGMCU_DBG_AUTH_ACK register*/
#define DBGMCU_DBG_AUTH_ACK_HOST BIT(0)
#define DBGMCU_DBG_AUTH_ACK_DEV BIT(1)

#define AUTH_DEV_DUMMY_VAL 0xbadf00d0

struct stm32_dbgmcu_mbx_config
{
	uintptr_t base;
	const struct device *clk_dbgmcu;
	const clk_subsys_t clk_dbgmcu_subsys;
	const uint32_t dbg_auth_wr_irq;
	const uint32_t dbg_auth_rd_irq;
};

static const struct device *dbgmcu_dev;

int stm32_dbgmcu_mbx_read_auth_host(uint32_t *value, uint32_t timeout_ms)
{
	const struct stm32_dbgmcu_mbx_config *drv_cfg = dev_get_config(dbgmcu_dev);
	uintptr_t reg_auth_ack = drv_cfg->base + DBGMCU_DBG_AUTH_ACK;
	uint32_t auth_ack = 0;

	if (!value)
		return -EINVAL;

	if (!timeout_ms)
		goto skip_wait;

	/* Wait until the debugger has written DBG_AUTH_HOST */
	if (mmio_read32_poll_timeout(reg_auth_ack, auth_ack, auth_ack & DBGMCU_DBG_AUTH_ACK_HOST,
				     timeout_ms * USEC_PER_MSEC))
	{
		EMSG("DBGMCU %s timeout\n", __func__);
		return -ETIMEDOUT;
	}

skip_wait:
	*value = mmio_read_32(drv_cfg->base + DBGMCU_DBG_AUTH_HOST);
	DMSG("DBGMCU DBG_AUTH_HOST = 0x%08x\n", *value);

	return 0;
}

int stm32_dbgmcu_mbx_write_auth_dev(uint32_t value, uint32_t timeout_ms)
{
	const struct stm32_dbgmcu_mbx_config *drv_cfg = dev_get_config(dbgmcu_dev);
	uintptr_t reg_auth_ack = drv_cfg->base + DBGMCU_DBG_AUTH_ACK;
	uint32_t auth_ack = 0;

	/* Wait until debugger has read DBG_AUTH_DEV */
	if (mmio_read32_poll_timeout(reg_auth_ack, auth_ack, !(auth_ack & DBGMCU_DBG_AUTH_ACK_DEV),
				     timeout_ms * USEC_PER_MSEC))
	{
		EMSG("DBGMCU %s timeout\n", __func__);
		return -ETIMEDOUT;
	}

	mmio_write_32(drv_cfg->base + DBGMCU_DBG_AUTH_DEV, value);
	DMSG("DBGMCU DBG_AUTH_DEV = 0x%08x\n", mmio_read_32(drv_cfg->base + DBGMCU_DBG_AUTH_DEV));

	return 0;
}

static void stm32_dbgmcu_mbx_start_tfm_adac_service(void)
{
	uint32_t debug_request;
	psa_status_t status;
	int ret;

	ret = stm32_dbgmcu_mbx_read_auth_host(&debug_request, MAILBOX_TIMEOUT_1S_IN_MS);
	if (ret || !debug_request)
		return;

	status = tfm_adac_service();
	if (status)
		EMSG("DBGMCU Secure debug required, request dropped (0x%x)\n", debug_request);
}

static __unused void stm32_dbgmcu_mbx_threaded_itr(const struct device *dev)
{
	const struct stm32_dbgmcu_mbx_config *drv_cfg = dev_get_config(dev);
	uintptr_t reg_auth_ack = drv_cfg->base + DBGMCU_DBG_AUTH_ACK;
	uint32_t auth_ack = 0;

	/* Wait until the debugger has written DBG_AUTH_HOST */
	if (mmio_read32_poll_timeout(reg_auth_ack, auth_ack, auth_ack & DBGMCU_DBG_AUTH_ACK_HOST,
				     MAILBOX_TIMEOUT_1S_IN_MS * USEC_PER_MSEC))
	{
		EMSG("DBGMCU Timeout checking DBGMCU_DBG_AUTH_ACK_HOST\n");
		return;
	}

	stm32_dbgmcu_mbx_start_tfm_adac_service();
}

int stm32_dbgmcu_mbx_init(const struct device *dev)
{
	const struct stm32_dbgmcu_mbx_config *drv_cfg = dev_get_config(dev);
	struct clk *clk_dbgmcu;
	int ret;

	clk_dbgmcu = clk_get(drv_cfg->clk_dbgmcu, drv_cfg->clk_dbgmcu_subsys);
	if (!clk_dbgmcu)
		return -ENODEV;

	/*
	 * Debug authentication protocol implements a bidirectional communication between
	 * the debugger (host) and the device (STM32) through the mailbox interface located in
	 * DBGMCU. DBGMCU is located in ck_dap clock domain. Clock enable for DBGMCU (DBGMCUEN)
	 * controls ck_icn_p_dbgmcu and ck_dap generation. That's why DBGMCU clock requires to
	 * be enabled as soon as possible, and kept enabled, to support secure debug authentication.
	 */
	ret = clk_enable(clk_dbgmcu);
	if (ret)
		return ret;

	/* Write DBG_AUTH_DEV inconditionnally as it could have already be written */
	mmio_write_32(drv_cfg->base + DBGMCU_DBG_AUTH_DEV, AUTH_DEV_DUMMY_VAL);

	DMSG("DBGMCU Authenticated Debug Mailbox online\n");

	return 0;
}

static const struct stm32_dbgmcu_mbx_config stm32_dbgmcu_mbx_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.clk_dbgmcu = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_dbgmcu_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(0, bits),
	/* Not used - managed by secure partition */
	.dbg_auth_wr_irq = DT_INST_IRQ_BY_NAME(0, dbg_auth_wr, irq),
	.dbg_auth_rd_irq = DT_INST_IRQ_BY_NAME(0, dbg_auth_rd, irq),
};

DEVICE_DT_INST_DEFINE(0, &stm32_dbgmcu_mbx_init, NULL,
		      NULL, &stm32_dbgmcu_mbx_cfg,
		      CORE, 10, NULL);

static const struct device *dbgmcu_dev = DEVICE_DT_INST_GET(0);

void DBG_AUTH_HOST_HANDLE(void) {
	stm32_dbgmcu_mbx_threaded_itr(dbgmcu_dev);
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
