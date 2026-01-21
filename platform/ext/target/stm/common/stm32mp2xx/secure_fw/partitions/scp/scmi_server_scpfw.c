/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */

#define DT_DRV_COMPAT st_stm32mp2_scmi

#include <device.h>
#include <devicetree.h>
#include "psa/service.h"
#include <stdint.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <mbox.h>
#include <debug.h>
#include <spi_mem.h>
#include <clk.h>
#include <pinctrl.h>
#include <reset.h>
#include <firewall.h>
#include <syscon.h>
#include <dt-bindings/scmi/stm32mp2-agents.h>
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
	const struct firewall_spec *firewall;
	const int n_firewall;
};

/*
 * Platform clocks exposed with SCMI
 */
static int plat_scmi_clk_get_rates_array(struct clk *clk, size_t index,
						unsigned long *rates,
						size_t *nb_elts)
{
	uint32_t plat_clock_flags = *(uint32_t *)clk->priv;

	if (!nb_elts)
		return -EINVAL;

	if (plat_clock_flags)
		return clk_get_rates_array(clk->parent, index, rates, nb_elts);

	if (!rates || !*nb_elts) {
		*nb_elts = 1;
		return 0;
	}

	if (index)
		return -EINVAL;

	assert(rates);

	/*
	 * Clocks not exposed have no effective parent/platform clock.
	 * Report a 0 Hz rate in this case.
	 */
	if (clk->parent)
		*rates = clk_get_rate(clk->parent);
	else
		*rates = 0;

	*nb_elts = 1;

	return 0;
}

static int plat_scmi_clk_get_rates_steps(struct clk *clk,
						unsigned long *min,
						unsigned long *max,
						unsigned long *step)
{
	uint32_t plat_clock_flags = *(uint32_t *)clk->priv;

	if (plat_clock_flags) {
		*min = 0;
		*max = UINT32_MAX;
		*step = 0;
	} else {
		*min = clk->rate;
		*max = *min;
		*step = 0;
	}

	return 0;
}

static const struct clk_ops plat_scmi_clk_ops = {
	.get_rates_array = plat_scmi_clk_get_rates_array,
	.get_rates_steps = plat_scmi_clk_get_rates_steps,
};
struct stm32_scmi_config {
	const int dt_agent_id;
	const char *dt_agent_name;
	const struct shared_mem *dt_shm;
	const struct mbox_dt_spec *dt_chan;
	const struct stm32_scmi_rd *dt_resets;
	const int ndt_resets;
	const int ndt_resets_max;
	const struct stm32_scmi_clkd *dt_clocks;
	const int ndt_clocks;
	const int ndt_clocks_max;
	const struct stm32_scmi_regud *dt_regus;
	const int ndt_regus;
	const int ndt_regus_max;
	const struct stm32_scmi_pd **dt_pd;
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

#define ZERO_ELE(_node_id, _prop, _idx, _n) { 0 },

/* Create all scmi pd handler */
#define _PD_PH_GET(_node_id) \
	&_PD_PH_NAME(_node_id)

#define _PD_PH_NAME(_node_id) \
	_CONCAT(_scmi_dt_pd_, DEVICE_DT_NAME_GET(_node_id))

#define STM32_SCMI_PD_INIT(n)								\
	DT_INST_ACCESS_CTRLS_DEFINE(n);							\
	static const struct stm32_scmi_pd _PD_PH_NAME(DT_DRV_INST(n)) =			\
	{										\
		.scmi_id = DT_INST_PROP(n, scmi_id),					\
		.name = DT_NODE_FULL_NAME(DT_DRV_INST(n)),				\
		.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),		\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
	        .regu_dev = DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(n, regu, 0)),		\
		.firewall = DT_INST_ACCESS_CTRLS_GET(n),				\
		.n_firewall = DT_INST_ACCESS_CTRLS_NUM(n),			\
	};

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp25_scmi_pd

DT_INST_FOREACH_STATUS_OKAY(STM32_SCMI_PD_INIT)

/* create all agents */
#define _PD_ELEM(_node_id, _prop, _idx, _n)					\
	_PD_PH_GET(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx))

#define _DT_INST_PD_LIST_NUM(n) DT_INST_PROP_LEN_OR(n, pd_list, 0)
#define _DT_INST_CLK_LIST_NUM(n) DT_INST_PROP_LEN_OR(n, clk_list, 0)
#define _DT_INST_RST_LIST_NUM(n) DT_INST_PROP_LEN_OR(n, rst_list, 0)


#define STM32_SCMI_INIT(n)							\
static const struct stm32_scmi_rd scmi_dt_resets_##n[] = {			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, rst_list),				\
	(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, rst_list, RST_ELE, (), n)),	\
	())									\
};										\
static const struct stm32_scmi_clkd scmi_dt_clocks_##n[] = {			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clk_list),				\
	(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, clk_list, CLK_ELE, (), n)),	\
	())									\
};										\
static const struct stm32_scmi_regud scmi_dt_regus_##n[] = {                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, regu_list),			\
	(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, regu_list, REGU_ELE, (), n)),	\
	())									\
};										\
static struct clk plat_clk_##n[] = {						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clk_list),				\
		(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, clk_list,		\
						    ZERO_ELE, (), n)),		\
		())								\
};										\
										\
static const struct stm32_scmi_pd *scmi_dt_pd_##n[] = {				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pd_list),				\
		(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, pd_list,		\
						     _PD_ELEM, (,), n)),	\
		())								\
};										\
										\
static const struct shared_mem scmi_dt_shmem_##n = {				\
		.area = (uintptr_t *)COND_CODE_1(DT_INST_NODE_HAS_PROP(n,	\
					memory_region),				\
					(DT_REG_ADDR(				\
					DT_INST_PHANDLE(n, memory_region))),	\
					(0)),					\
		.size = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, memory_region),	\
				(DT_REG_SIZE(					\
				DT_INST_PHANDLE(n, memory_region))),(0)),	\
};										\
static const const struct mbox_dt_spec scmi_dt_chan_##n = {			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, mboxes),				\
		(MBOX_DT_SPEC_GET_IDX(DT_DRV_INST(n), 0)),(0))};		\
										\
static const struct stm32_scmi_config stm32_scmi_cfg_##n = {			\
	.dt_agent_id = DT_INST_PROP(n, agent_id),				\
	.dt_agent_name =  DT_INST_PROP(n, agent_name),				\
	.dt_shm = &scmi_dt_shmem_##n,						\
	.dt_chan = &scmi_dt_chan_##n,						\
	.dt_resets = scmi_dt_resets_##n,					\
	.ndt_resets = _DT_INST_RST_LIST_NUM(n),				        \
	.ndt_resets_max = DT_INST_PROP_OR(n, rst_id_max, 0),			\
	.dt_clocks = scmi_dt_clocks_##n,					\
	.ndt_clocks = _DT_INST_CLK_LIST_NUM(n), 				\
	.ndt_clocks_max =  DT_INST_PROP_OR(n, clk_id_max, 0),			\
	.dt_regus  = scmi_dt_regus_##n,						\
	.ndt_regus = ARRAY_SIZE(scmi_dt_regus_##n),				\
	.ndt_regus_max = DT_INST_PROP_OR(n, regu_id_max, 0),			\
	.dt_pd = scmi_dt_pd_##n,						\
	.ndt_pd = _DT_INST_PD_LIST_NUM(n),					\
	.ndt_pd_max =  DT_INST_PROP_OR(n, pd_id_max, 0),			\
};										\
DEVICE_DT_INST_DEFINE(n ,&stm32_scmi_init, NULL,				\
		      &plat_clk_##n[0],						\
		      &stm32_scmi_cfg_##n,					\
		      CORE, 30,							\
		      NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32mp2_scmi

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
static const char dummy[]="";
extern int scp_com_init(const struct mbox_dt_spec *chan, const int agent_id);

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
		/*  initialialize mbox channel if some */
		scpfw_cfg.agent_config[index].channel_count = 1;
		channel_cfg = calloc(scpfw_cfg.agent_config[index].channel_count,
				     sizeof(*scpfw_cfg.agent_config[index].channel_config));
		/*  initialialize mbox channel if some */
		if ((scmi_cfg[i]->dt_chan) && (scmi_cfg[i]->dt_shm->area) &&
		    (scmi_cfg[i]->dt_shm->size)) {
			/*  smt module  */
			channel_cfg->shm.area = scmi_cfg[i]->dt_shm->area;
			channel_cfg->shm.size = scmi_cfg[i]->dt_shm->size;
			if (scp_com_init(scmi_cfg[i]->dt_chan, scmi_cfg[i]->dt_agent_id))
				psa_panic();
			channel_cfg->chan_mbx = (void *)(scmi_cfg[i]->dt_chan);
		}
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
			clk->priv = (struct sm32_scmi_clock *)&no_rate;
			/*  dt option to add */
			if (agent->dt_clocks[j].rate) {
				clk->flags = CLK_SET_RATE_PARENT;
				clk->priv = (struct stm32_scmi_clock *)&rate;
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

			if (regulator_common_is_init_enabled(dev))
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
			clk = clk_get(agent->dt_pd[j]->clk_dev,
				      agent->dt_pd[j]->clk_subsys);
			assert(agent->pd[j].scmi_id < agent->ndt_pd_max);
			channel_cfg->pd[agent->dt_pd[j]->scmi_id] =
				(struct scmi_pd ){
					.name = agent->dt_pd[j]->name,
					.regu = agent->dt_pd[j]->regu_dev,
					.clk = clk,
					.firewall = agent->dt_pd[j]->firewall,
					.n_firewall = agent->dt_pd[j]->n_firewall,
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
