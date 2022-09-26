/**
  ******************************************************************************
  * @file    system_stm32mp2xx.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M0+/Cortex-M33/Cortex-A35 Device System Source File
  *          for STM32MP2xx devices.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32mp2xx_system
  * @{
  */

/**
  * @brief Define to prevent recursive inclusion
  */
#ifndef __SYSTEM_STM32MP2XX_H
#define __SYSTEM_STM32MP2XX_H

#ifdef __cplusplus
 extern "C" {
#endif

/** @addtogroup STM32MP2xx_System_Includes
  * @{
  */
#include <stdint.h>
#include <stm32mp2xx.h>
#ifdef CORE_CA35
#include "irq_ctrl.h"
#endif

/**
  * @}
  */

/** @addtogroup STM32MP2xx_System_Exported_defines
  * @{
  */
#ifdef CORE_CA35
#define A35_SYSTICK_PHYSICALTIMERSOURCE 0xF5F5F5F5UL
#define A35_SYSTICK_VIRTUALTIMERSOURCE  0x21212121UL

#define A35_SUSPEND_SYSTICK 0xD15AB1EEUL
#define A35_RESUME_SYSTICK  0xEAB1EAB1UL

#define A35_NB_CORES 2

#define A35_IT_SECURE       0x5E
#define A35_IT_NON_SECURE   0x05

#define IT_TARGETS_A35_0     0x01
#define IT_TARGETS_A35_1     0x02
#define IT_TARGETS_A35_X     0x03

#define WA_CMSIS_ISSUE_IRQ_SETMODE
#define CR_CMSIS_IT_GROUP_MANAGEMENT
#endif

/**
  * @}
  */
/** @addtogroup STM32MP2xx_System_Exported_types
  * @{
  */
#ifdef CORE_CA35
typedef struct gic_interrupt_config
{
   /* Interrupt index : */
   /*        value in range [0;Max_IRQn] (see stm32mp2xxxxx.h) */
   IRQn_Type it_idx;
   /* Interrupt priority : */
   /*        value in range [0;(1<<(GIC_PRIO_BITS-1))-1], whatever security mode */
   /*        A shift of one-bit left will be applied at programming step */
   /*        in non-secure interrupt case. */
   uint8_t   it_priority;
   /* Interrupt trigger mode : */
   /*        value among IRQ_MODE_TRIG_* (see irq_ctrl.h) */
   uint8_t   it_trigger_mode;
   /* Interrupt security mode : */
   /*        value among {A35_IT_SECURE; A35_IT_NON_SECURE} */
   uint8_t   it_security_mode;
   /* Interrupt A35 CPU target(s) : */
   /*        value among {IT_TARGETS_A35_0; IT_TARGETS_A35_1; IT_TARGETS_A35_X} */
   uint8_t   it_a35_targets;
} gic_interrupt_config_t;
#endif

/**
  * @}
  */

/** @addtogroup STM32MP2x_System_Exported_Constants
  * @{
  */

/**
  \brief Exception / Interrupt Handler Function Prototype
*/
typedef void(*VECTOR_TABLE_Type)(void);
  * @}
  */

/** @addtogroup STM32MP2xx_System_Exported_Variables
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
extern uint32_t SystemCoreClock;          /*!< System Core1 Clock Frequency  */

#ifdef CORE_CA35
extern IRQHandler_t Interrupt_Handlers_Table[MAX_IRQ_n];
extern uint32_t Processing_IRQ_flag;
extern uint32_t IRQ_current_index;
extern uint32_t Processing_FIQ_flag;
extern uint32_t FIQ_current_index;
#endif

/**
  * @}
  */

/** @addtogroup STM32MP2xx_System_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32MP2xx_System_Exported_Functions
  * @{
  */

extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
#ifdef CORE_CA35
/* A35ss specific function to be used by HAL_InitTick() */
extern uint32_t SystemA35_TZ_STGEN_Start( uint32_t ck_ker_clk_freq );
/* A35ss specific function to set timer source to be used by SystemA35_SYSTICK_Config() */
extern uint32_t SystemA35_SYSTICK_TimerSourceConfig( uint32_t timer_source );
/* A35ss specific function to be used by HAL_SYSTICK_Config() */
extern uint32_t SystemA35_SYSTICK_Config( uint32_t ticks_number,
                                          uint32_t timer_priority );
/* A35ss specific function to be used by HAL_SuspendTick() and HAL_ResumeTick() */
/* for managing SysTick according to its timer source */
extern uint32_t SystemA35_ManageTick( uint32_t suspend_or_resume_tick );

/* Weak interrupt handler */
extern void Weak_ITHandler(void);

/* A35ss generic functions to route/unroute an interrupt through GIC400 to CPU target(s) */
extern uint32_t SystemA35_RouteInterrupt( gic_interrupt_config_t interrupt_config );
extern uint32_t SystemA35_UnrouteInterrupt( uint32_t interrupt_index );
/* A35ss specific function to trigger an interrupt at GIC400 input */
extern uint32_t SystemA35_TriggerInterruptAtGICInput( gic_interrupt_config_t interrupt_config );
/* A35ss specific function to get A35 secure state by reading GICD_ICFGR0 */
extern uint32_t SystemA35_IsA35InSecureState( void );
#endif

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_STM32MP2XX_H */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
