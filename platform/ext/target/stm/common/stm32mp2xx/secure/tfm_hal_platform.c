/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <device.h>
#include <psa_manifest/pid.h>
#include <tfm_hal_platform.h>
#include <tfm_plat_defs.h>
#include <target_cfg.h>
#include <region.h>
#include <init.h>
#include <lib/mmio.h>

#include <uart_stdout.h>
#include <stm32_dcache.h>
#include <debug.h>
#include <stm_version.h>

enum tfm_hal_status_t tfm_hal_platform_init(void)
{
	enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;

	plat_err = system_reset_cfg();
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_HAL_ERROR_GENERIC;
	}

	sys_init_run_level(INIT_LEVEL_PRE_CORE);

	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		if (stm32_dcache_enable(true, true))
			return TFM_HAL_ERROR_GENERIC;
	}

	sys_init_run_level(INIT_LEVEL_CORE);

	stdio_init();

	INFO("welcome to TF-M: "MODEL_VERSION"\n");
	INFO("board: "MODEL_BOARD"\n");
	INFO("dts: "MODEL_S_DTS"\n");

	sys_init_run_level(INIT_LEVEL_POST_CORE);
	sys_init_run_level(INIT_LEVEL_REST);

	/*
	 * Enable CP10/CP11 (FPU coprocessors) in Secure state and grant NS access.
	 *
	 * Even though TF-M Secure code never uses the FPU (-mfloat-abi=soft),
	 * Secure access to CP10/CP11 MUST be enabled before any write to FPCCR,
	 * because writes to FPCCR are ignored/unpredictable when CP10=0 in CPACR_S.
	 *
	 * Additionally, if NS code uses the FPU and a NS→S transition occurs while
	 * CONTROL_NS.FPCA=1, a NOCP UsageFault is triggered unless Secure CP10/CP11
	 * access is enabled first (ARM TRM note, also in tfm_arch_v8m_main.c).
	 *
	 * NS access via NSACR allows the NS application to use the FPU freely
	 * (NS CPACR_NS configuration is left to NS software).
	 */
	#define SCB_CPACR_CP10_Msk (3U << 20U)
	#define SCB_CPACR_CP11_Msk (3U << 22U)

	SCB->CPACR |= SCB_CPACR_CP10_Msk |  SCB_CPACR_CP11_Msk; /* CP10+CP11 full access (S) */
	SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk; /* allow NS access */
	__DSB();
	__ISB();

	/*
	 * Clear FPCCR.TS (Treat-as-Secure, bit 26).
	 *
	 * Reset value of FPCCR.TS = 1, which causes the processor to save NS
	 * FP callee registers (S16-S31 + FPSCR) on MSP_S during every NS→Secure
	 * exception transition, producing a 28-word callee-save frame instead of
	 * 10 words (signature 0xFEF5EDA5 instead of 0xFEFA125B).
	 *
	 * Since TF-M Secure code never uses the FPU (-mcpu=cortex-m33+nofp,
	 * -mfloat-abi=soft, CONFIG_TFM_ENABLE_FP=OFF), there is no reason to
	 * protect NS FP context from Secure reads.  Clearing TS means NS FP
	 * registers remain on the NS side, reducing every NS→S exception
	 * stacking by 72 bytes (18 words: S16-S31 + FPSCR + reserved).
	 *
	 * This write requires CP10 to be enabled in CPACR_S (done above).
	 */
	FPU->FPCCR &= ~FPU_FPCCR_TS_Msk;
	__DSB();
	if (FPU->FPCCR & FPU_FPCCR_TS_Msk)
		return TFM_HAL_ERROR_BAD_STATE;

	return TFM_HAL_SUCCESS;
}

/* Get address of non secure code start */
REGION_DECLARE(Load$$LR$$, LR_NS_PARTITION, $$Base);
#define NS_SECURE_BASE (uint32_t)&REGION_NAME(Load$$LR$$, LR_NS_PARTITION, $$Base)

uint32_t tfm_hal_get_ns_VTOR(void)
{
    return NS_SECURE_BASE;
}

uint32_t tfm_hal_get_ns_MSP(void)
{
    return *((uint32_t *)NS_SECURE_BASE);
}

uint32_t tfm_hal_get_ns_entry_point(void)
{
    return *((uint32_t *)(NS_SECURE_BASE + 4));
}

uint32_t tfm_hal_raise_notify_ns(void)
{
	TZ_NVIC_SetPendingIRQ_NS(RESERVED_9);
	return TFM_HAL_SUCCESS;
}

uint32_t tfm_hal_notify_ns_init(void)
{
	return TFM_HAL_SUCCESS;
}

uint32_t tfm_hal_check_boot_data_access_policy(int32_t partition_id)
{
#ifdef TFM_PARTITION_PM
	if (partition_id == TFM_SP_PM)
		return TFM_HAL_SUCCESS;
#endif

	return TFM_HAL_ERROR_GENERIC;
}

/* Resets the system with RCC when TDCID or reset only cortex M33. */
#define RCC_GRSTCSETR		0x400
#define _RCC_GRSTCSETR_SYSRST	BIT(0)
void tfm_hal_system_reset(void)
{
	__disable_irq();

	if (IS_ENABLED(STM32_M33TDCID)) {
		uintptr_t base = DT_REG_ADDR(DT_NODELABEL(rcc));

		IMSG("System reset\n");
		io_write32(base + RCC_GRSTCSETR, _RCC_GRSTCSETR_SYSRST);
	} else {
		IMSG("Cortex-M33 reset\n");
		NVIC_SystemReset();
	}

	/* The reset is not immediate, never give the hand back to the caller */
	while (1) {
		__WFE();
	}
}

void tfm_hal_system_halt(void)
{
	/*
	 * Disable IRQs to stop all threads, not just the thread that
	 * halted the system.
	 */
	__disable_irq();

	/*
	 * Enter sleep to reduce power consumption and do it in a loop in
	 * case a signal wakes up the CPU.
	 */
	while (1) {
		__WFE();
	}
}
