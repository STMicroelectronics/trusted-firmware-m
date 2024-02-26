/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stpmic2_regulators

#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <lib/timeout.h>

#include <device.h>
#include <i2c.h>
#include <regulator.h>
#include <linear_range.h>

/* Status Registers */
#define PRODUCT_ID		U(0x00)
#define VERSION_SR		U(0x01)
#define TURN_ON_SR		U(0x02)
#define TURN_OFF_SR		U(0x03)
#define RESTART_SR		U(0x04)
#define OCP_SR1			U(0x05)
#define OCP_SR2			U(0x06)
#define EN_SR1			U(0x07)
#define EN_SR2			U(0x08)
#define FS_CNT_SR1		U(0x09)
#define FS_CNT_SR2		U(0x0A)
#define FS_CNT_SR3		U(0x0B)
#define MODE_SR			U(0x0C)
/* Control Registers */
#define MAIN_CR			U(0x10)
#define VINLOW_CR		U(0x11)
#define PKEY_LKP_CR		U(0x12)
#define WDG_CR			U(0x13)
#define WDG_TMR_CR		U(0x14)
#define WDG_TMR_SR		U(0x15)
#define FS_OCP_CR1		U(0x16)
#define FS_OCP_CR2		U(0x17)
#define PADS_PULL_CR		U(0x18)
#define BUCKS_PD_CR1		U(0x19)
#define BUCKS_PD_CR2		U(0x1A)
#define LDOS_PD_CR1		U(0x1B)
#define LDOS_PD_CR2		U(0x1C)
#define BUCKS_MRST_CR		U(0x1D)
#define LDOS_MRST_CR		U(0x1E)
/* Buck CR */
#define BUCK1_MAIN_CR1		U(0x20)
#define BUCK1_MAIN_CR2		U(0x21)
#define BUCK1_ALT_CR1		U(0x22)
#define BUCK1_ALT_CR2		U(0x23)
#define BUCK1_PWRCTRL_CR	U(0x24)
#define BUCK2_MAIN_CR1		U(0x25)
#define BUCK2_MAIN_CR2		U(0x26)
#define BUCK2_ALT_CR1		U(0x27)
#define BUCK2_ALT_CR2		U(0x28)
#define BUCK2_PWRCTRL_CR	U(0x29)
#define BUCK3_MAIN_CR1		U(0x2A)
#define BUCK3_MAIN_CR2		U(0x2B)
#define BUCK3_ALT_CR1		U(0x2C)
#define BUCK3_ALT_CR2		U(0x2D)
#define BUCK3_PWRCTRL_CR	U(0x2E)
#define BUCK4_MAIN_CR1		U(0x2F)
#define BUCK4_MAIN_CR2		U(0x30)
#define BUCK4_ALT_CR1		U(0x31)
#define BUCK4_ALT_CR2		U(0x32)
#define BUCK4_PWRCTRL_CR	U(0x33)
#define BUCK5_MAIN_CR1		U(0x34)
#define BUCK5_MAIN_CR2		U(0x35)
#define BUCK5_ALT_CR1		U(0x36)
#define BUCK5_ALT_CR2		U(0x37)
#define BUCK5_PWRCTRL_CR	U(0x38)
#define BUCK6_MAIN_CR1		U(0x39)
#define BUCK6_MAIN_CR2		U(0x3A)
#define BUCK6_ALT_CR1		U(0x3B)
#define BUCK6_ALT_CR2		U(0x3C)
#define BUCK6_PWRCTRL_CR	U(0x3D)
#define BUCK7_MAIN_CR1		U(0x3E)
#define BUCK7_MAIN_CR2		U(0x3F)
#define BUCK7_ALT_CR1		U(0x40)
#define BUCK7_ALT_CR2		U(0x41)
#define BUCK7_PWRCTRL_CR	U(0x42)
/* LDO CR */
#define LDO1_MAIN_CR		U(0x4C)
#define LDO1_ALT_CR		U(0x4D)
#define LDO1_PWRCTRL_CR		U(0x4E)
#define LDO2_MAIN_CR		U(0x4F)
#define LDO2_ALT_CR		U(0x50)
#define LDO2_PWRCTRL_CR		U(0x51)
#define LDO3_MAIN_CR		U(0x52)
#define LDO3_ALT_CR		U(0x53)
#define LDO3_PWRCTRL_CR		U(0x54)
#define LDO4_MAIN_CR		U(0x55)
#define LDO4_ALT_CR		U(0x56)
#define LDO4_PWRCTRL_CR		U(0x57)
#define LDO5_MAIN_CR		U(0x58)
#define LDO5_ALT_CR		U(0x59)
#define LDO5_PWRCTRL_CR		U(0x5A)
#define LDO6_MAIN_CR		U(0x5B)
#define LDO6_ALT_CR		U(0x5C)
#define LDO6_PWRCTRL_CR		U(0x5D)
#define LDO7_MAIN_CR		U(0x5E)
#define LDO7_ALT_CR		U(0x5F)
#define LDO7_PWRCTRL_CR		U(0x60)
#define LDO8_MAIN_CR		U(0x61)
#define LDO8_ALT_CR		U(0x62)
#define LDO8_PWRCTRL_CR		U(0x63)
#define REFDDR_MAIN_CR		U(0x64)
#define REFDDR_ALT_CR		U(0x65)
#define REFDDR_PWRCTRL_CR	U(0x66)
/* INTERRUPT CR */
#define INT_PENDING_R1		U(0x70)
#define INT_PENDING_R2		U(0x71)
#define INT_PENDING_R3		U(0x72)
#define INT_PENDING_R4		U(0x73)
#define INT_CLEAR_R1		U(0x74)
#define INT_CLEAR_R2		U(0x75)
#define INT_CLEAR_R3		U(0x76)
#define INT_CLEAR_R4		U(0x77)
#define INT_MASK_R1		U(0x78)
#define INT_MASK_R2		U(0x79)
#define INT_MASK_R3		U(0x7A)
#define INT_MASK_R4		U(0x7B)
#define INT_SRC_R1		U(0x7C)
#define INT_SRC_R2		U(0x7D)
#define INT_SRC_R3		U(0x7E)
#define INT_SRC_R4		U(0x7F)
#define INT_DBG_LATCH_R1	U(0x80)
#define INT_DBG_LATCH_R2	U(0x81)
#define INT_DBG_LATCH_R3	U(0x82)
#define INT_DBG_LATCH_R4	U(0x83)

/* BUCKS_MRST_CR bits definition */
#define BUCK1_MRST		BIT(0)
#define BUCK2_MRST		BIT(1)
#define BUCK3_MRST		BIT(2)
#define BUCK4_MRST		BIT(3)
#define BUCK5_MRST		BIT(4)
#define BUCK6_MRST		BIT(5)
#define BUCK7_MRST		BIT(6)
#define REFDDR_MRST		BIT(7)

/* LDOS_MRST_CR bits definition */
#define LDO1_MRST		BIT(0)
#define LDO2_MRST		BIT(1)
#define LDO3_MRST		BIT(2)
#define LDO4_MRST		BIT(3)
#define LDO5_MRST		BIT(4)
#define LDO6_MRST		BIT(5)
#define LDO7_MRST		BIT(6)
#define LDO8_MRST		BIT(7)

/* LDOx_MAIN_CR */
#define LDO_VOUT_SHIFT		1
#define LDO_VOUT_MASK		GENMASK_32(5, 1)
#define LDO_BYPASS		BIT(6)
#define LDO1_INPUT_SRC		BIT(7)
#define LDO3_SNK_SRC		BIT(7)
#define LDO4_INPUT_SRC_SHIFT	6
#define LDO4_INPUT_SRC_MASK	GENMASK_32(7, 6)

/* PWRCTRL register bit definition */
#define PWRCTRL_EN		BIT(0)
#define PWRCTRL_RS		BIT(1)
#define PWRCTRL_SEL_SHIFT	2
#define PWRCTRL_SEL_MASK	GENMASK_32(3, 2)

/* BUCKx_MAIN_CR2 */
#define BUCKX_VOUT_SHIFT	0
#define BUCKX_VOUT_MASK		GENMASK_32(6, 0)

/* BUCKx_MAIN_CR2 */
#define PREG_MODE_SHIFT		1
#define PREG_MODE_MASK		GENMASK_32(2, 1)

/* BUCKS_PD_CR1 */
#define BUCK1_PD_MASK		GENMASK_32(1, 0)
#define BUCK2_PD_MASK		GENMASK_32(3, 2)
#define BUCK3_PD_MASK		GENMASK_32(5, 4)
#define BUCK4_PD_MASK		GENMASK_32(7, 6)

#define BUCK1_PD_FAST		BIT(1)
#define BUCK2_PD_FAST		BIT(3)
#define BUCK3_PD_FAST		BIT(5)
#define BUCK4_PD_FAST		BIT(7)

/* BUCKS_PD_CR2 */
#define BUCK5_PD_MASK		GENMASK_32(1, 0)
#define BUCK6_PD_MASK		GENMASK_32(3, 2)
#define BUCK7_PD_MASK		GENMASK_32(5, 4)

#define BUCK5_PD_FAST		BIT(1)
#define BUCK6_PD_FAST		BIT(3)
#define BUCK7_PD_FAST		BIT(5)

/* LDOS_PD_CR1 */
#define LDO1_PD			BIT(0)
#define LDO2_PD			BIT(1)
#define LDO3_PD			BIT(2)
#define LDO4_PD			BIT(3)
#define LDO5_PD			BIT(4)
#define LDO6_PD			BIT(5)
#define LDO7_PD			BIT(6)
#define LDO8_PD			BIT(7)

/* LDOS_PD_CR2 */
#define REFDDR_PD		BIT(0)

/* FS_OCP_CR1 */
#define FS_OCP_BUCK1		BIT(0)
#define FS_OCP_BUCK2		BIT(1)
#define FS_OCP_BUCK3		BIT(2)
#define FS_OCP_BUCK4		BIT(3)
#define FS_OCP_BUCK5		BIT(4)
#define FS_OCP_BUCK6		BIT(5)
#define FS_OCP_BUCK7		BIT(6)
#define FS_OCP_REFDDR		BIT(7)

/* FS_OCP_CR2 */
#define FS_OCP_LDO1		BIT(0)
#define FS_OCP_LDO2		BIT(1)
#define FS_OCP_LDO3		BIT(2)
#define FS_OCP_LDO4		BIT(3)
#define FS_OCP_LDO5		BIT(4)
#define FS_OCP_LDO6		BIT(5)
#define FS_OCP_LDO7		BIT(6)
#define FS_OCP_LDO8		BIT(7)

/* IRQ definitions */
#define IT_PONKEY_F		U(0)
#define IT_PONKEY_R		U(1)
#define IT_BUCK1_OCP		U(16)
#define IT_BUCK2_OCP		U(17)
#define IT_BUCK3_OCP		U(18)
#define IT_BUCK4_OCP		U(19)
#define IT_BUCK5_OCP		U(20)
#define IT_BUCK6_OCP		U(21)
#define IT_BUCK7_OCP		U(22)
#define IT_REFDDR_OCP		U(23)
#define IT_LDO1_OCP		U(24)
#define IT_LDO2_OCP		U(25)
#define IT_LDO3_OCP		U(26)
#define IT_LDO4_OCP		U(27)
#define IT_LDO5_OCP		U(28)
#define IT_LDO6_OCP		U(29)
#define IT_LDO7_OCP		U(30)
#define IT_LDO8_OCP		U(31)

enum stpmic2_prop_id {
	STPMIC2_MASK_RESET = 0,
	STPMIC2_PULL_DOWN,
	STPMIC2_BYPASS,
	STPMIC2_SINK_SOURCE,
	STPMIC2_OCP,
	STPMIC2_PWRCTRL_EN,
	STPMIC2_PWRCTRL_RS,
	STPMIC2_PWRCTRL_SEL,	/* takes arg = pwrctrl line number */
	STPMIC2_MAIN_PREG_MODE,	/* takes arg = preg mode HP=1, CCM=2 */
	STPMIC2_ALT_PREG_MODE,	/* takes arg = preg mode HP=1, CCM=2 */
	STPMIC2_BYPASS_UV,
};

struct regu_stpmic2_desc {
	const char *name;
	const struct linear_range *ranges;
	uint8_t nranges;
	uint8_t volt_cr;
	uint8_t volt_shift;
	uint8_t volt_mask;
	uint8_t en_cr;
	uint8_t alt_en_cr;
	uint8_t alt_volt_cr;
	uint8_t pwrctrl_cr;
	uint8_t msrt_reg;
	uint8_t msrt_mask;
	uint8_t pd_reg;
	uint8_t pd_val;
	uint8_t pd_mask;
	uint8_t ocp_reg;
	uint8_t ocp_mask;
	bool has_bypass;
	bool has_sink;
	bool has_preg;
};

/* Voltage tables in uV */
static const struct linear_range __maybe_unused buck1236_ranges[] = {
	LINEAR_RANGE_INIT(500000, 10000U, 0, 100),
	LINEAR_RANGE_INIT(1500000, 0, 101, 127),
};

static const struct linear_range __maybe_unused buck457_ranges[] = {
	LINEAR_RANGE_INIT(1500000, 0, 0, 100),
	LINEAR_RANGE_INIT(1600000, 100000, 101, 127),
};

static const struct linear_range __maybe_unused ldo235678_ranges[] = {
	LINEAR_RANGE_INIT(900000, 100000, 0, 31),
};

static const struct linear_range __maybe_unused ldo1_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 0, 0, 0),
};

static const struct linear_range __maybe_unused ldo4_ranges[] = {
	LINEAR_RANGE_INIT(3300000, 0, 0, 0),
};

static const struct linear_range __maybe_unused refddr_ranges[] = {
	LINEAR_RANGE_INIT(0, 0, 0, 0),
};

#define DEFINE_BUCK(_regu_name, _id, _pd, _ranges) {		\
	.name			= _regu_name,			\
	.ranges			= _ranges,			\
	.nranges		= ARRAY_SIZE(_ranges),		\
	.en_cr			= _id ## _MAIN_CR2,		\
	.volt_cr		= _id ## _MAIN_CR1,		\
	.volt_shift		= BUCKX_VOUT_SHIFT,		\
	.volt_mask		= BUCKX_VOUT_MASK,		\
	.alt_en_cr		= _id ## _ALT_CR2,		\
	.alt_volt_cr		= _id ## _ALT_CR1,		\
	.pwrctrl_cr		= _id ## _PWRCTRL_CR,		\
	.msrt_reg		= BUCKS_MRST_CR,		\
	.msrt_mask		= _id ## _MRST,			\
	.pd_reg			= _pd,				\
	.pd_val			= _id ## _PD_FAST,		\
	.pd_mask		= _id ## _PD_MASK,		\
	.ocp_reg		= FS_OCP_CR1,			\
	.ocp_mask		= FS_OCP_ ## _id,		\
	.has_bypass		= false,			\
	.has_sink		= false,			\
	.has_preg		= true,				\
}

#define DEFINE_LDO_BYPASS(_regu_name, _id, _pd, _ranges) {	\
	.name			= _regu_name,			\
	.ranges			= _ranges,			\
	.nranges		= ARRAY_SIZE(_ranges),		\
	.volt_shift		= LDO_VOUT_SHIFT,		\
	.volt_mask		= LDO_VOUT_MASK,		\
	.en_cr			= _id ## _MAIN_CR,		\
	.volt_cr		= _id ## _MAIN_CR,		\
	.alt_en_cr		= _id ## _ALT_CR,		\
	.alt_volt_cr		= _id ## _ALT_CR,		\
	.pwrctrl_cr		= _id ## _PWRCTRL_CR,		\
	.msrt_reg		= LDOS_MRST_CR,			\
	.msrt_mask		= _id ## _MRST,			\
	.pd_reg			= LDOS_PD_CR1,			\
	.pd_val			= _id ## _PD,			\
	.pd_mask		= _id ## _PD,			\
	.ocp_reg		= FS_OCP_CR2,			\
	.ocp_mask		= FS_OCP_ ## _id,		\
	.has_bypass		= true,				\
	.has_sink		= false,			\
}

#define DEFINE_LDO_BYPASS_SINK(_regu_name, _id, _pd, _ranges) {	\
	.name			= _regu_name,			\
	.ranges			= _ranges,			\
	.nranges		= ARRAY_SIZE(_ranges),		\
	.volt_shift		= LDO_VOUT_SHIFT,		\
	.volt_mask		= LDO_VOUT_MASK,		\
	.en_cr			= _id ## _MAIN_CR,		\
	.volt_cr		= _id ## _MAIN_CR,		\
	.alt_en_cr		= _id ## _ALT_CR,		\
	.alt_volt_cr		= _id ## _ALT_CR,		\
	.pwrctrl_cr		= _id ## _PWRCTRL_CR,		\
	.msrt_reg		= LDOS_MRST_CR,			\
	.msrt_mask		= _id ## _MRST,			\
	.pd_reg			= LDOS_PD_CR1,			\
	.pd_val			= _id ## _PD,			\
	.pd_mask		= _id ## _PD,			\
	.ocp_reg		= FS_OCP_CR2,			\
	.ocp_mask		= FS_OCP_ ## _id,		\
	.has_bypass		= true,				\
	.has_sink		= true,				\
}

#define DEFINE_LDO(_regu_name, _id, _pd, _ranges) {		\
	.name			= _regu_name,			\
	.ranges			= _ranges,			\
	.nranges		= ARRAY_SIZE(_ranges),		\
	.volt_shift		= LDO_VOUT_SHIFT,		\
	.volt_mask		= 0x0,				\
	.en_cr			= _id ## _MAIN_CR,		\
	.volt_cr		= _id ## _MAIN_CR,		\
	.alt_en_cr		= _id ## _ALT_CR,		\
	.alt_volt_cr		= _id ## _ALT_CR,		\
	.pwrctrl_cr		= _id ## _PWRCTRL_CR,		\
	.msrt_reg		= LDOS_MRST_CR,			\
	.msrt_mask		= _id ## _MRST,			\
	.pd_reg			= LDOS_PD_CR1,			\
	.pd_val			= _id ## _PD,			\
	.pd_mask		= _id ## _PD,			\
	.ocp_reg		= FS_OCP_CR2,			\
	.ocp_mask		= FS_OCP_ ## _id,		\
	.has_bypass		= false,			\
	.has_sink		= false,			\
}

#define DEFINE_REFDDR(_regu_name, _id, _pd, _ranges) {		\
	.name			= _regu_name,			\
	.ranges			= _ranges,			\
	.nranges		= ARRAY_SIZE(_ranges),		\
	.en_cr			= _id ## _MAIN_CR,		\
	.volt_cr		= _id ## _MAIN_CR,		\
	.alt_en_cr		= _id ## _ALT_CR,		\
	.alt_volt_cr		= _id ## _ALT_CR,		\
	.pwrctrl_cr		= _id ## _PWRCTRL_CR,		\
	.msrt_reg		= BUCKS_MRST_CR,		\
	.msrt_mask		= _id ## _MRST,			\
	.pd_reg			= LDOS_PD_CR2,			\
	.pd_val			= _id ## _PD,			\
	.pd_mask		= _id ## _PD,			\
	.ocp_reg		= FS_OCP_CR1,			\
	.ocp_mask		= FS_OCP_ ## _id,		\
	.has_bypass		= false,			\
	.has_sink		= false,			\
}

struct regu_stpmic2_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	const struct regu_stpmic2_desc desc;
	int32_t	st_bypass_uv;
	bool  st_mask_reset;
};

struct regu_stpmic2_data {
	struct regulator_common_data data;
	bool st_mask_reset;
	bool st_bypass;
	bool st_pwrctrl;
	bool st_pwrctrl_reset;
	bool st_sink_source;
	int32_t st_pwrctrl_sel;
	int32_t	st_bypass_uv;
};

static int stpmic2_update_en_crs(const struct device *dev,
				 uint8_t mask, uint8_t value)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;
	int err;

	err = i2c_reg_update_byte_dt(&drv_cfg->i2c, regu_desc->en_cr,
				     mask, value);
	if (err)
		return err;

	return i2c_reg_update_byte_dt(&drv_cfg->i2c, regu_desc->alt_en_cr,
				      mask, value);
}

static int stpmic2_set_prop(const struct device *dev,
			    enum stpmic2_prop_id prop, uint8_t arg)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;
	struct regu_stpmic2_data *drv_data = dev_get_data(dev);
	int err = 0;

	switch (prop) {
	case STPMIC2_PULL_DOWN:
		return i2c_reg_update_byte_dt(&drv_cfg->i2c, regu_desc->pd_reg,
					      regu_desc->pd_val,
					      regu_desc->pd_val);
	case STPMIC2_MASK_RESET:
		if (!regu_desc->msrt_mask)
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->msrt_reg,
					      regu_desc->msrt_mask,
					      regu_desc->msrt_mask);
	case STPMIC2_BYPASS:
		if (!regu_desc->has_bypass)
			return -ENOTSUP;

		/* clear sink source mode */
		if (regu_desc->has_sink) {
			err = stpmic2_update_en_crs(dev, LDO3_SNK_SRC, 0);
			if (err)
				return err;
		}

		return stpmic2_update_en_crs(dev, LDO_BYPASS,
					     drv_data->st_bypass ?
					     LDO_BYPASS : 0);
	case STPMIC2_SINK_SOURCE:
		if (!regu_desc->has_sink)
			return -ENOTSUP;

		/* clear bypass mode */
		err = stpmic2_update_en_crs(dev, LDO_BYPASS, 0);
		if (err)
			return err;

		return stpmic2_update_en_crs(dev, LDO3_SNK_SRC, LDO3_SNK_SRC);
	case STPMIC2_OCP:
		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->ocp_reg,
					      regu_desc->ocp_mask,
					      regu_desc->ocp_mask);
	case STPMIC2_PWRCTRL_EN:
		if (!regu_desc->pwrctrl_cr)
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->pwrctrl_cr,
					      PWRCTRL_EN | PWRCTRL_RS,
					      PWRCTRL_EN);
	case STPMIC2_PWRCTRL_RS:
		if (!regu_desc->pwrctrl_cr)
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->pwrctrl_cr,
					      PWRCTRL_EN | PWRCTRL_RS,
					      PWRCTRL_RS);
	case STPMIC2_PWRCTRL_SEL:
		if (!regu_desc->pwrctrl_cr)
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->pwrctrl_cr,
					      PWRCTRL_SEL_MASK,
					      _FLD_PREP(PWRCTRL_SEL, drv_data->st_pwrctrl_sel));
	case STPMIC2_MAIN_PREG_MODE:
		if ((!regu_desc->has_preg) || (arg > 2))
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->en_cr,
					      PREG_MODE_MASK,
					      _FLD_PREP(PREG_MODE, arg));
	case STPMIC2_ALT_PREG_MODE:
		if ((!regu_desc->has_preg) || (arg > 2))
			return -ENOTSUP;

		return i2c_reg_update_byte_dt(&drv_cfg->i2c,
					      regu_desc->alt_en_cr,
					      PREG_MODE_MASK,
					      _FLD_PREP(PREG_MODE, arg));
	default:
		err = -EINVAL;
	}

	return err;
}

static int stpmic2_reg_enable(const struct device *dev)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;

	return i2c_reg_update_byte_dt(&drv_cfg->i2c, regu_desc->en_cr, 1, 1);
}

static int stpmic2_reg_disable(const struct device *dev)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;

	return i2c_reg_update_byte_dt(&drv_cfg->i2c, regu_desc->en_cr, 1, 0);
}

static unsigned int stpmic2_reg_count_voltages(const struct device *dev)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;

	return linear_range_group_values_count(regu_desc->ranges,
					       regu_desc->nranges);
}

static int stpmic2_reg_list_voltage(const struct device *dev, unsigned int idx,
				     int32_t *volt_uv)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;

	return linear_range_group_get_value(regu_desc->ranges,
					    regu_desc->nranges, idx, volt_uv);
}

static int stpmic2_reg_set_voltage(const struct device *dev, int32_t min_uv,
				    int32_t max_uv)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;
	struct regu_stpmic2_data *drv_data = dev_get_data(dev);
	uint8_t reg_idx;
	int32_t val_uv;
	uint16_t idx = 0;
	int err;

	err = linear_range_group_get_win_index(regu_desc->ranges,
					       regu_desc->nranges,
					       min_uv, max_uv, &idx);

	err |= linear_range_group_get_value(regu_desc->ranges,
					    regu_desc->nranges, idx, &val_uv);

	if (err)
		return -EINVAL;

	if (val_uv == drv_data->st_bypass_uv) {
		err = stpmic2_set_prop(dev, STPMIC2_BYPASS, 0);
		if (err)
			return err;
	}

	reg_idx = (idx << regu_desc->volt_shift) & regu_desc->volt_mask;

	return i2c_reg_update_byte_dt(&drv_cfg->i2c,
				      regu_desc->volt_cr,
				      regu_desc->volt_mask, reg_idx);
}

static int stpmic2_reg_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;
	struct regu_stpmic2_data *drv_data = dev_get_data(dev);
	uint8_t val;
	int err;

	if (drv_data->st_bypass_uv != 0 && regu_desc->has_bypass) {

		err = i2c_reg_read_byte_dt(&drv_cfg->i2c,
					   regu_desc->en_cr, &val);
		if (err)
			return err;

		if (val & LDO_BYPASS) {
			*volt_uv = drv_data->st_bypass_uv;
			return 0;
		}
	}

	err = i2c_reg_read_byte_dt(&drv_cfg->i2c, regu_desc->volt_cr, &val);
	if (err)
		return err;

	val = (val & regu_desc->volt_mask) >> regu_desc->volt_shift;

	return linear_range_group_get_value(regu_desc->ranges,
					    regu_desc->nranges, val, volt_uv);
}

static int _show_reg(const struct i2c_dt_spec *i2c, uint8_t i2c_addr, char *name)
{
	uint8_t val;
	int err;

	if (!i2c_addr)
		return -ENODEV;

	err = i2c_reg_read_byte_dt(i2c, i2c_addr, &val);
	if (err) {
		DMSG("read %s error", name);
		return err;
	}

	IMSG("\t %-16s \t[%#02x]=%#02x", name, i2c_addr, val);
	return 0;
}

static int stpmic2_reg_show(const struct device *dev)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	const struct regu_stpmic2_desc *regu_desc = &drv_cfg->desc;

	IMSG("dump regu:%s", dev->name);

	_show_reg(&drv_cfg->i2c, regu_desc->volt_cr, "volt_cr");
	_show_reg(&drv_cfg->i2c, regu_desc->en_cr, "en_cr");
	_show_reg(&drv_cfg->i2c, regu_desc->alt_en_cr, "alt_en_cr");
	_show_reg(&drv_cfg->i2c, regu_desc->alt_volt_cr, "alt_volt_cr");
	_show_reg(&drv_cfg->i2c, regu_desc->pwrctrl_cr, "pwrctrl_cr");
	_show_reg(&drv_cfg->i2c, regu_desc->msrt_reg, "msrt_reg");
	_show_reg(&drv_cfg->i2c, regu_desc->pd_reg, "pd_reg");
	_show_reg(&drv_cfg->i2c, regu_desc->ocp_reg, "ocp_reg");
}

static int stpmic2_parse_prop(const struct device *dev)
{
	struct regu_stpmic2_data *drv_data = dev_get_data(dev);
	int err = 0;

	if (drv_data->st_mask_reset)
		err |= stpmic2_set_prop(dev, STPMIC2_MASK_RESET, 0);

	if (drv_data->st_pwrctrl_sel)
		err |= stpmic2_set_prop(dev, STPMIC2_PWRCTRL_SEL, 0);

	if (drv_data->st_pwrctrl_reset)
		err |= stpmic2_set_prop(dev, STPMIC2_PWRCTRL_RS, 0);

	if (drv_data->st_pwrctrl)
		err |= stpmic2_set_prop(dev, STPMIC2_PWRCTRL_EN, 0);

	if (drv_data->st_bypass)
		err |= stpmic2_set_prop(dev, STPMIC2_BYPASS, 0);

	if (drv_data->st_sink_source)
		err |= stpmic2_set_prop(dev, STPMIC2_SINK_SOURCE, 0);

	return err ? -EINVAL : 0;
}

static int __used stpmic2_reg_init(const struct device *dev)
{
	const struct regu_stpmic2_config *drv_cfg = dev_get_config(dev);
	uint8_t prod_id, version;
	int err;

	regulator_common_data_init(dev);

	if (!i2c_is_ready_dt(&drv_cfg->i2c))
		return -ENODEV;

	err = i2c_reg_read_byte_dt(&drv_cfg->i2c, PRODUCT_ID, &prod_id);
	if (err)
		return err;

	err = i2c_reg_read_byte_dt(&drv_cfg->i2c, VERSION_SR, &version);
	if (err)
		return err;

	err = stpmic2_parse_prop(dev);
	if (err)
		return err;

	DMSG("init:%s pmic id:%#x version:%#x", dev->name, prod_id, version);

	return regulator_common_init(dev, false);
};

static const struct regulator_driver_api stpmic2_api = {
	.enable = stpmic2_reg_enable,
	.disable = stpmic2_reg_disable,
	.count_voltages = stpmic2_reg_count_voltages,
	.list_voltage = stpmic2_reg_list_voltage,
	.set_voltage = stpmic2_reg_set_voltage,
	.get_voltage = stpmic2_reg_get_voltage,
	.show = stpmic2_reg_show,
};

#define REGULATOR_STPMIC2_DEFINE(node_id, id, macro_desc, reg_id, pd, ranges)		\
	static struct regu_stpmic2_data data_##id = {					\
		.st_mask_reset = DT_PROP(node_id, st_mask_reset),			\
		.st_bypass = DT_PROP(node_id, st_regulator_bypass),			\
		.st_pwrctrl = DT_PROP(node_id, st_pwrctrl_enable),			\
		.st_pwrctrl_reset = DT_PROP(node_id, st_pwrctrl_reset),			\
		.st_pwrctrl_sel = DT_PROP_OR(node_id, st_regulator_bypass_microvolt, 0),\
		.st_bypass_uv = DT_PROP_OR(node_id, st_pwrctrl_sel, 0),			\
		.st_sink_source = DT_PROP(node_id, st_sink_source),			\
	};										\
											\
	static const struct regu_stpmic2_config cfg_##id = {				\
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),			\
		.i2c = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),				\
		.desc = macro_desc(STRINGIFY(id), reg_id, pd, ranges),			\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, &stpmic2_reg_init, &data_##id, &cfg_##id,		\
			 CORE, 6, &stpmic2_api);

#define REGULATOR_STPMIC2_DEFINE_COND(inst, child, macro_desc, reg_id, pd, ranges)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),				\
		    (REGULATOR_STPMIC2_DEFINE(DT_INST_CHILD(inst, child),		\
					      inst ## _ ## child,			\
					      macro_desc, reg_id, pd, ranges)),		\
		    ())

#define REGULATOR_STPMIC2_DEFINE_ALL(inst)						\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck1, DEFINE_BUCK,				\
				      BUCK1, BUCKS_PD_CR1, buck1236_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck2, DEFINE_BUCK,				\
				      BUCK2, BUCKS_PD_CR1, buck1236_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck3, DEFINE_BUCK,				\
				      BUCK3, BUCKS_PD_CR1, buck1236_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck4, DEFINE_BUCK,				\
				      BUCK4, BUCKS_PD_CR1, buck457_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck5, DEFINE_BUCK,				\
				      BUCK5, BUCKS_PD_CR2, buck457_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck6, DEFINE_BUCK,				\
				      BUCK6, BUCKS_PD_CR2, buck1236_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, buck7, DEFINE_BUCK,				\
				      BUCK7, BUCKS_PD_CR2, buck457_ranges)		\
	REGULATOR_STPMIC2_DEFINE_COND(inst, refddr, DEFINE_REFDDR,			\
				      REFDDR, NULL, refddr_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo1, DEFINE_LDO,				\
				      LDO1, NULL, ldo1_ranges)				\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo2, DEFINE_LDO_BYPASS,			\
				      LDO2, NULL, ldo235678_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo3, DEFINE_LDO_BYPASS_SINK,		\
				      LDO3, NULL, ldo235678_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo4, DEFINE_LDO,				\
				      LDO4, NULL, ldo4_ranges)				\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo5, DEFINE_LDO_BYPASS,			\
				      LDO5, NULL, ldo235678_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo6, DEFINE_LDO_BYPASS,			\
				      LDO6, NULL, ldo235678_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo7, DEFINE_LDO_BYPASS,			\
				      LDO7, NULL, ldo235678_ranges)			\
	REGULATOR_STPMIC2_DEFINE_COND(inst, ldo8, DEFINE_LDO_BYPASS,			\
				      LDO8, NULL, ldo235678_ranges)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_STPMIC2_DEFINE_ALL)
