/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */

#define DT_DRV_COMPAT st_stm32mp2_scmi

#include <device.h>
#include "psa/service.h"
#include <stdint.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <debug.h>
#include <spi_mem.h>
#include <clk.h>
#include <pinctrl.h>
#include <reset.h>
#include <syscon.h>
#include <dt-bindings/scmi/stm32mp25-agents.h>
#include "dt-bindings/clock/stm32mp25-clks.h"
#include <stdlib.h>
#include "clk-stm32-core.h"
#include "tfm_sp_log.h"
#include "tfm_scmi.h"
#include <assert.h>
#include <scmi_agent_configuration.h>
/*  include memory layout for scmi tfm smt memory */
#include <region_defs.h>
/*
 * struct stm32_scmi_clk - Data for the exposed clock
 * @change_rate: SCMMI agent is allowed to change the rate
 */
struct stm32_scmi_clk {
	bool change_rate;
};
/*
 * struct stm32_scmi_clkd - Data for the exposed clock controller
 * @scmi_id: scmi id for voltage service
 * @clk_dev: clock controller manipulated by the SCMI channel
 * @clk_subsys; rcc id for the clock mainupation
 */
struct stm32_scmi_clkd {
	unsigned long scmi_id;
	const char *name;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	bool rate;
};

/*
 * struct stm32_scmi_regud - Data for the exposed voltage contreoller
 * @scmi_id: scmi id for voltage service
 * @regu_dev: regu controller manipulated by the SCMI channel
 */
struct stm32_scmi_regud {
	unsigned long scmi_id;
	const char *name;
	const struct device *regu_dev;
};

/*
 * struct stm32_scmi_rd - Data for the exposed reset controller
 * @scmi_id: scmi id for reset service
 * @name: Reset string ID exposed to channel
 * @rstctrl: Reset controller manipulated by the SCMI channel
 */
struct stm32_scmi_rd {
	unsigned long scmi_id;
	const char *name;
	const struct reset_control rstctrl[1];
};

/*
 * struct stm32_scmi_pd - Data for the exposed power domains
 * @scmi_id: scmi id for power domain service
 * @name: Power domain name exposed to the channel
 * @clk_dev: clock controller manipulated by the SCMI channel
 * @clk_subsys: rcc id for the clock mainupation
 * @regu_dev: regu controller manipulated by the SCMI channel
 */
struct stm32_scmi_pd {
	unsigned long scmi_id;
	const char *name;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct device *regu_dev;
};

/*
 * Platform clocks exposed with SCMI
 */
static int plat_scmi_clk_get_rates_steps(struct clk *clk,
						unsigned long *min,
						unsigned long *max,
						unsigned long *step)
{
	struct stm32_scmi_clk *scmi_clk = clk->priv;

	if (scmi_clk->change_rate) {
		*min = 0;
		*max = UINT32_MAX;
		*step = 0;
	} else {
		*min = clk->rate;
		*max = *min;
		*step = 0;
	}

	return TFM_SCMI_SUCCESS;
}

static const struct clk_ops plat_scmi_clk_ops = {
  .get_rates_steps = plat_scmi_clk_get_rates_steps,
};
struct stm32_scmi_config {
	const int dt_agent_id;
	const char *dt_agent_name;
	const struct stm32_scmi_rd *dt_resets;
	const int ndt_resets;
	const int ndt_resets_max;
	const struct stm32_scmi_clkd *dt_clocks;
	const int ndt_clocks;
	const int ndt_clocks_max;
	const struct stm32_scmi_regud *dt_regus;
	const int ndt_regus;
	const int ndt_regus_max;
	const struct stm32_scmi_pd *dt_pd;
	const int ndt_pd;
	const int ndt_pd_max;
};

#define AGENT_NUM DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

static const struct stm32_scmi_config *scmi_cfg[AGENT_NUM] = {0};
static struct clk *plat_clocks[AGENT_NUM] = {0};
static int stm32_scmi_init(const struct device *dev)
{
	int agent_count = ((struct stm32_scmi_config *)
			   dev_get_config(dev))->dt_agent_id -1;
	assert(agent_count <  ARRAY_SIZE(plat_clocks));
	assert(agent_count <  ARRAY_SIZE(scmi_cfg));
	scmi_cfg[agent_count] = dev_get_config(dev);
	plat_clocks[agent_count] = dev_get_data(dev);
	return 0;
}

#define SCMI_DT_ID( _node_id, _prop, _idx)					\
	DT_PROP(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), scmi_id)

#define SCMI_DT_RESET_NAME(_node_id, _prop, _idx)				\
	DT_PROP(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), reset_names)

#define DT_RESET_CONTROL_SINGLE(node_id)					\
{ LISTIFY(1, _DT_RCTL,  , node_id) }

#define SCMI_DT_RESET_CTRL(_node_id, _prop, _idx)				\
	 DT_RESET_CONTROL_SINGLE(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx))

#define SCMI_DT_CLOCK(_node_id, _prop, _idx)					\
	DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx)))

#define SCMI_DT_NAME(_node_id, _prop, _idx)					\
	DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx))

#define SCMI_DT_CLOCK_RATE(_node_id, _prop, _idx)				\
	DT_PROP(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), rate)

#define SCMI_DT_CLOCK_SUB(_node_id, _prop, _idx)				\
	(clk_subsys_t) DT_CLOCKS_CELL(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), bits)

#define SCMI_DT_REGU(_node_id, _prop, _idx)					\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), regu, 0))

#define RST_ELE(_node_id, _prop, _idx, _n)					\
	{									\
		.scmi_id = SCMI_DT_ID(_node_id, _prop, _idx),			\
		.rstctrl = SCMI_DT_RESET_CTRL(_node_id, _prop, _idx),		\
		.name = SCMI_DT_NAME(_node_id, _prop, _idx),			\
	},

#define CLK_ELE(_node_id, _prop, _idx, _n)					\
	{									\
		.scmi_id = SCMI_DT_ID(_node_id, _prop, _idx),			\
		.name = SCMI_DT_NAME(_node_id, _prop, _idx),			\
		.clk_subsys = SCMI_DT_CLOCK_SUB(_node_id, _prop, _idx),		\
		.clk_dev = SCMI_DT_CLOCK(_node_id, _prop, _idx),		\
		.rate = SCMI_DT_CLOCK_RATE(_node_id, _prop, _idx)		\
	},

#define REGU_ELE(_node_id, _prop, _idx, _n)					\
	{									\
		.scmi_id = SCMI_DT_ID(_node_id, _prop, _idx),			\
		.regu_dev = SCMI_DT_REGU(_node_id, _prop, _idx)			\
	},

#define PD_ELE(_node_id, _prop, _idx, _n)					\
	{									\
		.scmi_id = SCMI_DT_ID(_node_id, _prop, _idx),			\
		.name = SCMI_DT_NAME(_node_id, _prop, _idx),			\
		.clk_subsys = SCMI_DT_CLOCK_SUB(_node_id, _prop, _idx),		\
		.clk_dev = SCMI_DT_CLOCK(_node_id, _prop, _idx),		\
	        .regu_dev = SCMI_DT_REGU(_node_id, _prop, _idx)			\
	},

#define ZERO_ELE(_node_id, _prop, _idx, _n) { 0 },

#define STM32_SCMI_INIT(n)							\
static const struct stm32_scmi_rd scmi_dt_resets_##n[] = {			\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, rst_list, RST_ELE, (), n)	\
};										\
static const struct stm32_scmi_clkd scmi_dt_clocks_##n[] = {			\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, clk_list, CLK_ELE, (), n)	\
};										\
static const struct stm32_scmi_regud scmi_dt_regus_##n[] = {			\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, regu_list, REGU_ELE, (), n)	\
};										\
static const struct stm32_scmi_pd scmi_dt_pd_##n[] = {				\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(0, pd_list, PD_ELE, (), n)		\
};										\
static struct clk plat_clk_##n[] = {						\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, clk_list, ZERO_ELE, (), n)	\
};										\
										\
static const struct stm32_scmi_config stm32_scmi_cfg_##n = {			\
	.dt_agent_id = DT_INST_PROP(n, agent_id),				\
	.dt_agent_name =  DT_INST_PROP(n, agent_name),				\
	.dt_resets = scmi_dt_resets_##n,					\
	.ndt_resets = ARRAY_SIZE(scmi_dt_resets_##n),				\
	.ndt_resets_max = DT_INST_PROP(n, rst_id_max),				\
	.dt_clocks = scmi_dt_clocks_##n,					\
	.ndt_clocks = ARRAY_SIZE(scmi_dt_clocks_##n),				\
	.ndt_clocks_max =  DT_INST_PROP(n, clk_id_max),				\
	.dt_regus  = scmi_dt_regus_##n,						\
	.ndt_regus = ARRAY_SIZE(scmi_dt_regus_##n),				\
	.ndt_regus_max = DT_INST_PROP(n, regu_id_max),				\
	.dt_pd = scmi_dt_pd_##n,						\
	.ndt_pd = ARRAY_SIZE(scmi_dt_pd_##n),					\
	.ndt_pd_max =  DT_PROP(DT_DRV_INST(n), pd_id_max),			\
};										\
DEVICE_DT_INST_DEFINE(n ,&stm32_scmi_init,					\
		      &plat_clk_##n[0],						\
		      &stm32_scmi_cfg_##n,					\
		      CORE, 30,							\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_SCMI_INIT)

/*
 * Build the structure used to initialize SCP firmware.
 * We target a description from the DT so SCP firmware configuration
 * but we currently rely on local data (clock, reset) and 2 DT node
 * (voltage domain/regulators and DVFS).
 *
 * At early_init initcall level, prepare scpfw_cfg agent and channel parts.
 * At driver_init_late initcall level, get clock and reset devices.
 * When scmi-regulator-consumer driver probes, it adds the regulators.
 * When CPU OPP driver probesn it adds the DVFS part.
 */
static struct scpfw_config scpfw_cfg;

struct scpfw_config *scmi_scpfw_get_configuration(void)
{
	assert(scpfw_cfg.agent_count);

	return &scpfw_cfg;
}
static const char dummy[]="dummy";

int32_t scmi_scpfw_cfg_early_init(void)
{
	int i=0;
	int j=0;
	scpfw_cfg.agent_count = AGENT_NUM;
	scpfw_cfg.agent_config = calloc(AGENT_NUM, sizeof(*scpfw_cfg.agent_config));
	struct scpfw_channel_config *channel_cfg = NULL;

	for(i = 0; i < AGENT_NUM; i++) {
		/*  agent id 0 is uncorrect */
		int index = scmi_cfg[i]->dt_agent_id-1;
		assert(index < AGENT_NUM);
		scpfw_cfg.agent_config[index].name = scmi_cfg[i]->dt_agent_name;
		scpfw_cfg.agent_config[index].agent_id = scmi_cfg[i]->dt_agent_id;
		scpfw_cfg.agent_config[index].channel_count = 1;
		channel_cfg = calloc(scpfw_cfg.agent_config[index].channel_count,
				     sizeof(*scpfw_cfg.agent_config[index].channel_config));
		channel_cfg->shm.area = (uintptr_t *)S_SCMI_ADDR;
		channel_cfg->shm.size = S_SCMI_SIZE;

		scpfw_cfg.agent_config[index].channel_config = channel_cfg;
		channel_cfg->name = "channel";
		channel_cfg->clock_count = scmi_cfg[i]->ndt_clocks_max;
		channel_cfg->reset_count = scmi_cfg[i]->ndt_resets_max;
		channel_cfg->voltd_count = scmi_cfg[i]->ndt_regus_max;
		channel_cfg->pd_count = scmi_cfg[i]->ndt_pd_max;

		channel_cfg->clock =
			calloc(scpfw_cfg.agent_config[index].channel_config->clock_count,
			       sizeof(*scpfw_cfg.agent_config[index].channel_config->clock));
		channel_cfg->reset =
			calloc(scpfw_cfg.agent_config[index].channel_config->reset_count,
			       sizeof(*scpfw_cfg.agent_config[index].channel_config->reset));
		channel_cfg->voltd =
			calloc(scpfw_cfg.agent_config[index].channel_config->voltd_count,
			       sizeof(*scpfw_cfg.agent_config[index].channel_config->voltd));
		channel_cfg->pd =
			calloc(scpfw_cfg.agent_config[index].channel_config->pd_count,
			       sizeof(*scpfw_cfg.agent_config[index].channel_config->pd));

		/* initialize with dummy name */
		for(j = 0; j < channel_cfg->clock_count; j++)
			channel_cfg->clock[j] =
				(struct scmi_clock) {
					.name  = dummy,
				};

		for(j = 0; j < channel_cfg->reset_count; j++)
			channel_cfg->reset[j] =
				(struct scmi_reset) {
					.name  = dummy,
				};

		for(j = 0; j < channel_cfg->voltd_count; j++)
			channel_cfg->voltd[j] =
				(struct scmi_voltd) {
					.name  = dummy,
				};

		for(j = 0; j < channel_cfg->pd_count; j++)
			channel_cfg->pd[j] =
				(struct scmi_pd) {
					.name  = dummy,
				};
	}

	return TFM_SCMI_SUCCESS;
}

static const struct stm32_scmi_clk rate = {.change_rate = true };
static const struct stm32_scmi_clk no_rate = {.change_rate = false };

static int32_t scmi_scpfw_cfg_init_agent(const struct stm32_scmi_config *agent)
{
	struct scpfw_channel_config *channel_cfg = NULL;
	size_t j = 0;
	struct scmi_clock *scmi_clk;
	struct clk * clk;

	/* Clock and reset are exposed to agent#0/channel#0 */
	channel_cfg = scpfw_cfg.agent_config[agent->dt_agent_id-1].channel_config;
	assert(channel_cfg);
	if (channel_cfg->clock_count) {
		for (j = 0; j < agent->ndt_clocks; j++) {
			/*  Retrieve clk device with j indice*/
			clk = plat_clocks[agent->dt_agent_id-1]+j;
			/*  Retrieve scmi_clk according to scmi id, clk are
			 *  ordered in table according to scmi id  */
			assert(agent->dt_clocks[j].scmi_id < agent->ndt_clocks_max);

			scmi_clk = channel_cfg->clock +
				agent->dt_clocks[j].scmi_id;
			clk->parent = clk_get(agent->dt_clocks[j].clk_dev,
					      agent->dt_clocks[j].clk_subsys);

			if (!clk->parent)
			{
				LOG_INFFMT("\r\nget scmi clock  %d : no parent\r\n",
				       agent->dt_clocks[j].scmi_id);
				continue;
			}
			clk->name = agent->dt_clocks[j].name;
			clk->ops = &plat_scmi_clk_ops;
			clk->priv = (struct scmi_clock *)&no_rate;
			/*  dt option to add */
			if (agent->dt_clocks[j].rate) {
				clk->flags = CLK_SET_RATE_PARENT;
				clk->priv = (struct scmi_clock *)&rate;
			}

			clk->flags |= CLK_DUTY_CYCLE_PARENT;

			if (clk_register(clk))
				psa_panic();

			scmi_clk->clk = clk;
			scmi_clk->enabled = false;
			scmi_clk->name = clk->name;
		}
	}

	if (channel_cfg->reset_count) {
		/*  re-order reset within table according to scmi id  */
		for (j = 0; j <agent->ndt_resets; j++) {
			assert(agent->dt_resets[j].scmi_id < agent->ndt_resets_max);
			channel_cfg->reset[agent->dt_resets[j].scmi_id] =
				(struct scmi_reset) {
					.name = agent->dt_resets[j].name,
					.rstctrl = &agent->dt_resets[j].rstctrl[0],
				};
		}
	}

	if (channel_cfg->voltd_count) {
		/*  re-order regu within table according to scmi id  */
		for (j = 0; j <agent->ndt_regus; j++) {
			const struct device *dev = agent->dt_regus[j].regu_dev;
			bool enabled = false;
			assert(agent->dt_regus[j].scmi_id < agent->ndt_regus_max);
			if (regulator_enable(dev))
				LOG_INFFMT("\r\nFailed to enable SCMI regul %d\r\n",agent->dt_regus[j].scmi_id);
			else
				enabled = true;
			if (!agent->dt_regus[j].regu_dev->name)
				psa_panic();

			channel_cfg->voltd[agent->dt_regus[j].scmi_id] =
				(struct scmi_voltd ){
					.name = agent->dt_regus[j].regu_dev->name,
					.dev = agent->dt_regus[j].regu_dev,
					.enabled = enabled,
				};
		}
	}

	if (channel_cfg->pd_count) {

		/*  re-order pd within table according to scmi id  */
		for (j = 0; j < agent->ndt_pd; j++) {
			clk = clk_get(agent->dt_pd[j].clk_dev,
				      agent->dt_pd[j].clk_subsys);
			assert(agent->pd[j].scmi_id < agent->ndt_pd_max);
			channel_cfg->pd[agent->dt_pd[j].scmi_id] =
				(struct scmi_pd ){
					.name = agent->dt_pd[j].name,
					.regu = agent->dt_pd[j].regu_dev,
					.clk = clk,
				};
		}
	}

	return TFM_SCMI_SUCCESS;
}

int32_t scmi_scpfw_cfg_init(void)
{
	size_t i = 0;
	int32_t ret;

	for (i = 0; i < AGENT_NUM; i++) {
		ret =  scmi_scpfw_cfg_init_agent(scmi_cfg[i]);
		if (ret) return ret;
	}

	return TFM_SCMI_SUCCESS;
}

void scmi_scpfw_release_configuration(void)
{
	struct scpfw_channel_config *channel_cfg = NULL;
	struct scpfw_agent_config *agent_cfg = NULL;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;

	for (i = 0; i < scpfw_cfg.agent_count; i++) {
		agent_cfg = scpfw_cfg.agent_config + i;

		for (j = 0; j < agent_cfg->channel_count; j++) {
			channel_cfg = agent_cfg->channel_config + j;

			free(channel_cfg->clock);
			free(channel_cfg->reset);
			free(channel_cfg->voltd);
			free(channel_cfg->pd);
		}

		free(agent_cfg->channel_config);
	}

	free(scpfw_cfg.agent_config);
}
