/**************************************************************************//**
 * @file     system_stm32mp2xx_m33.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM33 Device Series
 * @version  V5.00
 * @date     02. November 2016
 ******************************************************************************/
/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stm32mp2xx.h"

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#if !defined  (HSI_VALUE)
 #if defined (USE_STM32MP257CXX_EMU)
   #define HSI_VALUE             ((uint32_t)200000U)     /*!< Value of the Internal High Speed oscillator in Hz*/
 #elif defined (USE_STM32MP257CXX_FPGA)
   #define HSI_VALUE             ((uint32_t)32000000U)   /*!< Value of the Internal High Speed oscillator in Hz*/
 #else /* USE_STM32MP257CXX_EMU | USE_STM32MP257CXX_FPGA */
   #define HSI_VALUE             ((uint32_t)64000000U)   /*!< Value of the Internal High Speed oscillator in Hz*/
 #endif /* else USE_STM32MP257CXX_EMU | USE_STM32MP257CXX_FPGA */
#endif /* HSI_VALUE */

/*----------------------------------------------------------------------------
  Externals
 *----------------------------------------------------------------------------*/
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  extern uint32_t __Vectors;
#endif

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = 0;

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  SCB->VTOR = (uint32_t) &__Vectors;
#endif

#if defined (__FPU_USED) && (__FPU_USED == 1U)
  SCB->CPACR |= ((3U << 10U*2U) |           /* enable CP10 Full Access */
                 (3U << 11U*2U)  );         /* enable CP11 Full Access */
#endif

#ifdef UNALIGNED_SUPPORT_DISABLE
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif
}
