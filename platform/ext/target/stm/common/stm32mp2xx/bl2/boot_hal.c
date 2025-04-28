/*
 * Copyright (C) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
/*
 * Allow to overwrite functions defined in platform/ext/common/boot_hal.c
 * for specific needs of stm32mp2
 */
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <boot_hal.h>
#include <cmsis.h>
#include <device.h>
#include <region.h>
#include <region_defs.h>
#include <Driver_Flash.h>
#include <bootutil/bootutil_log.h>
#include <init.h>
#include <debug.h>
#include <partition.h>
#include "flash_map/flash_map.h"

#include <stm32mp2_ddr.h>
#include <stm32_bsec3.h>
#include <stm32_dcache.h>
#include <stm32_tamp.h>
#include <stm_version.h>

#ifdef CRYPTO_HW_ACCELERATOR
#include "crypto_hw.h"
#endif /* CRYPTO_HW_ACCELERATOR */

extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_1;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_3;
#if (MCUBOOT_IMAGE_NUMBER == 3)
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_4;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_5;
#endif /* (MCUBOOT_IMAGE_NUMBER == 3) */

#define STM32_SOC_NAME_SIZE		24

#define STM32MP211A_PART_NB		U(0x40073E7D)
#define STM32MP211C_PART_NB		U(0x0007307D)
#define STM32MP211D_PART_NB		U(0xC0073E7D)
#define STM32MP211F_PART_NB		U(0x8007307D)
#define STM32MP213A_PART_NB		U(0x40073E1D)
#define STM32MP213C_PART_NB		U(0x0007301D)
#define STM32MP213D_PART_NB		U(0xC0073E1D)
#define STM32MP213F_PART_NB		U(0x8007301D)
#define STM32MP215A_PART_NB		U(0x40033E0D)
#define STM32MP215C_PART_NB		U(0x0003300D)
#define STM32MP215D_PART_NB		U(0xC0033E0D)
#define STM32MP215F_PART_NB		U(0x8003300D)

#define STM32MP231A_PART_NB		U(0x400B3FEF)
#define STM32MP231C_PART_NB		U(0x000B31EF)
#define STM32MP231D_PART_NB		U(0xC00B3FEF)
#define STM32MP231F_PART_NB		U(0x800B31EF)
#define STM32MP233A_PART_NB		U(0x400B3F8E)
#define STM32MP233C_PART_NB		U(0x000B318E)
#define STM32MP233D_PART_NB		U(0xC00B3F8E)
#define STM32MP233F_PART_NB		U(0x800B318E)
#define STM32MP235A_PART_NB		U(0x40082F82)
#define STM32MP235C_PART_NB		U(0x00082182)
#define STM32MP235D_PART_NB		U(0xC0082F82)
#define STM32MP235F_PART_NB		U(0x80082182)

#define STM32MP251A_PART_NB		U(0x400B3E6D)
#define STM32MP251C_PART_NB		U(0x000B306D)
#define STM32MP251D_PART_NB		U(0xC00B3E6D)
#define STM32MP251F_PART_NB		U(0x800B306D)
#define STM32MP253A_PART_NB		U(0x400B3E0C)
#define STM32MP253C_PART_NB		U(0x000B300C)
#define STM32MP253D_PART_NB		U(0xC00B3E0C)
#define STM32MP253F_PART_NB		U(0x800B300C)
#define STM32MP255A_PART_NB		U(0x40082E00)
#define STM32MP255C_PART_NB		U(0x00082000)
#define STM32MP255D_PART_NB		U(0xC0082E00)
#define STM32MP255F_PART_NB		U(0x80082000)
#define STM32MP257A_PART_NB		U(0x40002E00)
#define STM32MP257C_PART_NB		U(0x00002000)
#define STM32MP257D_PART_NB		U(0xC0002E00)
#define STM32MP257F_PART_NB		U(0x80002000)

#define STM32MP2_REV_A			U(0x08)
#define STM32MP2_REV_B			U(0x10)
#define STM32MP2_REV_X			U(0x12)
#define STM32MP2_REV_Y			U(0x11)
#define STM32MP2_REV_Z			U(0x09)

#if defined(STM32MP21xxxx)
#define STM32MP21_PKG_CUSTOM		U(0)
#define STM32MP21_PKG_AL_VFBGA361	U(1)
#define STM32MP21_PKG_AN_VFBGA273	U(3)
#define STM32MP21_PKG_AO_VFBGA225	U(4)
#define STM32MP21_PKG_AM_TFBGA289	U(5)
#elif defined(STM32MP23xxxx)
#define STM32MP23_PKG_CUSTOM		U(0)
#define STM32MP23_PKG_AL_VFBGA361	U(1)
#define STM32MP23_PKG_AK_VFBGA424	U(3)
#define STM32MP23_PKG_AJ_TFBGA361	U(7)
#else /* STM32MP25xxxx */
#define STM32MP25_PKG_CUSTOM		U(0)
#define STM32MP25_PKG_AL_VFBGA361	U(1)
#define STM32MP25_PKG_AK_VFBGA424	U(3)
#define STM32MP25_PKG_AI_TFBGA436	U(5)
#endif /* STM32MP21xxxx */

static enum tfm_plat_err_t get_chip_info(char name[STM32_SOC_NAME_SIZE])
{
	char *cpu_s, *cpu_r, *pkg;
	enum tfm_plat_err_t ret;
	uint32_t otp_val;

	memset(name, 0, STM32_SOC_NAME_SIZE);

	/* MPUs Part Numbers */
	ret = tfm_plat_otp_read(PLAT_OTP_ID_RPN,
				sizeof(uint32_t), (uint8_t *)&otp_val);
	if (ret != TFM_PLAT_ERR_SUCCESS)
		return ret;

	switch (otp_val) {
#if defined(STM32MP21xxxx)
	case STM32MP211A_PART_NB:
		cpu_s = "211A";
		break;
	case STM32MP211C_PART_NB:
		cpu_s = "211C";
		break;
	case STM32MP211D_PART_NB:
		cpu_s = "211D";
		break;
	case STM32MP211F_PART_NB:
		cpu_s = "211F";
		break;
	case STM32MP213A_PART_NB:
		cpu_s = "213A";
		break;
	case STM32MP213C_PART_NB:
		cpu_s = "213C";
		break;
	case STM32MP213D_PART_NB:
		cpu_s = "213D";
		break;
	case STM32MP213F_PART_NB:
		cpu_s = "213F";
		break;
	case STM32MP215A_PART_NB:
		cpu_s = "215A";
		break;
	case STM32MP215C_PART_NB:
		cpu_s = "215C";
		break;
	case STM32MP215D_PART_NB:
		cpu_s = "215D";
		break;
	case STM32MP215F_PART_NB:
		cpu_s = "215F";
		break;
#elif defined(STM32MP23xxxx)
	case STM32MP231A_PART_NB:
		cpu_s = "231A";
		break;
	case STM32MP231C_PART_NB:
		cpu_s = "231C";
		break;
	case STM32MP231D_PART_NB:
		cpu_s = "231D";
		break;
	case STM32MP231F_PART_NB:
		cpu_s = "231F";
		break;
	case STM32MP233A_PART_NB:
		cpu_s = "233A";
		break;
	case STM32MP233C_PART_NB:
		cpu_s = "233C";
		break;
	case STM32MP233D_PART_NB:
		cpu_s = "233D";
		break;
	case STM32MP233F_PART_NB:
		cpu_s = "233F";
		break;
	case STM32MP235A_PART_NB:
		cpu_s = "235A";
		break;
	case STM32MP235C_PART_NB:
		cpu_s = "235C";
		break;
	case STM32MP235D_PART_NB:
		cpu_s = "235D";
		break;
	case STM32MP235F_PART_NB:
		cpu_s = "235F";
		break;
#else /* STM32MP25xxxx */
	case STM32MP251A_PART_NB:
		cpu_s = "251A";
		break;
	case STM32MP251C_PART_NB:
		cpu_s = "251C";
		break;
	case STM32MP251D_PART_NB:
		cpu_s = "251D";
		break;
	case STM32MP251F_PART_NB:
		cpu_s = "251F";
		break;
	case STM32MP253A_PART_NB:
		cpu_s = "253A";
		break;
	case STM32MP253C_PART_NB:
		cpu_s = "253C";
		break;
	case STM32MP253D_PART_NB:
		cpu_s = "253D";
		break;
	case STM32MP253F_PART_NB:
		cpu_s = "253F";
		break;
	case STM32MP255A_PART_NB:
		cpu_s = "255A";
		break;
	case STM32MP255C_PART_NB:
		cpu_s = "255C";
		break;
	case STM32MP255D_PART_NB:
		cpu_s = "255D";
		break;
	case STM32MP255F_PART_NB:
		cpu_s = "255F";
		break;
	case STM32MP257A_PART_NB:
		cpu_s = "257A";
		break;
	case STM32MP257C_PART_NB:
		cpu_s = "257C";
		break;
	case STM32MP257D_PART_NB:
		cpu_s = "257D";
		break;
	case STM32MP257F_PART_NB:
		cpu_s = "257F";
		break;
#endif /* STM32MP21xxxx */
	default:
		cpu_s = "????";
		break;
	}

	/* Package */
	ret = tfm_plat_otp_read(PLAT_OTP_ID_PACKAGE,
				sizeof(uint32_t), (uint8_t *)&otp_val);
	if (ret != TFM_PLAT_ERR_SUCCESS)
		return ret;

	switch (otp_val & GENMASK(2, 0)) {
#if defined(STM32MP21xxxx)
	case STM32MP21_PKG_CUSTOM:
		pkg = "XX";
		break;
	case STM32MP21_PKG_AL_VFBGA361:
		pkg = "AL";
		break;
	case STM32MP21_PKG_AN_VFBGA273:
		pkg = "AN";
		break;
	case STM32MP21_PKG_AO_VFBGA225:
		pkg = "AO";
		break;
	case STM32MP21_PKG_AM_TFBGA289:
		pkg = "AM";
		break;
#elif defined(STM32MP23xxxx)
	case STM32MP23_PKG_CUSTOM:
		pkg = "XX";
		break;
	case STM32MP23_PKG_AJ_TFBGA361:
		pkg = "AJ";
		break;
	case STM32MP23_PKG_AK_VFBGA424:
		pkg = "AK";
		break;
	case STM32MP23_PKG_AL_VFBGA361:
		pkg = "AL";
		break;
#else /* STM32MP25xxxx */
	case STM32MP25_PKG_CUSTOM:
		pkg = "XX";
		break;
	case STM32MP25_PKG_AL_VFBGA361:
		pkg = "AL";
		break;
	case STM32MP25_PKG_AK_VFBGA424:
		pkg = "AK";
		break;
	case STM32MP25_PKG_AI_TFBGA436:
		pkg = "AI";
		break;
#endif /* STM32MP21xxxx */
	default:
		pkg = "??";
		break;
	}

	/* REVISION */
	ret = tfm_plat_otp_read(PLAT_OTP_ID_REV_ID,
				sizeof(uint32_t), (uint8_t *)&otp_val);
	if (ret != TFM_PLAT_ERR_SUCCESS)
		return ret;

	switch (otp_val) {
	case STM32MP2_REV_A:
		cpu_r = "A";
		break;
	case STM32MP2_REV_B:
		cpu_r = "B";
		break;
	case STM32MP2_REV_X:
		cpu_r = "X";
		break;
	case STM32MP2_REV_Y:
		cpu_r = "Y";
		break;
	case STM32MP2_REV_Z:
		cpu_r = "Z";
		break;
	default:
		cpu_r = "?";
		break;
	}

	snprintf(name, STM32_SOC_NAME_SIZE,
		 "STM32MP%s%s Rev.%s", cpu_s, pkg, cpu_r);

	return TFM_PLAT_ERR_SUCCESS;
}

int stm32mp2_init_debug(void)
{
#if defined(DAUTH_NONE)
#elif defined(DAUTH_NS_ONLY)
#elif defined(DAUTH_FULL)
	BOOT_LOG_WRN("\033[1;31m*******************************\033[0m");
	BOOT_LOG_WRN("\033[1;31m* The debug port is full open *\033[0m");
	BOOT_LOG_WRN("\033[1;31m* wait debugger interrupt     *\033[0m");
	BOOT_LOG_WRN("\033[1;31m*******************************\033[0m");
	stm32_bsec_write_debug_conf(DBG_FULL);
        __WFI();
#else
#if !defined(DAUTH_CHIP_DEFAULT)
#error "No debug authentication setting is provided."
#endif
#endif
	return 0;
}
SYS_INIT(stm32mp2_init_debug, CORE, 11);

#if defined(STM32_BOOT_DEV_SDMMC1) || defined(STM32_BOOT_DEV_SDMMC2)
extern struct flash_area flash_map[];

static int stm32mp2_prepare_fw(void)
{
	const partition_entry_t *tfm_entry;

        tfm_entry = get_partition_entry("m33fw-a");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm primary partition");
		return -EINVAL;
	}

	flash_map[0].fa_off = tfm_entry->start;
	flash_map[0].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33fw-b");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm secondary partition");
		return -EINVAL;
	}

	flash_map[1].fa_off = tfm_entry->start;
	flash_map[1].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33ddr-a");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition ddr fw primary partition");
		return -EINVAL;
	}

	flash_map[2].fa_off = tfm_entry->start;
	flash_map[2].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33ddr-b");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition ddr fw secondary partition");
		return -EINVAL;
	}

	flash_map[3].fa_off = tfm_entry->start;
	flash_map[3].fa_size = tfm_entry->length;

	return 0;
}
SYS_INIT(stm32mp2_prepare_fw, CORE, 15);

#if (DT_NODE_EXISTS(DT_NODELABEL(ca35_cube_fw))) && (MCUBOOT_IMAGE_NUMBER == 3)
static int stm32mp2_prep_a35_fw(void) {
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(tamp));
	const partition_entry_t *tfm_entry = NULL;
	int err = 0;

	tfm_entry = get_partition_entry("a35fw-a");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find cortex A35 firmware partition");
		return -EINVAL;
	}

	flash_map[4].fa_off = tfm_entry->start;
	flash_map[4].fa_size = tfm_entry->length;

	/* TODO: remove*/
	flash_map[5].fa_off = tfm_entry->start;
	flash_map[5].fa_size = tfm_entry->length;

	/* Fill the backup register 72 with the load address of the firmware */
	err = stm32_tamp_bkpreg_write(dev, 72, CA35_FW_DEST_ADDR + BL2_HEADER_SIZE);
	if (err)
		BOOT_LOG_ERR("Error while writing CA35 load address in backup register 72");

	return err;
}
SYS_INIT(stm32mp2_prep_a35_fw, CORE, 30);
#endif /* (DT_NODE_EXISTS(DT_NODELABEL(ca35_cube_fw))) */
#endif /* defined(STM32_BOOT_DEV_SDMMC1) || defined(STM32_BOOT_DEV_SDMMC2) */

/**
  * @brief  Platform init
  * @param  None
  * @retval status
  */
int32_t boot_platform_init(void)
{
	char name[STM32_SOC_NAME_SIZE];

	sys_init_run_level(INIT_LEVEL_PRE_CORE);

	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		int err;

		err = stm32_dcache_enable(true, true);
		if (err)
			return err;
	}

	sys_init_run_level(INIT_LEVEL_CORE);

	BOOT_LOG_INF("welcome to MCUboot: "MODEL_VERSION);

	if (get_chip_info(name) == TFM_PLAT_ERR_SUCCESS)
		BOOT_LOG_INF("cpu: %s", name);

	BOOT_LOG_INF("board: "MODEL_BOARD);
	BOOT_LOG_INF("dts: "MODEL_BL2_DTS);
	BOOT_LOG_INF("boot device: "MODEL_BOOT_DEV);
	BOOT_LOG_INF("mcu sysclk: %d", SystemCoreClock);

	return 0;
}

int32_t boot_platform_post_init(void)
{
	sys_init_run_level(INIT_LEVEL_POST_CORE);
	sys_init_run_level(INIT_LEVEL_REST);

	return 0;
}

#if defined(STM32_CACHE_ENABLED)
/* Override for cache operations */
void boot_platform_quit(struct boot_arm_vector_table *vt)
{
	/*
	 * Clang at O0, stores variables on the stack with SP relative addressing.
	 * When manually set the SP then the place of reset vector is lost.
	 * Static variables are stored in 'data' or 'bss' section, change of SP has
	 * no effect on them.
	 */
	static struct boot_arm_vector_table *vt_cpy;
	int32_t result;

	#ifdef CRYPTO_HW_ACCELERATOR
	result = crypto_hw_accelerator_finish();
	if (result) {
		while (1){}
	}
	#endif /* CRYPTO_HW_ACCELERATOR */

	#ifdef FLASH_DEV_NAME_0
	result = FLASH_DEV_NAME_0.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME */
	#ifdef FLASH_DEV_NAME_1
	result = FLASH_DEV_NAME_1.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME */
	#ifdef FLASH_DEV_NAME_2
	result = FLASH_DEV_NAME_2.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_2 */
	#ifdef FLASH_DEV_NAME_3
	result = FLASH_DEV_NAME_3.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_3 */
#if (MCUBOOT_IMAGE_NUMBER == 3)
	#ifdef FLASH_DEV_NAME_4
	result = FLASH_DEV_NAME_4.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_4 */
	#ifdef FLASH_DEV_NAME_5
	result = FLASH_DEV_NAME_5.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_5 */
#endif /* (MCUBOOT_IMAGE_NUMBER == 3)*/
	#ifdef FLASH_DEV_NAME_SCRATCH
	result = FLASH_DEV_NAME_SCRATCH.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_SCRATCH */

	vt_cpy = vt;
	#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
	|| defined(__ARM_ARCH_8_1M_MAIN__)
	/*
	 * Restore the Main Stack Pointer Limit register's reset value
	 * before passing execution to runtime firmware to make the
	 * bootloader transparent to it.
	 */
	__set_MSPLIM(0);
	#endif /* defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
	|| defined(__ARM_ARCH_8_1M_MAIN__) */

	/* Invalidate all the memory range accessible by the M-33 */
	if (stm32_dcache_clean(0x0, 0xFFFFFFFF))
		panic();
	if (stm32_dcache_full_inv())
		panic();
	if (stm32_dcache_disable())
		panic();

	__set_MSP(vt_cpy->msp);
	__DSB();
	__ISB();

	boot_jump_to_next_image(vt_cpy->reset);
}
#endif

int boot_platform_post_load(uint32_t image_id)
{
	if (image_id == DDR_FIRMWARE_ID) {
		BOOT_LOG_INF("BL2: image %d, enable DDR-FW", image_id);
		stm32mp2_ddr_dt_init();
	}
	return 0;
}
