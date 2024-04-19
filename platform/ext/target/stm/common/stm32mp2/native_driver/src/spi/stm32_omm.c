/*
 * Copyright (c) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_omm

#include <stdint.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/timeout.h>
#include <lib/utils_def.h>
#include <debug.h>
#include <clk.h>
#include <pinctrl.h>
#include <reset.h>
#include <stm32_rif.h>
#include <syscon.h>

/*
 * OCTOSPIM registers
 */
#define _OCTOSPIM_CR				0x0000U

/*
 * OCTOSPIM CR register bitfields
 */
#define _OCTOSPIM_CR_MUXEN			BIT(0)
#define _OCTOSPIM_CR_MUXEN_MODE_MASK		GENMASK_32(1, 0)
#define _OCTOSPIM_CR_CSSEL_OVR_EN		BIT(4)
#define _OCTOSPIM_CR_CSSEL_OVR_MASK		GENMASK_32(6, 4)
#define _OCTOSPIM_CR_CSSEL_OVR_SHIFT		5U
#define _OCTOSPIM_CR_REQ2ACK_MASK		GENMASK_32(23, 16)
#define _OCTOSPIM_CR_REQ2ACK_SHIFT		16U

/* OCTOSPI SYSCFG address mapping control register */
#define SYSCFG_OCTOSPIAMCR_MM1_256_MM2_0	0U
#define SYSCFG_OCTOSPIAMCR_MM1_192_MM2_64	1U
#define SYSCFG_OCTOSPIAMCR_MM1_128_MM2_128	2U
#define SYSCFG_OCTOSPIAMCR_MM1_64_MM2_192	3U
#define SYSCFG_OCTOSPIAMCR_MM1_0_MM2_256	4U

#define SZ_64M					0x04000000U
#define SZ_128M					0x08000000U
#define SZ_192M					0x0C000000U
#define SZ_256M					0x10000000U

#define OSPI_NB					2U

struct stm32_mm_region {
	uintptr_t start;
	uintptr_t end;
};

struct stm32_ospi_cfg {
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct reset_control rst_ctl;
	const struct reset_control rst_ctl_dll;
};

struct stm32_omm_cfg {
	uintptr_t base;
	uintptr_t mm_base;
	size_t mm_size;
	uintptr_t mm_ospi1_base;
	size_t mm_ospi1_size;
	uintptr_t mm_ospi2_base;
	size_t mm_ospi2_size;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct pinctrl_dev_config *pcfg;
	const struct rifprot_controller *rif_ctrl;
	const struct rifprot_config *rif_cfg;
	int n_rif_cfg;
	const struct device *amcr_dev;
	uint16_t amcr_base;
	uint8_t amcr_mask;
	uint8_t mux;
	uint32_t req2ack;
	uint8_t cssel_ovr;
	const struct stm32_ospi_cfg *ospi_cfg;
	int n_ospi_cfg;
};

static bool stm32_omm_region_contains(struct stm32_mm_region *r1,
				      struct stm32_mm_region *r2)
{
	return (r1->start <= r2->start) && (r1->end >= r2->end);
}

static bool stm32_omm_region_overlaps(struct stm32_mm_region *r1,
				      struct stm32_mm_region *r2)
{
	return (r1->start <= r2->end) && (r1->end >= r2->start);
}

static int stm32_omm_set_amcr(const struct device *dev)
{
	const struct stm32_omm_cfg *drv_cfg = dev_get_config(dev);
	uint32_t amcr;

	if ((drv_cfg->mm_ospi1_size + drv_cfg->mm_ospi2_size) != SZ_256M) {
		return -EINVAL;
	}

	switch (drv_cfg->mm_ospi1_size) {
	case 0U:
		amcr = SYSCFG_OCTOSPIAMCR_MM1_0_MM2_256;
		break;
	case SZ_64M:
		amcr = SYSCFG_OCTOSPIAMCR_MM1_64_MM2_192;
		break;
	case SZ_128M:
		amcr = SYSCFG_OCTOSPIAMCR_MM1_128_MM2_128;
		break;
	case SZ_192M:
		amcr = SYSCFG_OCTOSPIAMCR_MM1_192_MM2_64;
		break;
	case SZ_256M:
		amcr = SYSCFG_OCTOSPIAMCR_MM1_256_MM2_0;
		break;
	default:
		return -EINVAL;
	}

	syscon_clrsetbits(drv_cfg->amcr_dev, drv_cfg->amcr_base,
			  drv_cfg->amcr_mask, amcr & drv_cfg->amcr_mask);

	return 0;
}

static int stm32_omm_set_mm(const struct device *dev)
{
	const struct stm32_omm_cfg *drv_cfg = dev_get_config(dev);
	unsigned int ospi_i;
	struct stm32_mm_region mm_region;
	struct stm32_mm_region ospi_region[OSPI_NB];
	unsigned int valid_regions = 0U;

	mm_region.start = drv_cfg->mm_base;
	mm_region.end = drv_cfg->mm_base + drv_cfg->mm_size;
	if (drv_cfg->mm_size != 0U) {
		mm_region.end--;
	}

	ospi_region[0U].start = drv_cfg->mm_ospi1_base;
	ospi_region[0U].end = drv_cfg->mm_ospi1_base + drv_cfg->mm_ospi1_size;
	if (drv_cfg->mm_ospi1_size != 0U) {
		ospi_region[0U].end--;
	}

	ospi_region[1U].start = drv_cfg->mm_ospi2_base;
	ospi_region[1U].end = drv_cfg->mm_ospi2_base + drv_cfg->mm_ospi2_size;
	if (drv_cfg->mm_ospi2_size != 0U) {
		ospi_region[1U].end--;
	}

	for (ospi_i = 0U; ospi_i < OSPI_NB; ospi_i++) {
		if (ospi_region[ospi_i].start >= ospi_region[ospi_i].end) {
			continue;
		}

		if (!stm32_omm_region_contains(&mm_region, &ospi_region[ospi_i])) {
			return -EINVAL;
		}

		valid_regions++;
	}

	if ((valid_regions == OSPI_NB) &&
	    stm32_omm_region_overlaps(&ospi_region[0U], &ospi_region[1U])) {
		return -EINVAL;
	}

	if (valid_regions != 0U) {
		return stm32_omm_set_amcr(dev);
	}

	return 0;
}

static int stm32_omm_configure(const struct device *dev)
{
	const struct stm32_omm_cfg *drv_cfg = dev_get_config(dev);
	struct clk *clk;
	unsigned int ospi_i;
	unsigned long clk_rate_max = 0U;
	uint32_t cssel_ovr = 0U;
	uint32_t req2ack = 0U;
	int ret;

	for (ospi_i = 0U; ospi_i < OSPI_NB; ospi_i++) {
		const struct stm32_ospi_cfg *ospi_cfg = &drv_cfg->ospi_cfg[ospi_i];

		clk = clk_get(ospi_cfg->clk_dev, ospi_cfg->clk_subsys);
		clk_rate_max = MAX(clk_get_rate(clk), clk_rate_max);

		ret = clk_enable(clk);
		if (ret != 0) {
			return ret;
		}

		ret = reset_control_reset(&ospi_cfg->rst_ctl);
		if (ret != 0) {
			clk_disable(clk);
			return ret;
		}

		ret = reset_control_reset(&ospi_cfg->rst_ctl_dll);
		if (ret != 0) {
			clk_disable(clk);
			return ret;
		}

		clk_disable(clk);
	}

	if (((drv_cfg->mux & _OCTOSPIM_CR_MUXEN) == _OCTOSPIM_CR_MUXEN) &&
	    (drv_cfg->req2ack != 0U)) {
		unsigned long hclkn = NSEC_PER_SEC / clk_rate_max;
		unsigned long timing = div_round_up(drv_cfg->req2ack, hclkn) - 1U;

		if (timing >
		    _OCTOSPIM_CR_REQ2ACK_MASK >> _OCTOSPIM_CR_REQ2ACK_SHIFT) {
			req2ack = _OCTOSPIM_CR_REQ2ACK_MASK;
		} else {
			req2ack = timing << _OCTOSPIM_CR_REQ2ACK_SHIFT;
		}
	}

	if (drv_cfg->cssel_ovr != 0xfU) {
		cssel_ovr = (drv_cfg->cssel_ovr << _OCTOSPIM_CR_CSSEL_OVR_SHIFT) |
			    _OCTOSPIM_CR_CSSEL_OVR_EN;
	}

	clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	ret = clk_enable(clk);
	if (ret != 0) {
		return ret;
	}

	mmio_clrsetbits_32(drv_cfg->base + _OCTOSPIM_CR,
			   _OCTOSPIM_CR_REQ2ACK_MASK |
			   _OCTOSPIM_CR_CSSEL_OVR_MASK |
			   _OCTOSPIM_CR_MUXEN_MODE_MASK,
			   req2ack | cssel_ovr | drv_cfg->mux);

	clk_disable(clk);

	if ((drv_cfg->mux & _OCTOSPIM_CR_MUXEN) == _OCTOSPIM_CR_MUXEN) {
		/* If the mux is enabled, the 2 OSPI clocks have to be always enabled */
		for (ospi_i = 0U; ospi_i < OSPI_NB; ospi_i++) {
			const struct stm32_ospi_cfg *ospi_cfg = &drv_cfg->ospi_cfg[ospi_i];

			clk = clk_get(ospi_cfg->clk_dev, ospi_cfg->clk_subsys);

			ret = clk_enable(clk);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

int stm32_omm_init(const struct device *dev)
{
	const struct stm32_omm_cfg *drv_cfg = dev_get_config(dev);
	int ret;

	ret = stm32_omm_set_mm(dev);
	if (ret != 0) {
		return ret;
	}

	ret = stm32_omm_configure(dev);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if ((ret != 0) && (ret != -ENOENT)) {
		return ret;
	}

	return stm32_rifprot_set_config(drv_cfg->rif_ctrl,
					drv_cfg->rif_cfg, drv_cfg->n_rif_cfg);
}

#define DT_GET_MM_BASE_BY_NAME_OR(n, name)							\
	COND_CODE_1(DT_INST_PROP_HAS_NAME(n, memory_region, name),				\
		    (DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, memory_region, name))), (0U))

#define DT_GET_MM_SIZE_BY_NAME_OR(n, name)							\
	COND_CODE_1(DT_INST_PROP_HAS_NAME(n, memory_region, name),				\
		    (DT_REG_SIZE(DT_INST_PHANDLE_BY_NAME(n, memory_region, name))), (0U))

#define STM32_OSPI_CHILD_DEFINE(n)								\
	{											\
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(n)),					\
		.clk_subsys = (clk_subsys_t) DT_CLOCKS_CELL(n, bits),				\
		.rst_ctl = DT_RESET_CONTROL_GET_BY_IDX(n, 0),					\
		.rst_ctl_dll = DT_RESET_CONTROL_GET_BY_IDX(n, 1),				\
	},

#define STM32_OMM_INIT(n)									\
												\
PINCTRL_DT_INST_DEFINE(n);									\
												\
static const struct stm32_ospi_cfg stm32_ospi_cfg_##n[] = {					\
	DT_INST_FOREACH_CHILD(n, STM32_OSPI_CHILD_DEFINE)					\
};												\
												\
DT_RIFPROT_CTRL_EXTERN(DT_NODELABEL(rifsc));							\
DT_INST_RIFPROT_CONFIG_DEFINE(n);								\
												\
static const struct stm32_omm_cfg stm32_omm_cfg_##n = {						\
	.base = DT_INST_REG_ADDR_BY_NAME(n, omm),						\
	.mm_base = DT_INST_REG_ADDR_BY_NAME(n, omm_mm),						\
	.mm_size = DT_INST_REG_SIZE_BY_NAME(n, omm_mm),						\
	.mm_ospi1_base = DT_GET_MM_BASE_BY_NAME_OR(n, mm_ospi1),				\
	.mm_ospi1_size = DT_GET_MM_SIZE_BY_NAME_OR(n, mm_ospi1),				\
	.mm_ospi2_base = DT_GET_MM_BASE_BY_NAME_OR(n, mm_ospi2),				\
	.mm_ospi2_size = DT_GET_MM_SIZE_BY_NAME_OR(n, mm_ospi2),				\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),					\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),				\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),						\
	.rif_ctrl = DT_RIFPROT_CTRL_GET(DT_NODELABEL(rifsc)),					\
	.rif_cfg = DT_INST_RIFPROT_CONFIG_GET(n),						\
	.n_rif_cfg = ARRAY_SIZE(DT_INST_RIFPROT_CONFIG_GET(n)),					\
	.amcr_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, st_syscfg_amcr)),				\
	.amcr_base = DT_INST_PHA(n, st_syscfg_amcr, offset),					\
	.amcr_mask = DT_INST_PHA(n, st_syscfg_amcr, mask),					\
	.mux = DT_INST_PROP_OR(n, st_omm_mux, 0),						\
	.req2ack = DT_INST_PROP_OR(n, st_omm_req2ack_ns, 0),					\
	.cssel_ovr = DT_INST_PROP_OR(n, st_omm_cssel_ovr, 0xf),					\
	.ospi_cfg = stm32_ospi_cfg_##n,								\
	.n_ospi_cfg = ARRAY_SIZE(stm32_ospi_cfg_##n),						\
};												\
												\
DEVICE_DT_INST_DEFINE(n, &stm32_omm_init,							\
		      NULL, &stm32_omm_cfg_##n,							\
		      CORE, 11, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_OMM_INIT)
