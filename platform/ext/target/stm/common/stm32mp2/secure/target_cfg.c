/*
 * Copyright (C) 2020 STMicroelectronics.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cmsis.h>
#include <target_cfg.h>
#include <region.h>
#include <region_defs.h>
#include <tfm_secure_api.h>
#include <tfm_plat_defs.h>

#include <stm32_iac.h>
#include <stm32_serc.h>

/* The section names come from the scatter file */
REGION_DECLARE(Load$$LR$$, LR_NS_PARTITION, $$Base);
REGION_DECLARE(Load$$LR$$, LR_VENEER, $$Base);
REGION_DECLARE(Load$$LR$$, LR_VENEER, $$Limit);
REGION_DECLARE(Load$$LR$$, LR_SECONDARY_PARTITION, $$Base);

const struct memory_region_limits memory_regions = {
	.non_secure_code_start =
		(uint32_t)&REGION_NAME(Load$$LR$$, LR_NS_PARTITION, $$Base),

	.non_secure_partition_base =
		(uint32_t)&REGION_NAME(Load$$LR$$, LR_NS_PARTITION, $$Base),

	.non_secure_partition_limit =
		(uint32_t)&REGION_NAME(Load$$LR$$, LR_NS_PARTITION, $$Base) +
		NS_PARTITION_SIZE - 1,

	.veneer_base =
		(uint32_t)&REGION_NAME(Load$$LR$$, LR_VENEER, $$Base),

	.veneer_limit =
		(uint32_t)&REGION_NAME(Load$$LR$$, LR_VENEER, $$Limit),
};

/* Define Peripherals NS address range for the platform */
#define PERIPHERALS_BASE_NS_START (0x40000000)
#define PERIPHERALS_BASE_NS_END   (0x4FFFFFFF)

/* To write into AIRCR register, 0x5FA value must be write to the VECTKEY field,
 * otherwise the processor ignores the write.
 */
#define SCB_AIRCR_WRITE_MASK ((0x5FAUL << SCB_AIRCR_VECTKEY_Pos))

enum tfm_plat_err_t enable_fault_handlers(void)
{
	/* Explicitly set secure fault priority to the highest */
	NVIC_SetPriority(SecureFault_IRQn, 0);

	/* lower priority than SERC */
	NVIC_SetPriority(BusFault_IRQn, 2);

	/* Enables BUS, MEM, USG and Secure faults */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk
		| SCB_SHCSR_BUSFAULTENA_Msk
		| SCB_SHCSR_MEMFAULTENA_Msk
		| SCB_SHCSR_SECUREFAULTENA_Msk;
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t system_reset_cfg(void)
{
	uint32_t reg_value = SCB->AIRCR;

	/* Clear SCB_AIRCR_VECTKEY value */
	reg_value &= ~(uint32_t)(SCB_AIRCR_VECTKEY_Msk);

	/* Enable system reset request only to the secure world */
	reg_value |= (uint32_t)(SCB_AIRCR_WRITE_MASK | SCB_AIRCR_SYSRESETREQS_Msk);

	SCB->AIRCR = reg_value;

	return TFM_PLAT_ERR_SUCCESS;
}

/*----------------- NVIC interrupt target state to NS configuration ----------*/
enum tfm_plat_err_t nvic_interrupt_target_state_cfg(void)
{
	/* Target every interrupt to NS; unimplemented interrupts will be WI */
	for (uint8_t i=0; i<sizeof(NVIC->ITNS)/sizeof(NVIC->ITNS[0]); i++) {
		NVIC->ITNS[i] = 0xFFFFFFFF;
	}

	/* Make sure that IAC/SERF are targeted to S state */
	NVIC_ClearTargetState(IAC_IRQn);
	NVIC_ClearTargetState(SERF_IRQn);

	return TFM_PLAT_ERR_SUCCESS;
}

/*----------------- NVIC interrupt enabling for S peripherals ----------------*/
enum tfm_plat_err_t nvic_interrupt_enable()
{
	stm32_iac_enable_irq();
	stm32_serc_enable_irq();

	return TFM_PLAT_ERR_SUCCESS;
}

/*------------------- SAU/IDAU configuration functions -----------------------*/

void sau_and_idau_cfg(void)
{
	/* Disable SAU */
	TZ_SAU_Disable();

	/* Configures SAU regions to be non-secure */
	SAU->RNR  = 0U;
	SAU->RBAR = (memory_regions.non_secure_partition_base
		     & SAU_RBAR_BADDR_Msk);
	SAU->RLAR = (memory_regions.non_secure_partition_limit
		     & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

	SAU->RNR  = 1U;
	SAU->RBAR = (NS_DATA_START & SAU_RBAR_BADDR_Msk);
	SAU->RLAR = (NS_DATA_LIMIT & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

	/* Configures veneers region to be non-secure callable */
	SAU->RNR  = 2U;
	SAU->RBAR = (memory_regions.veneer_base  & SAU_RBAR_BADDR_Msk);
	SAU->RLAR = (memory_regions.veneer_limit & SAU_RLAR_LADDR_Msk)
		| SAU_RLAR_ENABLE_Msk | SAU_RLAR_NSC_Msk;

	/* Configure the peripherals space */
	SAU->RNR  = 3U;
	SAU->RBAR = (PERIPHERALS_BASE_NS_START & SAU_RBAR_BADDR_Msk);
	SAU->RLAR = (PERIPHERALS_BASE_NS_END & SAU_RLAR_LADDR_Msk)
		| SAU_RLAR_ENABLE_Msk;

	/* Force memory writes before continuing */
	__DSB();
	/* Flush and refill pipeline with updated permissions */
	__ISB();

	/* Enable SAU */
	TZ_SAU_Enable();
}
