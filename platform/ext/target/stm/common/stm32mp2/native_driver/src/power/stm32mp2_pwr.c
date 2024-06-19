/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_pwr

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <device.h>
#include <debug.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <reset.h>

#include <stm32mp2_pwr.h>
#include <stm32_rif.h>

/* PWR offset register */
#define _PWR_RSECCFGR			U(0x100)
#define _PWR_RPRIVCFGR			U(0x104)
#define _PWR_RCIDCFGR			U(0x108)
#define _PWR_WIOSECCFGR			U(0x180)
#define _PWR_WIOPRIVCFGR		U(0x184)
#define _PWR_WIOCIDCFGR			U(0x188)
#define _PWR_WIOSEMCR			U(0x18C)

// RCIDCFGR register bitfields
#define _RCIDCFGR_CFEN_MASK		BIT(0)
#define _RCIDCFGR_CFEN_SHIFT		0
#define _RCIDCFGR_SCID_MASK		GENMASK_32(6, 4)
#define _RCIDCFGR_SCID_SHIFT		4

// WIOCIDCFGR register bitfields
#define _WIOCIDCFGR_CFEN_MASK		BIT(0)
#define _WIOCIDCFGR_CFEN_SHIFT		0
#define _WIOCIDCFGR_SEMEN_MASK		BIT(1)
#define _WIOCIDCFGR_SEMEN_SHIFT		1
#define _WIOCIDCFGR_SCID_MASK		GENMASK_32(6, 4)
#define _WIOCIDCFGR_SCID_SHIFT		4
#define _WIOCIDCFGR_SEMWLC_MASK		GENMASK_32(23, 16)
#define _WIOCIDCFGR_SEMWLC_SHIFT	16

// SEMCR register bitfields
#define _WIOSEMCR_MUTEX_MASK		BIT(0)
#define _WIOSEMCR_MUTEX_SHIFT		0
#define _WIOSEMCR_SCID_MASK		GENMASK_32(6, 4)
#define _WIOSEMCR_SCID_SHIFT		U(4)

#define SEMAPHORE_IS_AVAILABLE(cid_cfgr, my_id)				\
	(_FLD_GET(_WIOCIDCFGR_CFEN, cid_cfgr) &&			\
	 _FLD_GET(_WIOCIDCFGR_SEMEN, cid_cfgr) &&			\
	 ((_FLD_GET(_WIOCIDCFGR_SEMWLC, cid_cfgr)) & BIT(my_id)))

/* RIF miscellaneous */
#define PWR_RIF_RES			U(13)
#define PWR_RIF_FIRST_WIO_ID		U(7)
#define RCID_X_OFFSET(_id)		(U(0x4) * (_id))
#define WIOCID_X_OFFSET(_id)		(U(0x8) * (_id - PWR_RIF_FIRST_WIO_ID))
#define WIOSEM_X_OFFSET(_id)		(U(0x8) * (_id - PWR_RIF_FIRST_WIO_ID))

#define MY_CID RIF_CID2

#define _PWR_CPU_MIN 1
#define _PWR_CPU_MAX 2

struct stm32mp2_pwr_config {
	uintptr_t base;
	const struct reset_control rst_ctl_bck;
	const struct rifprot_controller *rif_ctl;
};

/*
 * FIXME
 * must be reworked when we use copro service and power
 */
/*static uint32_t _cpux_base(uint32_t cpu)*/
/*{*/
/*        uint32_t offset = _PWR_CPU1D1SR;*/

/*        if (cpu < _PWR_CPU_MIN || cpu > _PWR_CPU_MAX)*/
/*                return 0;*/

/*        offset += sizeof(uint32_t) * (cpu - _PWR_CPU_MIN);*/
/*        return pdata.base + offset;*/
/*}*/

/*static int _cpu_state(uint32_t cpu, uint32_t *state)*/
/*{*/
/*        uint32_t cpux_base;*/

/*        cpux_base = _cpux_base(cpu);*/
/*        if (!cpux_base) {*/
/*                IMSG("cpu:%d not valid");*/
/*                return -1;*/
/*        }*/

/*        *state = mmio_read_32(cpux_base);*/
/*        return 0;*/
/*}*/

/*enum c_state stm32_pwr_cpu_get_cstate(uint32_t cpu)*/
/*{*/
/*        uint32_t state;*/

/*        if (_cpu_state(cpu, &state))*/
/*                return CERR;*/

/*        return _FLD_GET(_PWR_CPUXDXSR_CSTATE, state);*/
/*}*/

/*enum d_state stm32_pwr_cpu_get_dstate(uint32_t cpu)*/
/*{*/
/*        uint32_t state;*/

/*        if (_cpu_state(cpu, &state))*/
/*                return DERR;*/

/*        return _FLD_GET(_PWR_CPUXDXSR_DSTATE, state);*/
/*}*/

/*
 * There are two kinds of local resources in the PWR:
 *  - non-shareable resources (R0 to R6), that can be statically assigned
 *    to only one master.
 *  - shareable wake-up I/O resources (WIO1 to WIO6) that can be statically
 *    assigned to one master, or shared between different masters with
 *    semaphore protection
 */
static __unused
int stm32mp2_pwr_rif_acquire_sem(const struct rifprot_controller *ctl,
				 uint32_t id)
{
	uint32_t semcr;

	if (id < PWR_RIF_FIRST_WIO_ID)
		return 0;

	io_setbits32(ctl->rbase->sem + WIOSEM_X_OFFSET(id), _WIOSEMCR_MUTEX_MASK);

	semcr = io_read32(ctl->rbase->sem + WIOSEM_X_OFFSET(id));
	if (semcr != (_WIOSEMCR_MUTEX_MASK | _FLD_PREP(_WIOSEMCR_SCID, MY_CID)))
		return -EPERM;

	return 0;
}

static __unused
int stm32mp2_pwr_rif_release_sem(const struct rifprot_controller *ctl,
				 uint32_t id)
{
	uint32_t semcr;

	if (id < PWR_RIF_FIRST_WIO_ID)
		return 0;

	semcr = io_read32(ctl->rbase->sem + WIOSEM_X_OFFSET(id));
	/* if no semaphore */
	if (!(semcr & _WIOSEMCR_MUTEX_MASK))
		return 0;

	/* if semaphore taken but not my cid */
	if (semcr != (_WIOSEMCR_MUTEX_MASK | _FLD_PREP(_WIOSEMCR_SCID, MY_CID)))
		return -EPERM;

	io_clrbits32(ctl->rbase->sem + WIOSEM_X_OFFSET(id), _WIOSEMCR_MUTEX_MASK);

	return 0;
}

static __unused
int stm32mp2_pwr_rif_master_set_conf(const struct rifprot_controller *ctl,
				     struct rifprot_config *cfg)
{
	const struct stm32mp2_pwr_config *dev_cfg = dev_get_config(ctl->dev);
	uintptr_t base = dev_cfg->base;
	struct rif_base *rif_regs = (struct rif_base *)ctl->rbase;
	uint32_t shift = cfg->id;
	struct rif_base srbase = {
		.sec = base + _PWR_RSECCFGR,
		.priv = base + _PWR_RPRIVCFGR,
		.cid = base + _PWR_RCIDCFGR + RCID_X_OFFSET(cfg->id),
		.sem = 0,
	};

	if (cfg->id < PWR_RIF_FIRST_WIO_ID) {
		rif_regs = &srbase;
	} else {
		shift = cfg->id - PWR_RIF_FIRST_WIO_ID;
	}

	/* disable filtering befor write sec and priv cfgr */
	io_clrbits32(rif_regs->cid, _RCIDCFGR_CFEN_MASK);

	io_clrsetbits32(rif_regs->sec, BIT(shift), cfg->sec << shift);
	io_clrsetbits32(rif_regs->priv, BIT(shift), cfg->priv << shift);

	io_write32(rif_regs->cid, cfg->cid_attr);

	if (rif_regs->sem && SEMAPHORE_IS_AVAILABLE(cfg->cid_attr, MY_CID))
		return stm32_rifprot_acquire_sem(ctl, cfg->id);

	return 0;
}

static __unused
int stm32mp2_pwr_rif_set_conf(const struct rifprot_controller *ctl,
			      struct rifprot_config *cfg)
{
	uint32_t shift = cfg->id - PWR_RIF_FIRST_WIO_ID;
	bool write_cfg = false;
	uint32_t cidcfgr;
	int err = 0;

	/*
	 * if not TDCID
	 * write SECCFGR0 & PRIVCFGR0 if:
	 *  - shareable wake-up I/O resources (WIO1 to WIO6)
	 *  - SEM_EN=1 && SEMWLC=MY_CID && acquire semaphore
	 *  - SEM_EN=0 && SCID=MY_CID
	 */
	if (cfg->id < PWR_RIF_FIRST_WIO_ID)
		return -EPERM;

	cidcfgr = io_read32(ctl->rbase->cid + WIOCID_X_OFFSET(cfg->id));

	if (ctl->rbase->sem &&
	    SEMAPHORE_IS_AVAILABLE(cidcfgr, MY_CID)) {
		err = stm32_rifprot_acquire_sem(ctl, cfg->id);
		if (!err)
			write_cfg = true;
	} else if (!_FLD_GET(_WIOCIDCFGR_SEMEN, cidcfgr) &&
		   (_FLD_GET(_WIOCIDCFGR_SCID, cidcfgr) == MY_CID)) {
		write_cfg = true;
	}

	if (write_cfg) {
		io_clrsetbits32(ctl->rbase->sec, BIT(shift), cfg->sec << shift);
		io_clrsetbits32(ctl->rbase->priv, BIT(shift), cfg->priv << shift);
	}

	return err;
}

int stm32mp2_pwr_init(const struct device *dev)
{
	const struct stm32mp2_pwr_config *dev_cfg = dev_get_config(dev);
	uint32_t bdcr;

	/*
	 * Disable the backup domain write protection.
	 * The protection is enable at each reset by hardware
	 * and must be disabled by software.
	 */
	mmio_setbits_32(dev_cfg->base + _PWR_BDCR1, _PWR_BDCR1_DBD3P);
	mmio_read32_poll_timeout(dev_cfg->base + _PWR_BDCR1,
				 bdcr, (bdcr &  _PWR_BDCR1_DBD3P), 0);

	/* Reset backup domain on cold boot cases */
	reset_control_reset(&dev_cfg->rst_ctl_bck);

	if (dev_cfg->rif_ctl)
		return stm32_rifprot_init(dev_cfg->rif_ctl);

	return 0;
}

/*
 * FIXME:
 * when we add supply domain managment, we need to create a power domain
 * framework.
 */

#define PWR_RIF_SET_CONF_FUNC						\
	COND_CODE_1(IS_ENABLED(STM32_M33TDCID),				\
		    (stm32mp2_pwr_rif_master_set_conf),			\
		    (stm32mp2_pwr_rif_set_conf))

#define STM32_PWR_INIT(n)						\
									\
static __unused const struct rif_base rbase_##n = {			\
	.sec = DT_INST_REG_ADDR(n) + _PWR_WIOSECCFGR,			\
	.priv = DT_INST_REG_ADDR(n) + _PWR_WIOPRIVCFGR,			\
	.cid = DT_INST_REG_ADDR(n) + _PWR_WIOCIDCFGR,			\
	.sem = DT_INST_REG_ADDR(n) + _PWR_WIOSEMCR,			\
};									\
									\
static __unused struct rif_ops rops_##n = {				\
	.set_conf = PWR_RIF_SET_CONF_FUNC,				\
	.acquire_sem = stm32mp2_pwr_rif_acquire_sem,			\
	.release_sem = stm32mp2_pwr_rif_release_sem,			\
};									\
									\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, &rops_##n, PWR_RIF_RES);	\
									\
static const struct stm32mp2_pwr_config stm32mp2_pwr_cfg_##n = {	\
	.base = DT_INST_REG_ADDR(n),					\
	.rst_ctl_bck = DT_INST_RESET_CONTROL_GET(n),			\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),				\
};									\
									\
DEVICE_DT_INST_DEFINE(n, &stm32mp2_pwr_init,				\
		      NULL, &stm32mp2_pwr_cfg_##n,			\
		      PRE_CORE, 1, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_PWR_INIT)
