/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_RIFSC_H
#define __STM32_RIFSC_H

#include <dt-bindings/rif/stm32mp25-rifsc.h>

/* These macro and struct are used by rifsc and rif aware devices*/
#define RIFPROT_FLD(_field, _node_id, _prop, _idx)					\
	_FLD_GET(_field, DT_PROP_BY_IDX(_node_id, _prop, _idx))

#define STM32_RIFPROT(_node_id, _prop, _idx)						\
	{										\
		.id = RIFPROT_FLD(RIFSC_RISC_PER_ID, _node_id, _prop, _idx),		\
		.sec = RIFPROT_FLD(RIFSC_RISC_SEC, _node_id, _prop, _idx),		\
		.priv = RIFPROT_FLD(RIFSC_RISC_PRIV, _node_id, _prop, _idx),		\
		.cid_attr = RIFPROT_FLD(RIFSC_RISC_PERx_CID, _node_id, _prop, _idx),	\
	}

struct risup_cfg {
	uint32_t id;
	bool sec;
	bool priv;
	uint32_t cid_attr;
};

int stm32_rifsc_get_access_by_id(const struct device *dev, uint32_t id);
int stm32_set_risup(const struct device *dev, const struct risup_cfg *risup,
		    const int nrisup);

#endif /* __STM32_RIFSC_H */
