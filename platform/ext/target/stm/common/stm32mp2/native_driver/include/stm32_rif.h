/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#ifndef __STM32_RIF_H
#define __STM32_RIF_H

#include <dt-bindings/rif/stm32mp25-rif.h>

/* These macro and struct are used by rifsc and rif aware devices*/
#define RIFPROT_FLD(_field, _node_id, _prop, _idx) \
	_FLD_GET(_field, DT_PROP_BY_IDX(_node_id, _prop, _idx))

#define STM32_RIFPROT(_node_id, _prop, _idx)			\
	{							\
		.id = RIFPROT_FLD(RIFPROT_PER_ID,		\
				  _node_id, _prop, _idx),	\
		.sec = RIFPROT_FLD(RIFPROT_SEC,			\
				   _node_id, _prop, _idx),	\
		.priv = RIFPROT_FLD(RIFPROT_PRIV,		\
				    _node_id, _prop, _idx),	\
		.cid_attr = RIFPROT_FLD(RIFPROT_PERx_CID,	\
					_node_id, _prop, _idx),	\
	}

struct rifprot_config {
	uint32_t id;
	bool sec;
	bool priv;
	uint32_t cid_attr;
};

/*
 * Loop over each element of rifprot_cfg table
 */
#define for_each_rifprot_cfg(rcfg_table, rcfg_elem, nrcfg, i)		\
	for (i = 0, rcfg_elem = ((struct rifprot_config *)rcfg_table);	\
	     i < (nrcfg);						\
	     i++, rcfg_elem++)

#define RIFPROT_BASE(_sec, _priv, _cid, _sem)				\
	{								\
		.sec = _sec,						\
		.priv = _priv,						\
		.cid = _cid,						\
		.sem = _sem,						\
	}

struct rif_base {
	uintptr_t sec;
	uintptr_t priv;
	uintptr_t cid;
	uintptr_t sem;
};

struct rifprot_controller {
        struct rif_base rbase;
	int nperipherals;
        const struct rifprot_config *rifprot_cfg;
        const int nrifprot;
};

/**
 * @brief Obtain the variable name storing rifprot config for the given DT node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define RIFPROT_CONFIG_NAME(node_id) \
	_CONCAT(__rifprot_cfg, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Obtain the variable name storing rifprot controller for the given DT node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define RIFPROT_CTRL_NAME(node_id) \
	_CONCAT(__rifprot_ctrl, DEVICE_DT_NAME_GET(node_id))

#define RIFPROT_CONFIG_INIT(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, st_protreg),					\
		    (DT_FOREACH_PROP_ELEM_SEP(node_id, st_protreg, STM32_RIFPROT, (,))),	\
		    ())

/**
 * @brief Define all the rifprot configs for the given node identifier.
 *
 * @param node_id Node identifier.
 */
#define DT_RIFPROT_CONFIG_DEFINE(node_id)				\
	const struct rifprot_config RIFPROT_CONFIG_NAME(node_id)[] = {	\
		RIFPROT_CONFIG_INIT(node_id)				\
	}

#define DT_INST_RIFPROT_CONFIG_DEFINE(inst) \
	DT_RIFPROT_CONFIG_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Obtain a reference to the rifprot configs given a node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define DT_RIFPROT_CONFIG_GET(node_id) RIFPROT_CONFIG_NAME(node_id)

/**
 * @brief Obtain a reference to the rifprot config given current
 * compatible instance number.
 *
 * @param inst Instance number.
 *
 * @see #DT_RIFPROT_CONFIG_GET
 */
#define DT_INST_RIFPROT_CONFIG_GET(inst) \
	DT_RIFPROT_CONFIG_GET(DT_DRV_INST(inst))

/**
 * @brief Helper macro to define rifprot controller for a given node identifier.
 *
 * @param node_id Node identifier.
 * @param _sec Adresse base of secure configuration register
 * @param _priv Adresse base of privileged configuration register
 * @param _cid Adresse base of CID configuration register
 * @param _sem Adresse base of semaphore control register
 * @param _nper number of peripherals managed by controller.
 */
#define DT_RIFPROT_CTRL_DEFINE(node_id, _sec, _priv, _cid, _sem, _nper)		\
	DT_RIFPROT_CONFIG_DEFINE(node_id);					\
	extern const struct rifprot_controller RIFPROT_CTRL_NAME(node_id);	\
	const struct rifprot_controller RIFPROT_CTRL_NAME(node_id) = {		\
		.rbase = RIFPROT_BASE(_sec, _priv, _cid, _sem),			\
		.nperipherals = _nper,						\
		.rifprot_cfg = DT_RIFPROT_CONFIG_GET(node_id),			\
		.nrifprot = ARRAY_SIZE(DT_RIFPROT_CONFIG_GET(node_id)),		\
	}

/**
 * @brief Helper macro to define rifprot controller for a  given current
 * compatible instance number.
 *
 * @param inst Instance number.
 * @param _sec Adresse base of secure configuration register
 * @param _priv Adresse base of privileged configuration register
 * @param _cid Adresse base of CID configuration register
 * @param _sem Adresse base of semaphore control register
 * @param _nper number of peripherals managed by controller.
 *
 * @see #DT_RIFPROT_CTRL_DEFINE
 */
#define DT_INST_RIFPROT_CTRL_DEFINE(inst, _sec, _priv, _cid, _sem, _nper)	\
	DT_RIFPROT_CTRL_DEFINE(DT_DRV_INST(inst),				\
			       _sec, _priv, _cid, _sem, _nper)

/**
 * @brief Obtain a reference to the rifprot controller given a node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define DT_RIFPROT_CTRL_GET(node_id) &RIFPROT_CTRL_NAME(node_id)

/**
 * @brief Obtain a reference to the rifprot controller given current
 * compatible instance number.
 *
 * @param inst Instance number.
 *
 * @see #DT_RIFPROT_CTRL_GET
 */
#define DT_INST_RIFPROT_CTRL_GET(inst) \
	DT_RIFPROT_CTRL_GET(DT_DRV_INST(inst))

/**
 * @brief Helper macro to declare a external rifprot controller for a given node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define DT_RIFPROT_CTRL_EXTERN(node_id) \
	extern const struct rifprot_controller RIFPROT_CTRL_NAME(node_id)

/**
 * @brief Helper macro to declare a external rifprot controller for a given
 * current compatible instance number.
 *
 * @param inst Instance number.
 */
#define DT_INST_RIFPROT_CTRL_EXTERN(inst) \
	DT_RIFPROT_CTRL_EXTERN(DT_DRV_INST(inst))

#if (IS_ENABLED(STM32_SEC))
int stm32_rifprot_set_config(const struct rifprot_controller *rifprot_ctl,
			     const struct rifprot_config *rifprot_cfg,
			     int ncfg);
int stm32_rifprot_init(const struct rifprot_controller *rifprot_ctl);
#else
int inline stm32_rifprot_set_config(const struct rifprot_controller *rifprot_ctl,
				    const struct rifprot_config *rifprot_cfg,
				    int ncfg)
{
	return 0;
}

int inline stm32_rifprot_init(const struct rifprot_controller *rifprot_ctl)
{
	return 0;
}
#endif

#endif /* __STM32_RIF_H */
