// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <stm32_rif.h>

#include <dt-bindings/rif/stm32mp25-rif.h>

#define _PERIPH_IDS_PER_REG	32
#define SEC_PRIV_X_OFFSET(_id)	(U(0x4) * (_id / _PERIPH_IDS_PER_REG))
#define SEC_PRIV_X_SHIFT(_id)	(_id % _PERIPH_IDS_PER_REG)
#define CID_SEM_X_OFFSET(_id)	(U(0x8) * (_id))

// CIDCFGR register bitfields
#define _CIDCFGR_CFEN_MASK	BIT(0)
#define _CIDCFGR_CFEN_SHIFT	0
#define _CIDCFGR_SEMEN_MASK	BIT(1)
#define _CIDCFGR_SEMEN_SHIFT	1
#define _CIDCFGR_SCID_MASK	GENMASK_32(6, 4)
#define _CIDCFGR_SCID_SHIFT	4
#define _CIDCFGR_SEMWLC_MASK	GENMASK_32(23, 16)
#define _CIDCFGR_SEMWLC_SHIFT	16

#define SEMAPHORE_IS_AVAILABLE(cid_cfgr, my_id)			\
	(_FLD_GET(_CIDCFGR_CFEN, cid_cfgr) &&			\
	 _FLD_GET(_CIDCFGR_SEMEN, cid_cfgr) &&			\
	 ((_FLD_GET(_CIDCFGR_SEMWLC, cid_cfgr)) == BIT(my_id)))

// SEMCR register bitfields
#define _SEMCR_MUTEX_MASK	BIT(0)
#define _SEMCR_MUTEX_SHIFT	0
#define _SEMCR_SCID_MASK	GENMASK_32(6, 4)
#define _SEMCR_SCID_SHIFT	U(4)

#define MY_CID RIF_CID2

#if (IS_ENABLED(STM32_SEC))
// tfm_s
int stm32_rifprot_setup(const struct rifprot_controller *rifprot_ctl,
			struct rifprot_config *rifprot_cfg);

static int stm32_rifprot_semaphore_acquire(const struct rifprot_controller *rifprot_ctl,
					   uint32_t id)
{
	uint32_t semcr;

	io_setbits32(rifprot_ctl->rbase.sem + CID_SEM_X_OFFSET(id), _SEMCR_MUTEX_MASK);

	semcr = io_read32(rifprot_ctl->rbase.sem + CID_SEM_X_OFFSET(id));
	if (semcr != (_SEMCR_MUTEX_MASK | _FLD_PREP(_SEMCR_SCID, MY_CID)))
		return -EPERM;

	return 0;
}

int stm32_rifprot_set_config(const struct rifprot_controller *rifprot_ctl,
			     const struct rifprot_config *rifprot_cfg, int ncfg)
{
	int nelem = rifprot_ctl->nrifprot;
	struct rifprot_config *rcfg_elem;
	int i = 0;
	int err;

	for_each_rifprot_cfg(rifprot_cfg, rcfg_elem, ncfg, i) {
		err = stm32_rifprot_setup(rifprot_ctl, rcfg_elem);
		if (err) {
			EMSG("rifprot id:%d setup fail",
			     rcfg_elem->id, nelem);
			return err;
		}
	}

	return 0;
}

int stm32_rifprot_init(const struct rifprot_controller *rifprot_ctl)
{
	return stm32_rifprot_set_config(rifprot_ctl, rifprot_ctl->rifprot_cfg,
					rifprot_ctl->nrifprot);
}

#if (IS_ENABLED(STM32_M33TDCID))
// tfm_s_cm33tdcid
int stm32_rifprot_setup(const struct rifprot_controller *rifprot_ctl,
			struct rifprot_config *rifprot_cfg)
{
	uintptr_t offset = SEC_PRIV_X_OFFSET(rifprot_cfg->id);
	uint32_t shift = SEC_PRIV_X_SHIFT(rifprot_cfg->id);

	if (rifprot_cfg->id >= rifprot_ctl->nperipherals)
		return -EINVAL;

	io_clrsetbits32(rifprot_ctl->rbase.sec + offset, BIT(shift),
			rifprot_cfg->sec << shift);

	io_clrsetbits32(rifprot_ctl->rbase.priv + offset, BIT(shift),
			rifprot_cfg->priv << shift);

	io_write32(rifprot_ctl->rbase.cid + CID_SEM_X_OFFSET(rifprot_cfg->id),
		   rifprot_cfg->cid_attr);

	if (rifprot_ctl->rbase.sem &&
	    SEMAPHORE_IS_AVAILABLE(rifprot_cfg->cid_attr, MY_CID))
		return stm32_rifprot_semaphore_acquire(rifprot_ctl,
						       rifprot_cfg->id);

	return 0;
}

#else
//tfm_s_ca35tdcid
int stm32_rifprot_setup(const struct rifprot_controller *rifprot_ctl,
			struct rifprot_config *rifprot_cfg)
{
	uintptr_t offset = SEC_PRIV_X_OFFSET(rifprot_cfg->id);
	uint32_t shift = SEC_PRIV_X_SHIFT(rifprot_cfg->id);
	bool write_cfg = false;
	uint32_t cidcfgr;
	int err = 0;

	/*
	 * if not TDCID
	 * write RCC_SECCFGR0 & RCC_PRIVCFGR0 if:
	 *  - SEM_EN=1 && SEMWLC=MY_CID && acquire semaphore
	 *  - SEM_EN=0 && SCID=MY_CID
	 */

	cidcfgr = io_read32(rifprot_ctl->rbase.cid +
			    CID_SEM_X_OFFSET(rifprot_cfg->id));

	if (rifprot_ctl->rbase.sem &&
	    SEMAPHORE_IS_AVAILABLE(cidcfgr, MY_CID)) {
		err = stm32_rifprot_semaphore_acquire(rifprot_ctl,
						      rifprot_cfg->id);
		if (!err)
			write_cfg = true;
	} else if (!_FLD_GET(_CIDCFGR_SEMEN, cidcfgr) &&
		   (_FLD_GET(_CIDCFGR_SCID, cidcfgr) == MY_CID)) {
		write_cfg = true;
	}

	if (write_cfg) {
		io_clrsetbits32(rifprot_ctl->rbase.sec + offset,
				BIT(shift), rifprot_cfg->sec << shift);

		io_clrsetbits32(rifprot_ctl->rbase.priv + offset,
				BIT(shift), rifprot_cfg->priv << shift);
	}

	return err;
}
#endif
#endif
