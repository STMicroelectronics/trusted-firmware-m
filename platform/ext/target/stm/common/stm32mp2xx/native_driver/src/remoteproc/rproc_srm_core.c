// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 *
 */
#define DT_DRV_COMPAT rproc_srm_core
#include <device.h>
#include <devicetree.h>
#include <lib/utils_def.h>

#include <clk.h>
#include <cmsis.h>
#include <debug.h>
#include <errno.h>
#include <firewall.h>
#include <inttypes.h>
#include <regulator.h>
#include <reset.h>
#include <rproc_srm_core.h>
#include <stdint.h>
#include <stdbool.h>

struct clock_control {
	const struct device *dev;
	const clk_subsys_t subsys;
};

/* periph config */
struct periph_cfg {
	const char *name;
	const struct clock_control *clk_ctl;
	int n_clk;
	const struct reset_control *rst_ctl;
	int n_rst;
	const struct device *vin_supply;
	const struct firewall_spec *firewall_ctrls;
	const int n_firewall_ctrls;
};

struct rproc_srm_config {
	/* peripheral configs */
	const struct periph_cfg **cfg;
	/* number of peripheral configs */
	int n_cfg;
};

static __unused int periph_reset(const struct periph_cfg *cfg)
{
	const struct firewall_spec *firewall;
	const struct reset_control *rst_ctl;
	const struct clock_control *clk_ctl;
	const struct device *dev = NULL;
	struct clk *clk;
	int i, err;

	/* Check if regulator enabled if not enabled reset not required */
	dev = cfg->vin_supply;
	if ((dev) && (!regulator_is_enabled(dev))) {
		return 0;
	}

	/* Set rif config for ca35 peripheral on m33td side */
	err = set_for_each_firewall(cfg->firewall_ctrls, firewall, cfg->n_firewall_ctrls, i);
	if (err != 0) {
		DMSG("[%s] fail to set firewall [%d} %s\n", __func__, i,
		     firewall->dev->name);
		return err;
	}

	/* Enable peripheral clock */
	for (i = 0, clk_ctl = cfg->clk_ctl; i < cfg->n_clk; i++, clk_ctl++) {
		clk = clk_get(clk_ctl->dev, clk_ctl->subsys);
		if (clk)
			clk_enable(clk);
		else
			EMSG("[%s] fail to get clock ctrl [%d] %s subsys %d\n", __func__, i,
			     clk_ctl->dev->name, clk_ctl->subsys);
	}

	/* Reset peripheral */
	for (i = 0, rst_ctl = cfg->rst_ctl; i < cfg->n_rst; i++, rst_ctl++) {
		err = reset_control_reset(rst_ctl);
		if (err) {
			EMSG("[%s] fail to reset [%d] %s %d\n", __func__, i,
			     rst_ctl->dev->name, rst_ctl->id);

			return err;
		}
	}

	/* Disable peripheral clock */
	for (i = 0, clk_ctl = cfg->clk_ctl; i < cfg->n_clk; i++, clk_ctl++) {
		clk = clk_get(clk_ctl->dev, clk_ctl->subsys);
		if (clk)
			clk_disable(clk);
		else
			EMSG("[%s] fail to get clock ctrl [%d] %s subsys %d\n", __func__, i,
			     clk_ctl->dev->name, clk_ctl->subsys);

	}

	/* Restore rif config for ca35 peripheral */
	err = release_for_each_firewall(cfg->firewall_ctrls, firewall, cfg->n_firewall_ctrls, i);
	if (err != 0) {
		DMSG("[%s] fail to release firewall [%d} %s\n", __func__, i,
		     firewall->dev->name);
		return err;
	}

	return err;
}

__unused int rproc_srm_reset(const struct device *dev)
{
	const struct rproc_srm_config *cfg = dev_get_config(dev);
	int i, err = 0;

	for (i = 0; i < cfg->n_cfg; i++) {
		err = periph_reset(cfg->cfg[i]);
		if (err) {
			DMSG("[%s] fail to reset cfg [%d] %s\n", __func__, i, cfg->cfg[i]->name);
			return err;
		}
	}

	return 0;
}

#define DT_CLOCK_CONTROL_GET_BY_IDX(node_id, idx)					\
	{										\
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),		\
		.subsys = (clk_subsys_t)DT_CLOCKS_CELL_BY_IDX(node_id, idx, bits)	\
	}

#define _DT_CLOCK_CTL(clk_id, node_id) DT_CLOCK_CONTROL_GET_BY_IDX(node_id, clk_id)

#define DT_CLOCK_CONTROL(node_id)						\
									{	\
		LISTIFY(DT_NUM_CLOCKS(node_id), _DT_CLOCK_CTL, (, ), node_id)	\
	}

#define DT_CHILD_NAME_STRING(node_id)\
	DT_FOREACH_NODELABEL(node_id, DT_NODELABEL_STRING_ARRAY_ENTRY_INTERNAL)

#define PARENT_NODE(n) DT_DRV_INST(n)

#define PERIPH_CHILD_CFG_N(n, child)							\
	DT_ACCESS_CTRLS_DEFINE(child);							\
	COND_CODE_1(DT_NODE_HAS_PROP(child, clocks),					\
	(										\
		static const struct clock_control clk_ctrl_##n##_##child[] =		\
			DT_CLOCK_CONTROL(child);					\
	),										\
	(/* no clocks */))								\
	COND_CODE_1(DT_NODE_HAS_PROP(child, resets),					\
	(										\
		static const struct reset_control periph_rst_ctrl_##n##_##child[] =	\
			DT_RESETS_CONTROL(child);					\
	),										\
	(/* no resets */))								\
	static const struct periph_cfg periph_cfg_##n##_##child = {			\
		.name = DT_CHILD_NAME_STRING(child)					\
		.firewall_ctrls   = DT_ACCESS_CTRLS_GET(child),				\
		.n_firewall_ctrls = DT_ACCESS_CTRLS_NUM(child),				\
		.clk_ctl = COND_CODE_1(DT_NODE_HAS_PROP(child, clocks),			\
			(clk_ctrl_##n##_##child),					\
			(NULL)),							\
		.n_clk   = COND_CODE_1(DT_NODE_HAS_PROP(child, clocks),			\
			(DT_NUM_CLOCKS(child)),						\
			(0)),								\
		.rst_ctl = COND_CODE_1(DT_NODE_HAS_PROP(child, resets),			\
			(periph_rst_ctrl_##n##_##child),				\
			(NULL)),							\
		.n_rst   = COND_CODE_1(DT_NODE_HAS_PROP(child, resets),			\
			(DT_NUM_RESETS(child)),						\
			(0)),								\
		.vin_supply = COND_CODE_1(DT_NODE_HAS_PROP(child, vin_supply),		\
			DT_DEV_REGULATOR_SUPPLY(child, vin),				\
			(NULL)),							\
	}

#define PERIPH_CHILD_CFG(child) PERIPH_CHILD_CFG_N(n, child);

#define STM32_PERIPH_CFGS_DECLARE(n)					\
	DT_FOREACH_CHILD_STATUS_OKAY(PARENT_NODE(n),			\
				     PERIPH_CHILD_CFG)

#define PERIPH_CHILD_ARRAY_ELEM(child) &periph_cfg_##n##_##child,

#define STM32_PERIPH_CFGS_ARRAY(n)					\
	static const struct periph_cfg *periph_cfg_##n[] = {		\
		DT_FOREACH_CHILD_STATUS_OKAY(PARENT_NODE(n),		\
					     PERIPH_CHILD_ARRAY_ELEM)	\
	}

#define RPROC_SRM_CORE_INIT(n, name)					\
									\
	DT_INST_ACCESS_CTRLS_DEFINE(n);					\
									\
	STM32_PERIPH_CFGS_DECLARE(n);					\
	STM32_PERIPH_CFGS_ARRAY(n);					\
									\
	static const struct rproc_srm_config _##name##_cfg##n = {	\
		.cfg  = periph_cfg_##n,					\
		.n_cfg = ARRAY_SIZE(periph_cfg_##n),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,				\
			      NULL, &_##name##_cfg##n,			\
			      CORE, 9, NULL);

DT_INST_FOREACH_STATUS_OKAY_VARGS(RPROC_SRM_CORE_INIT, DT_DRV_COMPAT)
