/*
 * Copyright (C) 2021-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp2_ddr

#include <errno.h>
#include <string.h>

#include <stm32mp_ddr_debug.h>
#include <stm32mp_ddr.h>
#include <stm32mp_ddr_test.h>
#include <stm32mp2_ddr.h>
#include <stm32mp2_ddr_helpers.h>
#include <lib/mmio.h>
#include <lib/delay.h>

#include <ddrphy_phyinit.h>
#include <device.h>
#include <regulator.h>
#include <clk.h>

/*
 * Cortex m33 System control starts at 0xE000 0000
 * Max ddr size is 0xE000 0000 - DDR_MEM_BASE
 */
#define DDR_MAX_SIZE	(0xE0000000 - DDR_MEM_BASE)

uintptr_t stm32mp_ddrphyc_base(void)
{
	return DT_INST_REG_ADDR_BY_NAME(0, phy);
}

uintptr_t stm32mp_ddrctrl_base(void)
{
	return DT_INST_REG_ADDR_BY_NAME(0, ctrl);
}

uintptr_t stm32_ddrdbg_get_base(void)
{
	return DT_INST_REG_ADDR_BY_NAME(0, dbg);
}

uintptr_t stm32mp_pwr_base(void)
{
	return DT_REG_ADDR(DT_NODELABEL(pwr));
}

uintptr_t stm32mp_rcc_base(void)
{
	return DT_REG_ADDR(DT_NODELABEL(rcc));
}

int stm32mp_board_ddr_power_init(enum ddr_type ddr_type)
{
	const struct device *dev_vpp, *dev_vdd, *dev_vref, *dev_vtt;
	int err;

	dev_vpp = DT_INST_DEV_REGULATOR_SUPPLY(0, vpp);
	if (!dev_vpp)
		return -ENODEV;

	err = regulator_common_set_min_voltage(dev_vpp);
	if (err)
		return err;

	dev_vdd = DT_INST_DEV_REGULATOR_SUPPLY(0, vdd);
	if (!dev_vdd)
		return -ENODEV;

	err = regulator_common_set_min_voltage(dev_vdd);
	if (err)
		return err;

	dev_vref = DT_INST_DEV_REGULATOR_SUPPLY(0, vref);
	if (!dev_vref)
		return -ENODEV;

	dev_vtt = DT_INST_DEV_REGULATOR_SUPPLY(0, vtt);
	if (!dev_vtt)
		return -ENODEV;

	err = regulator_enable(dev_vpp);
	if (err)
		return err;

	/* could be set via enable_ramp_delay on vpp_ddr */
	udelay(2000);

	err = regulator_enable(dev_vdd);
	if (err)
		return err;

	err = regulator_enable(dev_vref);
	if (err)
		return err;

	err = regulator_enable(dev_vtt);
	if (err)
		return err;

	return 0;
}

#define DDR_CLK_DEV	DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0))
#define DDR_CLK_ID	DT_INST_CLOCKS_CELL(0, bits)

static int stm32mp2_ddr_dt_init(void)
{
	unsigned long ret;
	struct clk *clk;

	struct stm32mp_ddr_config drv_cfg = {
		.info = {
			.speed = DT_INST_PROP(0, st_mem_speed),
			.size = DT_INST_PROP(0, st_mem_size),
			.name = DT_INST_PROP(0, st_mem_name),
		},
		.c_reg = DT_INST_PROP(0, st_ctl_reg),
		.c_timing = DT_INST_PROP(0, st_ctl_timing),
		.c_map = DT_INST_PROP(0,  st_ctl_map),
		.c_perf = DT_INST_PROP(0, st_ctl_perf),
	};

	struct stm32mp_ddr_priv drv_data = {
		.info = {
			.base = DDR_MEM_BASE,
			.size = DDR_MAX_SIZE,
		},
		.ctl = (struct stm32mp_ddrctl *)DT_INST_REG_ADDR_BY_NAME(0, ctrl),
		.phy = (struct stm32mp_ddrphy *)DT_INST_REG_ADDR_BY_NAME(0, phy),
		.pwr = DT_REG_ADDR(DT_NODELABEL(pwr)),
		.rcc = DT_REG_ADDR(DT_NODELABEL(rcc)),
	};

	uint32_t basic[DT_INST_PROP_LEN(0, st_phy_basic)] = DT_INST_PROP(0, st_phy_basic);
	uint32_t advanced[DT_INST_PROP_LEN(0, st_phy_advanced)] = DT_INST_PROP(0, st_phy_advanced);
	uint32_t mr[DT_INST_PROP_LEN(0, st_phy_mr)] = DT_INST_PROP(0, st_phy_mr);
	uint32_t swizzle[DT_INST_PROP_LEN(0, st_phy_swizzle)] = DT_INST_PROP(0, st_phy_swizzle);

	/* these structures are composed of int and int[] */
	memcpy((void *)&userinputbasic, basic, sizeof(userinputbasic));
	memcpy((void *)&userinputadvanced, advanced, sizeof(userinputadvanced));
	memcpy((void *)&userinputmoderegister, mr, sizeof(userinputmoderegister));
	memcpy((void *)&userinputswizzle, swizzle, sizeof(userinputswizzle));

	drv_cfg.self_refresh = false;

/*        if (stm32mp_is_wakeup_from_standby()) {*/
/*                drv_cfg.self_refresh = true;*/
/*        }*/

	clk = clk_get(DDR_CLK_DEV, (clk_subsys_t)DDR_CLK_ID);
	if (!clk)
		return -ENODEV;

	ret = clk_enable(clk);
	if (ret)
		return ret;

	stm32mp2_ddr_init(&drv_data, &drv_cfg);

	if (drv_cfg.self_refresh) {
		ret = stm32mp_ddr_test_rw_access(&drv_data.info);
		if (ret != 0UL) {
			DDR_ERROR("DDR rw test: Can't access memory @ %#lx\n", ret);
			panic();
		}

		/* TODO Restore area overwritten by training */
		//stm32_restore_ddr_training_area();
	} else {
		size_t retsize;

		ret = stm32mp_ddr_test_data_bus(&drv_data.info);
		if (ret != 0UL) {
			DDR_ERROR("DDR data bus test: can't access memory @ %#lx\n", ret);
			panic();
		}

		ret = stm32mp_ddr_test_addr_bus(&drv_data.info);
		if (ret != 0UL) {
			DDR_ERROR("DDR addr bus test: can't access memory @ %#lx\n", ret);
			panic();
		}

		retsize = stm32mp_ddr_check_size(&drv_data.info, drv_cfg.info.size);
		if (retsize < drv_cfg.info.size) {
			DDR_ERROR("DDR size: %#x does not match DT config: %#x\n",
			      retsize, drv_cfg.info.size);
			panic();
		}

		DDR_INFO("Memory size = %#x (%u MB)\n", retsize, retsize / (1024U * 1024U));
	}

	/*
	 * Initialization sequence has configured DDR registers with settings.
	 * The Self Refresh (SR) mode corresponding to these settings has now
	 * to be set.
	 */
	ddr_set_sr_mode(ddr_read_sr_mode());

	return 0;
}

#define CHECK_DT_VS_STRUCT(prop, struct_name) \
	(DT_INST_PROP_LEN(0, prop) == (sizeof(struct struct_name) / sizeof(uint32_t)))

BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_phy_basic, user_input_basic),
	     "st,phy-basic property not match with user_input_basic size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_phy_advanced, user_input_advanced),
	     "st,phy-advanced property not match with user_input_advanced size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_phy_mr, user_input_mode_register),
	     "st,phy-mr property not match with user_input_mode_register size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_phy_swizzle, user_input_swizzle),
	     "st,phy-swizzle property not match with user_input_swizzle size");

BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_ctl_reg, stm32mp2_ddrctrl_reg),
	     "st,ctl-reg property not match with stm32mp2_ddrctrl_reg size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_ctl_timing, stm32mp2_ddrctrl_timing),
	     "st,ctl-timing property not match with stm32mp2_ddrctrl_timing size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_ctl_map, stm32mp2_ddrctrl_map),
	     "st,ctl-map property not match with stm32mp2_ddrctrl_map size");
BUILD_ASSERT(CHECK_DT_VS_STRUCT(st_ctl_perf, stm32mp2_ddrctrl_perf),
	     "st,ctl-perf property not match with stm32mp2_ddrctrl_perf size");

SYS_INIT(stm32mp2_ddr_dt_init, CORE, 20);
