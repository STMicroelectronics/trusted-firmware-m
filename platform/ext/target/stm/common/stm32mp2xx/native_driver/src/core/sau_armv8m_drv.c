// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022, STMicroelectronics
 */
#include <cmsis.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <device.h>
#include <region.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#define DT_DRV_COMPAT arm_armv8m_sau

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one sau compatible node is supported");

#define _SAU_CTRL		U(0x00)
#define _SAU_TYPE		U(0x04)
#define _SAU_RNR		U(0x08)
#define _SAU_RBAR		U(0x0C)
#define _SAU_RLAR		U(0x10)
#define _SAU_SFSR		U(0x14)
#define _SAU_SFAR		U(0x18)

/* CTRL bitfield */
#define _CTRL_ENABLE_SHIFT	0
#define _CTRL_ENABLE_MASK	BIT(0)
#define _CTRL_ALLNS_SHIFT	1
#define _CTRL_ALLNS_MASK	BIT(1)

/* TYPE bitfield */
#define _TYPE_SREGION_SHIFT	0
#define _TYPE_SREGION_MASK	GENMASK_32(7,0)

/* RNR bitfield */
#define _RNR_SREGION_SHIFT	0
#define _RNR_SREGION_MASK	GENMASK_32(7,0)

/* RBAR bitfield */
#define _RBAR_BADDR_SHIFT	5
#define _RBAR_BADDR_MASK	GENMASK_32(31,5)

/* RLAR bitfield */
#define _RLAR_ENABLE_SHIFT	0
#define _RLAR_ENABLE_MASK	BIT(0)
#define _RLAR_NSC_SHIFT		1
#define _RLAR_NSC_MASK		BIT(1)
#define _RLAR_LADDR_SHIFT	5
#define _RLAR_LADDR_MASK	GENMASK_32(31,5)
#define _RLAR_ATTRS_MASK	(_RLAR_NSC_MASK | _RLAR_ENABLE_MASK)

/* if needed Peripheral region must be defined in soc header file */
#ifndef PERIPH_BASE_NS
#warning "Peripheral NS base region not defined"
#define PERIPH_BASE_NS
#endif
#ifndef PERIPH_SIZE
#warning "Peripheral NS size region not defined"
#define PERIPH_SIZE
#endif

enum sau_armv8m_attr_t {
    SAU_DIS,
    SAU_EN,
    SAU_NSC,
};

struct sau_region {
	uint32_t base;
	uint32_t limit;
	enum sau_armv8m_attr_t attr;
};

#define for_each_sau_region(rgt, rg, nr, idx)	\
	for (rg = ((struct sau_region *)rgt);	\
	     idx >= 0 && idx < (nr);		\
	     idx++, rg = rg + 1)


struct arm_sau_config {
	uintptr_t base;
	const struct sau_region *regions;
	const int n_regions;
};

struct arm_sau_data {
	uint32_t hw_n_regions;
};

static void arm_sau_set_region(const struct device *dev)
{
	const struct arm_sau_config *drv_cfg = dev_get_config(dev);
	struct sau_region *rg;
	int32_t idx = 0;

	/* Disable SAU */
	TZ_SAU_Disable();

	for_each_sau_region(drv_cfg->regions, rg, drv_cfg->n_regions, idx) {
		io_write32(drv_cfg->base + _SAU_RNR, idx);
		io_write32(drv_cfg->base + _SAU_RBAR, rg->base & _RBAR_BADDR_MASK);
		io_write32(drv_cfg->base + _SAU_RLAR, ((rg->limit & _RLAR_LADDR_MASK) |
						       (rg->attr & _RLAR_ATTRS_MASK)));
	}

	/* Force memory writes before continuing */
	__DSB();
	/* Flush and refill pipeline with updated permissions */
	__ISB();

	/* Enable SAU */
	TZ_SAU_Enable();
}

static void arm_sau_get_hwconfig(const struct device *dev)
{
	const struct arm_sau_config *drv_cfg = dev_get_config(dev);
	struct arm_sau_data *drv_data = dev_get_data(dev);

	drv_data->hw_n_regions = _FLD_GET(_TYPE_SREGION, io_read32(drv_cfg->base + _SAU_TYPE));
}

static int __maybe_unused arm_sau_init(const struct device *dev)
{
	const struct arm_sau_config *drv_cfg = dev_get_config(dev);
	struct arm_sau_data *drv_data = dev_get_data(dev);

	arm_sau_get_hwconfig(dev);

	if (drv_cfg->n_regions > drv_data->hw_n_regions)
		return -ENOTSUP;

	arm_sau_set_region(dev);

	return 0;
}

#define SAU_REGION(_base, _limit, _attr) {_base, _limit, _attr},

#define SAU_MR_PHANDLE(_node_id, _prop, _idx)							\
	DT_PHANDLE_BY_IDX(_node_id, _prop, _idx)

#define SAU_MR_HAS_SAU_NSC(mr_node_id)								\
	DT_PROP_OR(mr_node_id, arm_sau_nsc, 0)

#define SAU_MR_ATTR_EN(_mr_node_id)								\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(_mr_node_id), (SAU_EN), (SAU_DIS))

#define SAU_MR_ATTR_NSC(_mr_node_id)								\
	COND_CODE_1(SAU_MR_HAS_SAU_NSC(_mr_node_id), (SAU_NSC), (0))

#define SAU_MR_ATTRS(_mr_node_id)								\
	(SAU_MR_ATTR_NSC(_mr_node_id) | SAU_MR_ATTR_EN(_mr_node_id))

#define SAU_MR_REG_BASE(_mr_node_id)								\
	(DT_REG_ADDR(_mr_node_id))

#define SAU_MR_REG_LIMIT(_mr_node_id)								\
	(DT_REG_ADDR(_mr_node_id) + DT_REG_SIZE(_mr_node_id) - 1)

#define DT_SAU_REGION(_node_id, _prop, _idx)							\
	SAU_REGION(SAU_MR_REG_BASE(SAU_MR_PHANDLE(_node_id, _prop, _idx)),			\
		   SAU_MR_REG_LIMIT(SAU_MR_PHANDLE(_node_id, _prop, _idx)),			\
		   SAU_MR_ATTRS(SAU_MR_PHANDLE(_node_id, _prop, _idx)))

#define DT_INST_SAU_MEMORY_REGION(inst)								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, memory_region),					\
		    (DT_INST_FOREACH_PROP_ELEM_SEP(inst, memory_region, DT_SAU_REGION, ())),	\
		    ())

#define SAU_BUILTIN_PERIPHERAL(_base, _size)							\
	COND_CODE_1(IS_EMPTY(_base), (), (COND_CODE_1(IS_EMPTY(_size), (),			\
	(SAU_REGION(PERIPH_BASE_NS,								\
		   PERIPH_BASE_NS + PERIPH_SIZE - 1,						\
		   SAU_EN)))))

#ifndef TFM_LINKER_VENEERS_START
/*
 * if the user does not define a specific veneer area,
 * this region is automatically defined and sized by the linker
 */
REGION_DECLARE(Image$$, ER_VENEER, $$Base);
REGION_DECLARE(Image$$, VENEER_ALIGN, $$Limit);
#define VENEER_BASE	(uint32_t)&REGION_NAME(Image$$, ER_VENEER, $$Base)
#define VENEER_LIMIT	(uint32_t)&REGION_NAME(Image$$, VENEER_ALIGN, $$Limit)

#define SAU_BUILTIN_VENEER(n) SAU_REGION(VENEER_BASE, VENEER_LIMIT, SAU_EN | SAU_NSC)
#else
/* SAU defined by device tree */
#define SAU_BUILTIN_VENEER(n) ()
#endif

#define ARM_SAU_INIT(n)										\
												\
static const struct sau_region sau_dt_regions_##n[] = {						\
	SAU_BUILTIN_PERIPHERAL(PERIPH_BASE_NS, PERIPH_SIZE)					\
	SAU_BUILTIN_VENEER(n)									\
	DT_INST_SAU_MEMORY_REGION(n)								\
};												\
												\
static const struct arm_sau_config cfg_##n = {							\
	.base = DT_INST_REG_ADDR(n),								\
	.regions = sau_dt_regions_##n,								\
	.n_regions = ARRAY_SIZE(sau_dt_regions_##n),						\
};												\
												\
static struct arm_sau_data data_##n = {};							\
												\
DEVICE_DT_INST_DEFINE(n, &arm_sau_init, NULL,							\
		      &data_##n,								\
		      &cfg_##n,									\
		      ARCH, 1,									\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(ARM_SAU_INIT)
