/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_omi

#include <stdint.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <lib/timeout.h>

#include <target_cfg.h>
#include <device.h>

#include <debug.h>
#include <spi_mem.h>
#include <stm32_ospi.h>
#include <clk.h>
#include <pinctrl.h>
#include <reset.h>

/* Timeout for device interface reset */
#define _TIMEOUT_US_1_MS	1000U

/* OCTOSPI registers */
#define _OSPI_CR		0x00U
#define _OSPI_DCR1		0x08U
#define _OSPI_DCR2		0x0CU
#define _OSPI_SR		0x20U
#define _OSPI_FCR		0x24U
#define _OSPI_DLR		0x40U
#define _OSPI_AR		0x48U
#define _OSPI_DR		0x50U
#define _OSPI_CCR		0x100U
#define _OSPI_TCR		0x108U
#define _OSPI_IR		0x110U
#define _OSPI_ABR		0x120U

/* OCTOSPI control register */
#define _OSPI_CR_EN		BIT(0)
#define _OSPI_CR_ABORT		BIT(1)
#define _OSPI_CR_CSSEL		BIT(24)
#define _OSPI_CR_FMODE		GENMASK_32(29, 28)
#define _OSPI_CR_FMODE_SHIFT	28U
#define _OSPI_CR_FMODE_INDW	0U
#define _OSPI_CR_FMODE_INDR	1U
#define _OSPI_CR_FMODE_MM	3U

/* OCTOSPI device configuration register 1 */
#define _OSPI_DCR1_CKMODE	BIT(0)
#define _OSPI_DCR1_DLYBYP	BIT(3)
#define _OSPI_DCR1_CSHT		GENMASK_32(13, 8)
#define _OSPI_DCR1_CSHT_SHIFT	8U
#define _OSPI_DCR1_DEVSIZE	GENMASK_32(20, 16)

/* OCTOSPI device configuration register 2 */
#define _OSPI_DCR2_PRESCALER	GENMASK_32(7, 0)

/* OCTOSPI status register */
#define _OSPI_SR_TEF		BIT(0)
#define _OSPI_SR_TCF		BIT(1)
#define _OSPI_SR_FTF		BIT(2)
#define _OSPI_SR_SMF		BIT(3)
#define _OSPI_SR_BUSY		BIT(5)

/* OCTOSPI flag clear register */
#define _OSPI_FCR_CTEF		BIT(0)
#define _OSPI_FCR_CTCF		BIT(1)
#define _OSPI_FCR_CSMF		BIT(3)

/* OCTOSPI communication configuration register */
#define _OSPI_CCR_ADMODE_SHIFT	8U
#define _OSPI_CCR_ADSIZE_SHIFT	12U
#define _OSPI_CCR_ABMODE_SHIFT	16U
#define _OSPI_CCR_ABSIZE_SHIFT	20U
#define _OSPI_CCR_DMODE_SHIFT	24U

/* OCTOSPI timing configuration register */
#define _OSPI_TCR_DCYC		GENMASK_32(4, 0)
#define _OSPI_TCR_SSHIFT	BIT(30)

#define _OSPI_MAX_CHIP		2U

#define _OSPI_FIFO_TIMEOUT_US	30U
#define _OSPI_CMD_TIMEOUT_US	1000U
#define _OSPI_BUSY_TIMEOUT_US	100U
#define _OSPI_ABT_TIMEOUT_US	100U

#define _FREQ_100MHZ		100000000U

struct stm32_omi_config {
	uintptr_t base;
	uintptr_t mm_base;
	size_t mm_size;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct pinctrl_dev_config *pcfg;
	const struct reset_control *rst_ctl;
	const int n_rst;
};

static struct stm32_ospi_platdata stm32_ospi;

__attribute__((weak))
int stm32_ospi_get_platdata(struct stm32_ospi_platdata *pdata)
{
	return -ENODEV;
}

static uintptr_t ospi_base(void)
{
	return stm32_ospi.drv_cfg->base;
}

static int stm32_ospi_wait_for_not_busy(void)
{
	uint32_t sr;
	int ret;

	ret = mmio_read32_poll_timeout(ospi_base() + _OSPI_SR, sr,
				       (sr & _OSPI_SR_BUSY) == 0U,
				       _OSPI_BUSY_TIMEOUT_US);
	if (ret != 0) {
		ERROR("%s: busy timeout\n", __func__);
	}

	return ret;
}

static int stm32_ospi_wait_cmd(const struct spi_mem_op *op)
{
	uint32_t sr;
	int ret = 0;


	ret = mmio_read32_poll_timeout(ospi_base() + _OSPI_SR, sr,
				       (sr & _OSPI_SR_TCF) != 0U,
				       _OSPI_CMD_TIMEOUT_US);
	if (ret != 0) {
		ERROR("%s: cmd timeout\n", __func__);
	} else if ((mmio_read_32(ospi_base() + _OSPI_SR) & _OSPI_SR_TEF) != 0U) {
		ERROR("%s: transfer error\n", __func__);
		ret = -EIO;
	}

	/* Clear flags */
	mmio_write_32(ospi_base() + _OSPI_FCR, _OSPI_FCR_CTCF | _OSPI_FCR_CTEF);

	if (ret == 0) {
		ret = stm32_ospi_wait_for_not_busy();
	}

	return ret;
}

static void stm32_ospi_read_fifo(uint8_t *val, uintptr_t addr)
{
	*val = mmio_read_8(addr);
}

static void stm32_ospi_write_fifo(uint8_t *val, uintptr_t addr)
{
	mmio_write_8(addr, *val);
}

static int stm32_ospi_poll(const struct spi_mem_op *op)
{
	void (*fifo)(uint8_t *val, uintptr_t addr);
	uint32_t len;
	uint8_t *buf;
	uint32_t sr;
	int ret;

	if (op->data.dir == SPI_MEM_DATA_IN) {
		fifo = stm32_ospi_read_fifo;
	} else {
		fifo = stm32_ospi_write_fifo;
	}

	buf = (uint8_t *)op->data.buf;

	for (len = op->data.nbytes; len != 0U; len--) {
		ret = mmio_read32_poll_timeout(ospi_base() + _OSPI_SR, sr,
					       (sr & _OSPI_SR_FTF) != 0U,
					       _OSPI_FIFO_TIMEOUT_US);
		if (ret != 0) {
			ERROR("%s: fifo timeout\n", __func__);
			return ret;
		}

		fifo(buf++, ospi_base() + _OSPI_DR);
	}

	return 0;
}

static int stm32_ospi_mm(const struct spi_mem_op *op)
{
	const struct stm32_omi_config *drv_cfg = stm32_ospi.drv_cfg;

	memcpy(op->data.buf,
	       (void *)(drv_cfg->mm_base + (size_t)op->addr.val),
	       op->data.nbytes);

	return 0;
}

static int stm32_ospi_tx(const struct spi_mem_op *op, uint8_t fmode)
{
	if (op->data.nbytes == 0U) {
		return 0;
	}

	if (fmode == _OSPI_CR_FMODE_MM) {
		return stm32_ospi_mm(op);
	}

	return stm32_ospi_poll(op);
}

static unsigned int stm32_ospi_get_mode(uint8_t buswidth)
{
	switch (buswidth) {
	case SPI_MEM_BUSWIDTH_8_LINE:
		return 4U;
	case SPI_MEM_BUSWIDTH_4_LINE:
		return 3U;
	default:
		return buswidth;
	}
}

static int stm32_ospi_send(const struct spi_mem_op *op, uint8_t fmode)
{
	uint32_t ccr;
	uint32_t dcyc = 0U;
	uint32_t cr;
	int ret;

	VERBOSE("%s: cmd:%x mode:%d.%d.%d.%d addr:%x len:%x\n",
		__func__, op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	ret = stm32_ospi_wait_for_not_busy();
	if (ret != 0) {
		return ret;
	}

	if (op->data.nbytes != 0U) {
		mmio_write_32(ospi_base() + _OSPI_DLR, op->data.nbytes - 1U);
	}

	if ((op->dummy.buswidth != 0U) && (op->dummy.nbytes != 0U)) {
		dcyc = op->dummy.nbytes * 8U / op->dummy.buswidth;
	}

	mmio_clrsetbits_32(ospi_base() + _OSPI_TCR, _OSPI_TCR_DCYC, dcyc);

	mmio_clrsetbits_32(ospi_base() + _OSPI_CR, _OSPI_CR_FMODE,
			   fmode << _OSPI_CR_FMODE_SHIFT);

	ccr = stm32_ospi_get_mode(op->cmd.buswidth);

	if (op->addr.nbytes != 0U) {
		ccr |= (op->addr.nbytes - 1U) << _OSPI_CCR_ADSIZE_SHIFT;
		ccr |= stm32_ospi_get_mode(op->addr.buswidth) <<
		       _OSPI_CCR_ADMODE_SHIFT;
	}

	if (op->data.nbytes != 0U) {
		ccr |= stm32_ospi_get_mode(op->data.buswidth) <<
		       _OSPI_CCR_DMODE_SHIFT;
	}

	mmio_write_32(ospi_base() + _OSPI_CCR, ccr);

	mmio_write_32(ospi_base() + _OSPI_IR, op->cmd.opcode);

	if ((op->addr.nbytes != 0U) && (fmode != _OSPI_CR_FMODE_MM)) {
		mmio_write_32(ospi_base() + _OSPI_AR, op->addr.val);
	}

	ret = stm32_ospi_tx(op, fmode);

	/*
	 * Abort in:
	 * - Error case.
	 * - Memory mapped read: prefetching must be stopped if we read the last
	 *   byte of device (device size - fifo size). If device size is not
	 *   known then prefetching is always stopped.
	 */
	if ((ret != 0) || (fmode == _OSPI_CR_FMODE_MM)) {
		goto abort;
	}

	/* Wait end of TX in indirect mode */
	ret = stm32_ospi_wait_cmd(op);
	if (ret != 0) {
		goto abort;
	}

	return 0;

abort:
	mmio_setbits_32(ospi_base() + _OSPI_CR, _OSPI_CR_ABORT);

	/* Wait clear of abort bit by hardware */
	ret = mmio_read32_poll_timeout(ospi_base() + _OSPI_CR, cr,
				       (cr & _OSPI_CR_ABORT) == 0U,
				       _OSPI_ABT_TIMEOUT_US);

	mmio_write_32(ospi_base() + _OSPI_FCR, _OSPI_FCR_CTCF);

	if (ret != 0) {
		ERROR("%s: exec op error\n", __func__);
	}

	return ret;
}

static int stm32_ospi_exec_op(const struct spi_mem_op *op)
{
	uint8_t fmode = _OSPI_CR_FMODE_INDW;

	if ((op->data.dir == SPI_MEM_DATA_IN) && (op->data.nbytes != 0U)) {
		fmode = _OSPI_CR_FMODE_INDR;
	}

	return stm32_ospi_send(op, fmode);
}

static int stm32_ospi_dirmap_read(const struct spi_mem_op *op)
{
	const struct stm32_omi_config *drv_cfg = stm32_ospi.drv_cfg;
	uint8_t fmode = _OSPI_CR_FMODE_INDR;
	size_t addr_max;

	addr_max = op->addr.val + op->data.nbytes + 1U;
	if ((addr_max < drv_cfg->mm_size) && (op->addr.buswidth != 0U)) {
		fmode = _OSPI_CR_FMODE_MM;
	}

	return stm32_ospi_send(op, fmode);
}

static int stm32_ospi_claim_bus(unsigned int cs)
{
	uint32_t cr;

	if (cs >= _OSPI_MAX_CHIP) {
		return -ENODEV;
	}

	/* Set chip select and enable the controller */
	cr = _OSPI_CR_EN;
	if (cs == 1U) {
		cr |= _OSPI_CR_CSSEL;
	}

	mmio_clrsetbits_32(ospi_base() + _OSPI_CR, _OSPI_CR_CSSEL, cr);

	return 0;
}

static void stm32_ospi_release_bus(void)
{
	mmio_clrbits_32(ospi_base() + _OSPI_CR, _OSPI_CR_EN);
}

static int stm32_ospi_set_speed(unsigned int hz)
{
	const struct stm32_omi_config *drv_cfg = stm32_ospi.drv_cfg;
	struct clk *clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	unsigned long ospi_clk = clk_get_rate(clk);
	uint32_t prescaler = UINT8_MAX;
	uint32_t csht;
	int ret;

	if (ospi_clk == 0U) {
		return -EINVAL;
	}

	if (hz > 0U) {
		prescaler = div_round_up(ospi_clk, hz) - 1U;
		if (prescaler > UINT8_MAX) {
			prescaler = UINT8_MAX;
		}
	}

	csht = div_round_up((5U * ospi_clk) / (prescaler + 1U), _FREQ_100MHZ);
	csht = ((csht - 1U) << _OSPI_DCR1_CSHT_SHIFT) & _OSPI_DCR1_CSHT;

	ret = stm32_ospi_wait_for_not_busy();
	if (ret != 0) {
		return ret;
	}

	mmio_clrsetbits_32(ospi_base() + _OSPI_DCR2, _OSPI_DCR2_PRESCALER,
			   prescaler);

	mmio_clrsetbits_32(ospi_base() + _OSPI_DCR1, _OSPI_DCR1_CSHT, csht);

	VERBOSE("%s: speed=%lu\n", __func__, ospi_clk / (prescaler + 1U));

	return 0;
}

static int stm32_ospi_set_mode(unsigned int mode)
{
	int ret;

	if ((mode & SPI_CS_HIGH) != 0U) {
		return -ENODEV;
	}

	ret = stm32_ospi_wait_for_not_busy();
	if (ret != 0) {
		return ret;
	}

	if (((mode & SPI_CPHA) != 0U) && ((mode & SPI_CPOL) != 0U)) {
		mmio_setbits_32(ospi_base() + _OSPI_DCR1, _OSPI_DCR1_CKMODE);
	} else if (((mode & SPI_CPHA) == 0U) && ((mode & SPI_CPOL) == 0U)) {
		mmio_clrbits_32(ospi_base() + _OSPI_DCR1, _OSPI_DCR1_CKMODE);
	} else {
		return -ENODEV;
	}

#if DEBUG
	VERBOSE("%s: mode=0x%x\n", __func__, mode);

	if ((mode & SPI_RX_OCTAL) != 0U) {
		VERBOSE("rx: octal\n");
	} else if ((mode & SPI_RX_QUAD) != 0U) {
		VERBOSE("rx: quad\n");
	} else if ((mode & SPI_RX_DUAL) != 0U) {
		VERBOSE("rx: dual\n");
	} else {
		VERBOSE("rx: single\n");
	}

	if ((mode & SPI_TX_OCTAL) != 0U) {
		VERBOSE("tx: octal\n");
	} else if ((mode & SPI_TX_QUAD) != 0U) {
		VERBOSE("tx: quad\n");
	} else if ((mode & SPI_TX_DUAL) != 0U) {
		VERBOSE("tx: dual\n");
	} else {
		VERBOSE("tx: single\n");
	}
#endif

	return 0;
}

static const struct spi_bus_ops stm32_ospi_bus_ops = {
	.claim_bus = stm32_ospi_claim_bus,
	.release_bus = stm32_ospi_release_bus,
	.set_speed = stm32_ospi_set_speed,
	.set_mode = stm32_ospi_set_mode,
	.exec_op = stm32_ospi_exec_op,
	.dirmap_read = stm32_ospi_dirmap_read,
};

int stm32_ospi_init(void)
{
	int ret;

	ret = stm32_ospi_get_platdata(&stm32_ospi);
	if (ret != 0) {
		return ret;
	}

	mmio_write_32(ospi_base() + _OSPI_TCR, _OSPI_TCR_SSHIFT);
	mmio_write_32(ospi_base() + _OSPI_DCR1,
		      _OSPI_DCR1_DEVSIZE | _OSPI_DCR1_DLYBYP);

	return spi_mem_init_slave(stm32_ospi.spi_slave, &stm32_ospi_bus_ops);
};

int stm32_ospi_deinit(void)
{
	const struct stm32_omi_config *drv_cfg = stm32_ospi.drv_cfg;
	struct clk *clk;
	int err, i;

	clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);

	for (i = 0; i < drv_cfg->n_rst; i++) {
		err = reset_control_reset(&drv_cfg->rst_ctl[i]);
		if (err)
			return err;
	}

	clk_disable(clk);

	return 0;
}

/********************************************************/
struct stm32_omi_data {
	struct spi_slave *spi_slave;
};

int stm32_omi_init(const struct device *dev)
{
	const struct stm32_omi_config *drv_cfg = dev_get_config(dev);
	struct clk *clk;
	int err, i;

	clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	if (!clk)
		return -ENODEV;

	err = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err)
		return err;

	err = clk_enable(clk);
	if (err)
		return err;

	for (i = 0; i < drv_cfg->n_rst; i++) {
		err = reset_control_reset(&drv_cfg->rst_ctl[i]);
		if (err)
			return err;
	}

	mmio_write_32(drv_cfg->base + _OSPI_TCR, _OSPI_TCR_SSHIFT);
	mmio_write_32(drv_cfg->base + _OSPI_DCR1,
		      _OSPI_DCR1_DEVSIZE | _OSPI_DCR1_DLYBYP);

	/* temporaire, wait full integration*/
	stm32_ospi.drv_cfg = drv_cfg;

	return 0;
}

#define STM32_OMI_INIT(n)							\
										\
PINCTRL_DT_INST_DEFINE(n);							\
										\
static const struct reset_control rst_ctrl_##n[] = DT_INST_RESETS_CONTROL(n);	\
										\
static const struct stm32_omi_config stm32_omi_cfg_##n = {			\
	.base = DT_INST_REG_ADDR_BY_NAME(n, omi),				\
	.mm_base = DT_INST_REG_ADDR_BY_NAME(n, omi_mm),				\
	.mm_size = DT_INST_REG_SIZE_BY_NAME(n, omi_mm),				\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),		\
	.rst_ctl = rst_ctrl_##n,						\
        .n_rst = DT_INST_NUM_RESETS(n),						\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
};										\
										\
static struct stm32_omi_data stm32_omi_data_##n = {};				\
										\
DEVICE_DT_INST_DEFINE(n,							\
		      &stm32_omi_init,						\
		      &stm32_omi_data_##n, &stm32_omi_cfg_##n,			\
		      CORE, 10,							\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_OMI_INIT)
