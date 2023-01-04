/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <lib/utils_def.h>
#include <region.h>
#include <target_cfg.h>
#include <device_cfg.h>
#include <string.h>
#include <tfm_hal_isolation.h>
#include <mmio_defs.h>

#include <mpu_armv8m_drv.h>
#include <sau_armv8m_drv.h>
#include <stm32_icache.h>

#define PROT_BOUNDARY_VAL \
    ((1U << HANDLE_ATTR_PRIV_POS) & HANDLE_ATTR_PRIV_MASK)

/* It can be retrieved from the MPU_TYPE register. */
#define MPU_REGION_NUM                  8
#ifdef CONFIG_TFM_ENABLE_MEMORY_PROTECT
static uint32_t n_configured_regions = 0;

REGION_DECLARE(Image$$, ER_VENEER, $$Base);
REGION_DECLARE(Image$$, VENEER_ALIGN, $$Limit);
REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit);
REGION_DECLARE(Image$$, TFM_APP_CODE_START, $$Base);
REGION_DECLARE(Image$$, TFM_APP_CODE_END, $$Base);
REGION_DECLARE(Image$$, TFM_APP_RW_STACK_START, $$Base);
REGION_DECLARE(Image$$, TFM_APP_RW_STACK_END, $$Base);
#ifdef TFM_SP_META_PTR_ENABLE
REGION_DECLARE(Image$$, TFM_SP_META_PTR, $$RW$$Base);
REGION_DECLARE(Image$$, TFM_SP_META_PTR, $$RW$$Limit);
#endif

static const struct mpu_armv8m_region_cfg_t __maybe_unused mpu_regions[] = {
	/* Veneer region */
	{
		0, /* will be updated before using */
		(uint32_t)&REGION_NAME(Image$$, ER_VENEER, $$Base),
		(uint32_t)&REGION_NAME(Image$$, VENEER_ALIGN, $$Limit),
		MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
		MPU_ARMV8M_XN_EXEC_OK,
		MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
		MPU_ARMV8M_SH_NONE
	},
	/* TFM Core unprivileged code region */
	{
		0, /* will be updated before using */
		(uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Base),
		(uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit),
		MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
		MPU_ARMV8M_XN_EXEC_OK,
		MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
		MPU_ARMV8M_SH_NONE
	},
	/* RO region */
	{
		0, /* will be updated before using */
		(uint32_t)&REGION_NAME(Image$$, TFM_APP_CODE_START, $$Base),
		(uint32_t)&REGION_NAME(Image$$, TFM_APP_CODE_END, $$Base),
		MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
		MPU_ARMV8M_XN_EXEC_OK,
		MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
		MPU_ARMV8M_SH_NONE
	},
	/* RW, ZI and stack as one region */
	{
		0, /* will be updated before using */
		(uint32_t)&REGION_NAME(Image$$, TFM_APP_RW_STACK_START, $$Base),
		(uint32_t)&REGION_NAME(Image$$, TFM_APP_RW_STACK_END, $$Base),
		MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
		MPU_ARMV8M_XN_EXEC_NEVER,
		MPU_ARMV8M_AP_RW_PRIV_UNPRIV,
		MPU_ARMV8M_SH_NONE
	},
#ifdef TFM_SP_META_PTR_ENABLE
	/* TFM partition metadata pointer region */
	{
		0, /* will be updated before using */
		(uint32_t)&REGION_NAME(Image$$, TFM_SP_META_PTR, $$RW$$Base),
		(uint32_t)&REGION_NAME(Image$$, TFM_SP_META_PTR, $$ZI$$Limit),
		MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
		MPU_ARMV8M_XN_EXEC_NEVER,
		MPU_ARMV8M_AP_RW_PRIV_UNPRIV,
		MPU_ARMV8M_SH_NONE
	},
#endif
#if defined(STM32_PS_OSPI)
	/*
	 * used by ospi
	 * for memory mapping to generate external memory command
	 */
	{
		0, /* will be updated before using */
		OSPI1_MEM_BASE,
		OSPI1_MEM_BASE + SPI_NOR_FLASH_SIZE - 1,
		MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX,
		MPU_ARMV8M_XN_EXEC_NEVER,
		MPU_ARMV8M_AP_RW_PRIV_ONLY,
		MPU_ARMV8M_SH_NONE
	},
#endif
};

enum tfm_hal_status_t __maybe_unused tfm_hal_mpu_init(void)
{
	struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };
	struct mpu_armv8m_region_cfg_t *rg, localcfg;
	int32_t i;

	mpu_armv8m_clean(&dev_mpu_s);

	for_each_mpu_region(mpu_regions, rg, ARRAY_SIZE(mpu_regions), i) {
		memcpy(&localcfg, rg, sizeof(localcfg));
		localcfg.region_nr = i;

		if (mpu_armv8m_region_enable(&dev_mpu_s, &localcfg) != MPU_ARMV8M_OK)
			return TFM_HAL_ERROR_GENERIC;
	}

	n_configured_regions = i;

	mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
			  HARDFAULT_NMI_ENABLE);

	return 0;
}
#endif /* CONFIG_TFM_ENABLE_MEMORY_PROTECT */

enum tfm_hal_status_t tfm_hal_set_up_static_boundaries(uintptr_t *p_spm_boundary)
{
	/* Set up isolation boundaries between SPE and NSPE */
	sau_init();

	/* Set up static isolation boundaries inside SPE */
#ifdef CONFIG_TFM_ENABLE_MEMORY_PROTECT
	if (tfm_hal_mpu_init())
		return TFM_HAL_ERROR_GENERIC;
#endif

	*p_spm_boundary = (uintptr_t)PROT_BOUNDARY_VAL;

	return TFM_HAL_SUCCESS;
}

/*
 * Implementation of tfm_hal_bind_boundary() on STM:
 *
 * The API encodes some attributes into a handle and returns it to SPM.
 * The attributes include isolation boundaries, privilege, and MMIO information.
 * When scheduler switches running partitions, SPM compares the handle between
 * partitions to know if boundary update is necessary. If update is required,
 * SPM passes the handle to platform to do platform settings and update
 * isolation boundaries.
 *
 * The handle should be unique under isolation level 3. The implementation
 * encodes an index at the highest 8 bits to assure handle uniqueness. While
 * under isolation level 1/2, handles may not be unique.
 *
 * The encoding format assignment:
 * - For isolation level 3
 *      BIT | 31        24 | 23         20 | ... | 7           4 | 3        0 |
 *          | Unique Index | Region Attr 5 | ... | Region Attr 1 | Privileged |
 *
 *      In which the "Region Attr i" is:
 *      BIT |       3      | 2        0 |
 *          | 1: RW, 0: RO | MMIO Index |
 *
 * - For isolation level 1/2
 *      BIT | 31                           0 |
 *          | 1: privileged, 0: unprivileged |
 *
 * This is a reference implementation on STM, and may have some limitations.
 * 1. The maximum number of allowed MMIO regions is 5.
 * 2. Highest 8 bits are for index. It supports 256 unique handles at most.
 */
/*
 * TODO: boundary service in partitions is not yet implemented
 * */
enum tfm_hal_status_t tfm_hal_bind_boundary(const struct partition_load_info_t *p_ldinf,
					    uintptr_t *p_boundary)
{
	return TFM_HAL_SUCCESS;
}

enum tfm_hal_status_t tfm_hal_activate_boundary(const struct partition_load_info_t *p_ldinf,
						uintptr_t boundary)
{
	return TFM_HAL_SUCCESS;
}

enum tfm_hal_status_t tfm_hal_memory_check(uintptr_t boundary, uintptr_t base,
                                           size_t size, uint32_t access_type)
{
	return TFM_HAL_SUCCESS;
}

bool tfm_hal_boundary_need_switch(uintptr_t boundary_from,
                                  uintptr_t boundary_to)
{
    if (boundary_from == boundary_to) {
        return false;
    }

    if (((uint32_t)boundary_from & HANDLE_ATTR_PRIV_MASK) &&
        ((uint32_t)boundary_to & HANDLE_ATTR_PRIV_MASK)) {
        return false;
    }
    return true;
}
