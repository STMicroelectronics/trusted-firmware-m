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
#include <tfm_hal_isolation.h>

#include <mpu_armv8m_drv.h>

#define MPU_REGION_VENEERS           0
#define MPU_REGION_TFM_UNPRIV_CODE   1
#define PARTITION_REGION_RO          3
#define PARTITION_REGION_RW_STACK    4
#define PARTITION_REGION_PERIPH      5
#define PARTITION_REGION_SHARE       6

#if TFM_LVL == 2
#define MPU_REGION_NS_STACK          2
#elif TFM_LVL == 3
#define MPU_REGION_NS_DATA           2
#endif

#ifdef TFM_SP_META_PTR_ENABLE
#define MPU_REGION_SP_META_PTR       7
#endif /* TFM_SP_META_PTR_ENABLE */

REGION_DECLARE(Load$$LR$$, LR_VENEER, $$Base);
REGION_DECLARE(Load$$LR$$, LR_VENEER, $$Limit);
REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit);
#ifdef TFM_SP_META_PTR_ENABLE
REGION_DECLARE(Image$$, TFM_SP_META_PTR, $$RW$$Base);
REGION_DECLARE(Image$$, TFM_SP_META_PTR, $$RW$$Limit);
#endif
#ifndef TFM_PSA_API
REGION_DECLARE(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Limit);
#endif
#if TFM_LVL == 2
REGION_DECLARE(Image$$, TFM_APP_CODE_START, $$Base);
REGION_DECLARE(Image$$, TFM_APP_CODE_END, $$Base);
REGION_DECLARE(Image$$, TFM_APP_RW_STACK_START, $$Base);
REGION_DECLARE(Image$$, TFM_APP_RW_STACK_END, $$Base);
REGION_DECLARE(Image$$, ARM_LIB_STACK, $$ZI$$Base);
REGION_DECLARE(Image$$, ARM_LIB_STACK, $$ZI$$Limit);
#endif

static const struct mpu_armv8m_region_cfg_t __maybe_unused mpu_regions[] = {
    /* Veneer region */
    {
        MPU_REGION_VENEERS,
        (uint32_t)&REGION_NAME(Load$$LR$$, LR_VENEER, $$Base),
        (uint32_t)&REGION_NAME(Load$$LR$$, LR_VENEER, $$Limit),
        MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
        MPU_ARMV8M_XN_EXEC_OK,
        MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
    /* TFM Core unprivileged code region */
    {
        MPU_REGION_TFM_UNPRIV_CODE,
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Base),
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit),
        MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
        MPU_ARMV8M_XN_EXEC_OK,
        MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
#ifdef TFM_SP_META_PTR_ENABLE
    /* TFM partition metadata pointer region */
    {
        MPU_REGION_SP_META_PTR,
        (uint32_t)&REGION_NAME(Image$$, TFM_SP_META_PTR, $$RW$$Base),
        (uint32_t)&REGION_NAME(Image$$, TFM_SP_META_PTR, $$ZI$$Limit),
        MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
        MPU_ARMV8M_XN_EXEC_NEVER,
        MPU_ARMV8M_AP_RW_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
#endif
#if TFM_LVL == 2
    {
        MPU_REGION_NS_STACK,
        (uint32_t)&REGION_NAME(Image$$, ARM_LIB_STACK, $$ZI$$Base),
        (uint32_t)&REGION_NAME(Image$$, ARM_LIB_STACK, $$ZI$$Limit),
        MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
        MPU_ARMV8M_XN_EXEC_NEVER,
        MPU_ARMV8M_AP_RW_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
    /* RO region */
    {
        PARTITION_REGION_RO,
        (uint32_t)&REGION_NAME(Image$$, TFM_APP_CODE_START, $$Base),
        (uint32_t)&REGION_NAME(Image$$, TFM_APP_CODE_END, $$Base),
        MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
        MPU_ARMV8M_XN_EXEC_OK,
        MPU_ARMV8M_AP_RO_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
    /* RW, ZI and stack as one region */
    {
        PARTITION_REGION_RW_STACK,
        (uint32_t)&REGION_NAME(Image$$, TFM_APP_RW_STACK_START, $$Base),
        (uint32_t)&REGION_NAME(Image$$, TFM_APP_RW_STACK_END, $$Base),
        MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
        MPU_ARMV8M_XN_EXEC_NEVER,
        MPU_ARMV8M_AP_RW_PRIV_UNPRIV,
        MPU_ARMV8M_SH_NONE
    },
#endif
};

enum tfm_hal_status_t __maybe_unused tfm_hal_mpu_init(void)
{
	struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };
	struct mpu_armv8m_region_cfg_t *rg;
	int32_t i;

	mpu_armv8m_clean(&dev_mpu_s);

	for_each_mpu_region(mpu_regions, rg, ARRAY_SIZE(mpu_regions), i) {
		if (mpu_armv8m_region_enable(&dev_mpu_s, rg) != MPU_ARMV8M_OK)
			return TFM_HAL_ERROR_GENERIC;
	}

	mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
			HARDFAULT_NMI_ENABLE);

	return 0;
}

enum tfm_hal_status_t tfm_hal_set_up_static_boundaries(void)
{
    /* Set up isolation boundaries between SPE and NSPE */
	sau_and_idau_cfg();

    /* Set up static isolation boundaries inside SPE */
#ifdef CONFIG_TFM_ENABLE_MEMORY_PROTECT
    if (tfm_hal_mpu_init())
        return TFM_HAL_ERROR_GENERIC;
#endif

	return TFM_HAL_SUCCESS;
}
