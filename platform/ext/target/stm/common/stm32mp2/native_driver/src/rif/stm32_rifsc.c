// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#include <drivers/stm32_rifsc.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <tee_api_defines.h>
#include <trace.h>
#include <util.h>

/* RIFSC offset register */
#define _RIFSC_RISC_SECCFGR0		U(0x10)
#define _RIFSC_RISC_PRIVCFGR0		U(0x30)
#define _RIFSC_RISC_PER0_CIDCFGR	U(0x100)
#define _RIFSC_RIMC_ATTR0		U(0xC10)

#define _RIFSC_HWCFGR3			U(0xFE8)
#define _RIFSC_HWCFGR2			U(0xFEC)
#define _RIFSC_HWCFGR1			U(0xFF0)
#define _RIFSC_VERR			U(0xFF4)

/* RIFSC_HWCFGR2 register fields */
#define _RIFSC_HWCFGR2_CFG1_MASK	GENMASK_32(15, 0)
#define _RIFSC_HWCFGR2_CFG1_SHIFT	0
#define _RIFSC_HWCFGR2_CFG2_MASK	GENMASK_32(23, 16)
#define _RIFSC_HWCFGR2_CFG2_SHIFT	16
#define _RIFSC_HWCFGR2_CFG3_MASK	GENMASK_32(31, 24)
#define _RIFSC_HWCFGR2_CFG3_SHIFT	24

/* RIFSC_HWCFGR1 register fields */
#define _RIFSC_HWCFGR1_CFG1_MASK	GENMASK_32(3, 0)
#define _RIFSC_HWCFGR1_CFG1_SHIFT	0
#define _RIFSC_HWCFGR1_CFG2_MASK	GENMASK_32(7, 4)
#define _RIFSC_HWCFGR1_CFG2_SHIFT	4
#define _RIFSC_HWCFGR1_CFG3_MASK	GENMASK_32(11, 8)
#define _RIFSC_HWCFGR1_CFG3_SHIFT	8
#define _RIFSC_HWCFGR1_CFG4_MASK	GENMASK_32(15, 12)
#define _RIFSC_HWCFGR1_CFG4_SHIFT	12
#define _RIFSC_HWCFGR1_CFG5_MASK	GENMASK_32(19, 16)
#define _RIFSC_HWCFGR1_CFG5_SHIFT	16
#define _RIFSC_HWCFGR1_CFG6_MASK	GENMASK_32(23, 20)
#define _RIFSC_HWCFGR1_CFG6_SHIFT	20

/* RIFSC_VERR register fields */
#define _RIFSC_VERR_MINREV_MASK		GENMASK_32(3, 0)
#define _RIFSC_VERR_MINREV_SHIFT	0
#define _RIFSC_VERR_MAJREV_MASK		GENMASK_32(7, 4)
#define _RIFSC_VERR_MAJREV_SHIFT	4

/* Periph id per register */
#define _PERIPH_IDS_PER_REG		32
#define _OFST_PERX_CIDCFGR		U(0x8)

/* max entries */
#define MAX_RIMU	16
#define MAX_RISUP	128

#define _RIF_FLD_PREP(field, value)	(((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _RIF_FLD_GET(field, value)	(((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))

/*
 * no common errno between component
 * define rif internal errno
 */
#define	RIF_ERR_NOMEM		12	/* Out of memory */
#define RIF_ERR_NODEV		19	/* No such device */
#define RIF_ERR_INVAL		22	/* Invalid argument */
#define RIF_ERR_NOTSUP		45	/* Operation not supported */

static struct rifsc_driver_data rifsc_drvdata;
static struct stm32_rifsc_platdata rifsc_pdata;

static void stm32_rifsc_get_driverdata(struct stm32_rifsc_platdata *pdata)
{
	uint32_t regval = 0;

	regval = io_read32(pdata->base + _RIFSC_HWCFGR1);
	rifsc_drvdata.rif_en = _RIF_FLD_GET(_RIFSC_HWCFGR1_CFG1, regval) != 0;
	rifsc_drvdata.sec_en = _RIF_FLD_GET(_RIFSC_HWCFGR1_CFG2, regval) != 0;
	rifsc_drvdata.priv_en = _RIF_FLD_GET(_RIFSC_HWCFGR1_CFG3, regval) != 0;

	regval = io_read32(pdata->base + _RIFSC_HWCFGR2);
	rifsc_drvdata.nb_risup = _RIF_FLD_GET(_RIFSC_HWCFGR2_CFG1, regval);
	rifsc_drvdata.nb_rimu = _RIF_FLD_GET(_RIFSC_HWCFGR2_CFG2, regval);
	rifsc_drvdata.nb_risal = _RIF_FLD_GET(_RIFSC_HWCFGR2_CFG3, regval);

	pdata->drv_data = &rifsc_drvdata;

	regval = io_read8(pdata->base + _RIFSC_VERR);

	DMSG("RIFSC version %"PRIu32".%"PRIu32,
	     _RIF_FLD_GET(_RIFSC_VERR_MAJREV, regval),
	     _RIF_FLD_GET(_RIFSC_VERR_MINREV, regval));

	DMSG("HW cap: enabled[rif:sec:priv]:[%s:%s:%s] nb[risup|rimu|risal]:[%"PRIu8",%"PRIu8",%"PRIu8"]",
	     rifsc_drvdata.rif_en ? "true" : "false",
	     rifsc_drvdata.sec_en ? "true" : "false",
	     rifsc_drvdata.priv_en ? "true" : "false",
	     rifsc_drvdata.nb_risup,
	     rifsc_drvdata.nb_rimu,
	     rifsc_drvdata.nb_risal);
}

#ifdef CFG_DT
#include <dt-bindings/soc/stm32mp25-rifsc.h>

#define RIFSC_COMPAT			"st,stm32-rifsc"
#define RIFSC_RISC_CFEN_MASK		BIT(0)
#define RIFSC_RISC_SEM_EN_MASK		BIT(1)
#define RIFSC_RISC_SCID_MASK		GENMASK_32(7, 4)
#define RIFSC_RISC_SEC_MASK		BIT(8)
#define RIFSC_RISC_PRIV_MASK		BIT(9)
#define RIFSC_RISC_SEML_MASK		GENMASK_32(23, 16)
#define RIFSC_RISC_PER_ID_MASK		GENMASK_32(31, 24)

#define RIFSC_RISC_PERx_CID_MASK	(RIFSC_RISC_CFEN_MASK | \
					 RIFSC_RISC_SEM_EN_MASK | \
					 RIFSC_RISC_SCID_MASK | \
					 RIFSC_RISC_SEML_MASK)

#define RIFSC_RIMC_MODE_MASK		BIT(2)
#define RIFSC_RIMC_MCID_MASK		GENMASK_32(6, 4)
#define RIFSC_RIMC_MSEC_MASK		BIT(8)
#define RIFSC_RIMC_MPRIV_MASK		BIT(9)
#define RIFSC_RIMC_M_ID_MASK		GENMASK_32(23, 16)

#define RIFSC_RIMC_ATTRx_MASK		(RIFSC_RIMC_MODE_MASK | \
					 RIFSC_RIMC_MCID_MASK | \
					 RIFSC_RIMC_MSEC_MASK | \
					 RIFSC_RIMC_MPRIV_MASK)

struct dt_id_attr {
	/* The effective size of the array is meaningless here */
	fdt32_t id_attr[1];
};

static int stm32_rifsc_dt_conf_risup(struct stm32_rifsc_platdata *pdata,
				     void *fdt, int node)
{
	const struct dt_id_attr *conf_list = NULL;
	int i = 0;
	int len = 0;

	conf_list = (const struct dt_id_attr *)fdt_getprop(fdt, node,
							   "st,risup", &len);
	if (!conf_list) {
		IMSG("No RISUP configuration in DT");
		return -RIF_ERR_NODEV;
	}

	len = len / sizeof(uint32_t);

	pdata->nrisup = len;
	pdata->risup = calloc(len, sizeof(*pdata->risup));
	if (!pdata->risup)
		return -RIF_ERR_NOMEM;

	for (i = 0; i < len; i++) {
		uint32_t value = fdt32_to_cpu(conf_list->id_attr[i]);
		struct risup_cfg *risup = pdata->risup + i;

		risup->id = _RIF_FLD_GET(RIFSC_RISC_PER_ID, value);
		risup->sec = (bool)_RIF_FLD_GET(RIFSC_RISC_SEC, value);
		risup->priv = (bool)_RIF_FLD_GET(RIFSC_RISC_PRIV, value);
		risup->cid_attr = _RIF_FLD_GET(RIFSC_RISC_PERx_CID, value);
	}

	return 0;
}

static int stm32_rifsc_dt_conf_rimu(struct stm32_rifsc_platdata *pdata,
				    void *fdt, int node)
{
	const struct dt_id_attr *conf_list = NULL;
	int i = 0;
	int len = 0;

	conf_list = (const struct dt_id_attr *)fdt_getprop(fdt, node,
							   "st,rimu", &len);
	if (!conf_list) {
		IMSG("No RIMU configuration in DT");
		return -RIF_ERR_NODEV;
	}

	len = len / sizeof(uint32_t);

	pdata->nrimu = len;
	pdata->rimu = calloc(len, sizeof(*pdata->rimu));
	if (!pdata->rimu)
		return -RIF_ERR_NOMEM;

	for (i = 0; i < len; i++) {
		uint32_t value = fdt32_to_cpu(conf_list->id_attr[i]);
		struct rimu_cfg *rimu = pdata->rimu + i;

		rimu->id = _RIF_FLD_GET(RIFSC_RIMC_M_ID, value);
		rimu->attr = _RIF_FLD_GET(RIFSC_RIMC_ATTRx, value);
	}

	return 0;
}

static int stm32_rifsc_parse_fdt(struct stm32_rifsc_platdata *pdata)
{
	static struct io_pa_va base;
	void *fdt = NULL;
	int node = 0;
	int err = 0;

	fdt = get_embedded_dt();
	node = fdt_node_offset_by_compatible(fdt, 0, RIFSC_COMPAT);
	if (node < 0)
		return -RIF_ERR_NODEV;

	base.pa = _fdt_reg_base_address(fdt, node);
	if (base.pa == DT_INFO_INVALID_REG)
		return -RIF_ERR_INVAL;

	pdata->base = io_pa_or_va_secure(&base);

	stm32_rifsc_get_driverdata(pdata);

	err = stm32_rifsc_dt_conf_risup(pdata, fdt, node);
	if (err)
		return err;

	return stm32_rifsc_dt_conf_rimu(pdata, fdt, node);
}

__weak int stm32_rifsc_get_platdata(struct stm32_rifsc_platdata *pdata __unused)
{
	/* In DT config, the platform datas are fill by DT file */
	return 0;
}

#else
static int stm32_rifsc_parse_fdt(struct stm32_rifsc_platdata *pdata)
{
	return -RIF_ERR_NOTSUP;
}

/*
 * This function could be overridden by platform to define
 * pdata of rifsc driver
 */
__weak int stm32_rifsc_get_platdata(struct stm32_rifsc_platdata *pdata)
{
	return -RIF_ERR_NODEV;
}
#endif /*CFG_DT*/

static int stm32_risup_cfg(struct stm32_rifsc_platdata *pdata,
			   struct risup_cfg *risup)
{
	struct rifsc_driver_data *drv_data = pdata->drv_data;
	uintptr_t offset = sizeof(uint32_t) * (risup->id / _PERIPH_IDS_PER_REG);
	uint32_t shift = risup->id % _PERIPH_IDS_PER_REG;

	if (!risup || risup->id >= drv_data->nb_risup)
		return -RIF_ERR_INVAL;

	if (drv_data->sec_en)
		io_clrsetbits32(pdata->base + _RIFSC_RISC_SECCFGR0 + offset,
				BIT(shift), risup->sec << shift);

	if (drv_data->priv_en)
		io_clrsetbits32(pdata->base + _RIFSC_RISC_PRIVCFGR0 + offset,
				BIT(shift), risup->priv << shift);

	if (drv_data->rif_en) {
		offset = _OFST_PERX_CIDCFGR * risup->id;
		io_write32(pdata->base + _RIFSC_RISC_PER0_CIDCFGR + offset,
			   risup->cid_attr);
	}

	return 0;
}

static int stm32_risup_setup(struct stm32_rifsc_platdata *pdata)
{
	struct rifsc_driver_data *drv_data = pdata->drv_data;
	int i = 0;
	int err = 0;

	for (i = 0; i < pdata->nrisup && i < drv_data->nb_risup; i++) {
		struct risup_cfg *risup = pdata->risup + i;

		err = stm32_risup_cfg(pdata, risup);
		if (err) {
			EMSG("risup cfg(%d/%d) error", i + 1, pdata->nrisup);
			return err;
		}
	}

	return 0;
}

static int stm32_rimu_cfg(struct stm32_rifsc_platdata *pdata,
			  struct rimu_cfg *rimu)
{
	struct rifsc_driver_data *drv_data = pdata->drv_data;
	uintptr_t offset =  _RIFSC_RIMC_ATTR0 + (sizeof(uint32_t) * rimu->id);

	if (!rimu || rimu->id >= drv_data->nb_rimu)
		return -RIF_ERR_INVAL;

	if (drv_data->rif_en)
		io_write32(pdata->base + offset, rimu->attr);

	return 0;
}

static int stm32_rimu_setup(struct stm32_rifsc_platdata *pdata)
{
	struct rifsc_driver_data *drv_data = pdata->drv_data;
	int i = 0;
	int err = 0;

	for (i = 0; i < pdata->nrimu && i < drv_data->nb_rimu; i++) {
		struct rimu_cfg *rimu = pdata->rimu + i;

		err = stm32_rimu_cfg(pdata, rimu);
		if (err) {
			EMSG("rimu cfg(%d/%d) error", i + 1, pdata->nrimu);
			return err;
		}
	}

	return 0;
}

int stm32_rifsc_init(void)
{
	int err = 0;

	err = stm32_rifsc_get_platdata(&rifsc_pdata);
	if (err)
		return err;

	err = stm32_rifsc_parse_fdt(&rifsc_pdata);
	if (err && err != -RIF_ERR_NOTSUP)
		return err;

	if (!rifsc_pdata.drv_data)
		stm32_rifsc_get_driverdata(&rifsc_pdata);

	err = stm32_risup_setup(&rifsc_pdata);
	if (err)
		return err;

	return stm32_rimu_setup(&rifsc_pdata);
}

static TEE_Result rifsc_init(void)
{
	if (stm32_rifsc_init()) {
		panic();
		return TEE_ERROR_GENERIC;
	}

	/*
	 * There is only a boot init, no dynamic service.
	 * The memory can be freeing
	 */
	free(rifsc_pdata.risup);
	free(rifsc_pdata.rimu);

	return TEE_SUCCESS;
}
early_init(rifsc_init);
