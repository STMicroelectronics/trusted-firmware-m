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
#include <tfm_plat_defs.h>
#include <lib/utils_def.h>

#include <stm32_iac.h>
#include <stm32_serc.h>

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
