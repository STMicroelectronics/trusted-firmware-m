/*
 * Copyright (C) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
 */

#ifndef CLK_STM32_CORE_H
#define CLK_STM32_CORE_H

struct mux_cfg {
	uint16_t	offset;
	uint8_t		shift;
	uint8_t		width;
	uint8_t		bitrdy;
};

struct gate_cfg {
	uint16_t	offset;
	uint8_t		bit_idx;
	uint8_t		set_clr;
};

struct clk_div_table {
	unsigned int	val;
	unsigned int	div;
};

struct div_cfg {
	uint16_t	offset;
	uint8_t		shift;
	uint8_t		width;
	uint8_t		flags;
	uint8_t		bitrdy;
	const struct clk_div_table *table;
};

struct parent_cfg {
	uint8_t		num_parents;
	const uint16_t	*id_parents;
	struct mux_cfg	*mux;
};

struct stm32_clk_priv;

struct stm32_clk_ops {
	unsigned long (*recalc_rate)(struct stm32_clk_priv *priv, int id, unsigned long rate);
	int (*get_parent)(struct stm32_clk_priv *priv, int id);
	int (*enable)(struct stm32_clk_priv *priv, int id);
	void (*disable)(struct stm32_clk_priv *priv, int id);
	bool (*is_enabled)(struct stm32_clk_priv *priv, int id);
};

struct clk_stm32 {
	const char		*name;
	uint16_t		binding;
	uint16_t		parent;
	uint8_t			flags;
	void			*clock_cfg;
	const struct stm32_clk_ops	*ops;
};

#define CLK_IS_ROOT 0

#define MUX_NO_BIT_RDY UINT8_MAX

#define CLK_IS_CRITICAL 	BIT(0)
#define CLK_IGNORE_UNUSED 	BIT(1)

struct stm32_clk_priv {
	uintptr_t			base;
	const unsigned int		num;
	const struct clk_stm32		*clks;
	const struct parent_cfg		*parents;
	const struct gate_cfg		*gates;
	const struct div_cfg		*div;
	unsigned int			*gate_refcounts;
	void 				*pdata;
};

struct stm32_clk_bypass {
	uint16_t	offset;
	uint8_t		bit_byp;
	uint8_t		bit_digbyp;
};

struct stm32_clk_css {
	uint16_t	offset;
	uint8_t		bit_css;
};

struct stm32_clk_drive {
	uint16_t	offset;
	uint8_t		drv_shift;
	uint8_t		drv_width;
	uint8_t		drv_default;
};

struct clk_oscillator_data {
	const char		*name;
	unsigned long		frequency;
	uint16_t		gate_id;
	uint16_t		gate_rdy_id;
	struct stm32_clk_bypass *bypass;
	struct stm32_clk_css	*css;
	struct stm32_clk_drive	*drive;
};

struct clk_fixed_rate {
	const char *name;
	unsigned long fixed_rate;
};

struct clk_gate_cfg {
	uint32_t	offset;
	uint8_t		bit_idx;
};

#define _NO_ID UINT16_MAX

#define CLK_DIVIDER_ONE_BASED		BIT(0)
#define CLK_DIVIDER_POWER_OF_TWO	BIT(1)
#define CLK_DIVIDER_ALLOW_ZERO		BIT(2)
#define CLK_DIVIDER_HIWORD_MASK		BIT(3)
#define CLK_DIVIDER_ROUND_CLOSEST	BIT(4)
#define CLK_DIVIDER_READ_ONLY		BIT(5)
#define CLK_DIVIDER_MAX_AT_ZERO		BIT(6)
#define CLK_DIVIDER_BIG_ENDIAN		BIT(7)

#define MUX_MAX_PARENTS U(0x8000)
#define MUX_PARENT_MASK GENMASK(14, 0)
#define MUX_FLAG	U(0x8000)
#define MUX(mux)	((mux) | MUX_FLAG)

#define NO_GATE		0

int stm32_init_clocks(struct stm32_clk_priv *priv);
void stm32_clk_register(void);

struct stm32_clk_priv *clk_stm32_get_priv(void);
uintptr_t clk_stm32_get_rcc_base(void);

int clk_get_index(struct stm32_clk_priv *priv, unsigned long binding_id);
const struct clk_stm32 *_clk_get(struct stm32_clk_priv *priv, int id);


uint16_t _clk_set_mux_by_index(uint16_t id, uint8_t index);

void clk_oscillator_set_bypass(struct stm32_clk_priv *priv, int id, bool digbyp, bool bypass);
void clk_oscillator_set_drive(struct stm32_clk_priv *priv, int id, uint8_t lsedrv);
void clk_oscillator_set_css(struct stm32_clk_priv *priv, int id, bool css);

int _clk_stm32_gate_wait_ready(struct stm32_clk_priv *priv, uint16_t gate_id, bool ready_on);

int clk_oscillator_wait_ready(struct stm32_clk_priv *priv, int id, bool ready_on);
int clk_oscillator_wait_ready_on(struct stm32_clk_priv *priv, int id);
int clk_oscillator_wait_ready_off(struct stm32_clk_priv *priv, int id);

const char *_clk_stm32_get_name(struct stm32_clk_priv *priv, int id);
const char *clk_stm32_get_name(struct stm32_clk_priv *priv, unsigned long binding_id);

void _clk_stm32_gate_disable(struct stm32_clk_priv *priv, uint16_t gate_id);
int _clk_stm32_gate_enable(struct stm32_clk_priv *priv, uint16_t gate_id);

int _clk_stm32_set_parent(struct stm32_clk_priv *priv, int id, int src_id);
int _clk_get_parent_index(struct stm32_clk_priv *priv, int id);
int _clk_stm32_get_parent(struct stm32_clk_priv *priv, int id);

unsigned long _clk_stm32_get_rate(struct stm32_clk_priv *priv, int id);

int _clk_stm32_set_div(struct stm32_clk_priv *priv, int _idx,
		       uint32_t value);

bool _stm32_clk_is_flags(struct stm32_clk_priv *priv, int id, uint8_t flag);

int _clk_stm32_enable(struct stm32_clk_priv *priv, int id);

void _clk_stm32_disable(struct stm32_clk_priv *priv, int id);

bool _clk_stm32_is_enable(struct stm32_clk_priv *priv, int id);

int clk_stm32_get_counter(unsigned long binding_id);

void clk_stm32_oscillator_init(struct stm32_clk_priv *priv, int id, uint32_t frequency);

unsigned long _clk_stm32_divider_recalc(struct stm32_clk_priv *priv,
					int div_id,
					unsigned long prate);

unsigned long clk_stm32_divider_recalc(struct stm32_clk_priv *priv, int idx,
				       unsigned long prate);

extern const struct stm32_clk_ops clk_stm32_divider_ops;

struct clk_stm32_div_cfg {
	int id;
};

#define STM32_DIV(idx, _binding, _parent, _flags, _div_id)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (_parent),\
	.flags = (_flags),\
	.clock_cfg	= &(struct clk_stm32_div_cfg) {\
		.id	= (_div_id),\
	},\
	.ops = &clk_stm32_divider_ops,\
}

int clk_stm32_gate_enable(struct stm32_clk_priv *priv, int idx);
void clk_stm32_gate_disable(struct stm32_clk_priv *priv, int idx);

bool _clk_stm32_gate_is_enabled(struct stm32_clk_priv *priv, int gate_id);
bool clk_stm32_gate_is_enabled(struct stm32_clk_priv *priv, int idx);

extern const struct stm32_clk_ops clk_stm32_gate_ops;

struct clk_stm32_gate_cfg {
	int id;
};

#define STM32_GATE(idx, _binding, _parent, _flags, _gate_id)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (_parent),\
	.flags = (_flags),\
	.clock_cfg	= &(struct clk_stm32_gate_cfg) {\
		.id	= (_gate_id),\
	},\
	.ops = &clk_stm32_gate_ops,\
}

struct fixed_factor_cfg {
	unsigned int mult;
	unsigned int div;
};

unsigned long fixed_factor_recalc_rate(struct stm32_clk_priv *priv,
				       int _idx, unsigned long prate);

extern const struct stm32_clk_ops clk_fixed_factor_ops;

#define FIXED_FACTOR(idx, _idx, _parent, _mult, _div)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent = _parent,\
	.clock_cfg	= &(struct fixed_factor_cfg) {\
		.mult = _mult,\
		.div = _div,\
	},\
	.ops = &clk_fixed_factor_ops,\
}

extern const struct stm32_clk_ops clk_gate_ops;

#define GATE(idx, _binding, _parent, _flags, _offset, _bit_idx)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (_parent),\
	.flags = (_flags),\
	.clock_cfg	= &(struct clk_gate_cfg) {\
		.offset		= (_offset),\
		.bit_idx	= (_bit_idx),\
	},\
	.ops = &clk_gate_ops,\
}

#define STM32_MUX(idx, _binding, _mux_id, _flags)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (MUX(_mux_id)),\
	.flags = (_flags),\
	.clock_cfg = NULL,\
	.ops = NULL,\
}

struct clk_timer_cfg {
	uint32_t apbdiv;
	uint32_t timpre;
};

extern const struct stm32_clk_ops clk_timer_ops;

#define CLK_SET_RATE_PARENT 0

#define CK_TIMER(idx, _idx, _parent, _flags, _apbdiv, _timpre)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent = _parent,\
	.flags = (CLK_SET_RATE_PARENT | (_flags)),\
	.clock_cfg	= &(struct clk_timer_cfg) {\
		.apbdiv = _apbdiv,\
		.timpre = _timpre,\
	},\
	.ops = &clk_timer_ops,\
}

struct clk_stm32_fixed_rate_cfg {
	unsigned long rate;
};

extern const struct stm32_clk_ops clk_stm32_fixed_rate_ops;

#define CLK_FIXED_RATE(idx, _binding, _rate)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (CLK_IS_ROOT),\
	.clock_cfg	= &(struct clk_stm32_fixed_rate_cfg) {\
		.rate	= (_rate),\
	},\
	.ops = &clk_stm32_fixed_rate_ops,\
}

#define BYPASS(_offset, _bit_byp, _bit_digbyp) &(struct stm32_clk_bypass) {\
	.offset = _offset,\
	.bit_byp = _bit_byp,\
	.bit_digbyp = _bit_digbyp,\
}

#define CSS(_offset, _bit_css)	&(struct stm32_clk_css) {\
	.offset = _offset,\
	.bit_css = _bit_css,\
}

#define DRIVE(_offset, _shift, _width, _default) &(struct stm32_clk_drive) {\
	.offset = _offset,\
	.drv_shift = _shift,\
	.drv_width = _width,\
	.drv_default = _default,\
}

#define OSCILLATOR(idx_osc, _id, _name, _gate_id, _gate_rdy_id, _bypass, _css,\
		   _drive)[idx_osc] = {\
	.name		= _name,\
	.gate_id	= _gate_id,\
	.gate_rdy_id	= _gate_rdy_id,\
	.bypass		= _bypass,\
	.css		= _css,\
	.drive		= _drive,\
}

extern const struct stm32_clk_ops clk_stm32_osc_ops;

extern const struct stm32_clk_ops clk_stm32_osc_nogate_ops;

void __unused clk_stm32_display_clock_tree(void);
int clk_stm32_set_div(struct stm32_clk_priv *priv, int div_id, uint32_t value);

#endif /* CLK_STM32_CORE_H */
