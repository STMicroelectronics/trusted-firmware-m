/*
 * Copyright (C) 2020 STMicroelectronics.
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

/*
 * This file is derivative of CMSIS V5.6.0 startup_ARMv81MML.c
 * Git SHA: b5f0603d6a584d1724d952fd8b0737458b90d62b
 */

#include "cmsis.h"

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;

extern void __PROGRAM_START(void) __NO_RETURN;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
void Reset_Handler  (void) __NO_RETURN;

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
#define DEFAULT_IRQ_HANDLER(handler_name)  \
void __WEAK handler_name(void) __NO_RETURN; \
void handler_name(void) { \
    while(1); \
}

/* Exceptions */
DEFAULT_IRQ_HANDLER(NMI_Handler)
DEFAULT_IRQ_HANDLER(HardFault_Handler)
DEFAULT_IRQ_HANDLER(MemManage_Handler)
DEFAULT_IRQ_HANDLER(BusFault_Handler)
DEFAULT_IRQ_HANDLER(UsageFault_Handler)
DEFAULT_IRQ_HANDLER(SecureFault_Handler)
DEFAULT_IRQ_HANDLER(SVC_Handler)
DEFAULT_IRQ_HANDLER(DebugMon_Handler)
DEFAULT_IRQ_HANDLER(PendSV_Handler)
DEFAULT_IRQ_HANDLER(SysTick_Handler)

/* Core interrupts */
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(PVD_AVD_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(PVD_IRQHandler)
DEFAULT_IRQ_HANDLER(PVM_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(IWDG3_IRQHandler)
DEFAULT_IRQ_HANDLER(IWDG4_IRQHandler)
DEFAULT_IRQ_HANDLER(IWDG1_RST_IRQHandler)
DEFAULT_IRQ_HANDLER(IWDG2_RST_IRQHandler)
DEFAULT_IRQ_HANDLER(IWDG4_RST_IRQHandler)
DEFAULT_IRQ_HANDLER(IWDG5_RST_IRQHandler)
DEFAULT_IRQ_HANDLER(WWDG1_IRQHandler)
DEFAULT_IRQ_HANDLER(WWDG2_IRQHandler)
DEFAULT_IRQ_HANDLER(WWDG2_RST_IRQHandler)
DEFAULT_IRQ_HANDLER(TAMP_IRQHandler)
DEFAULT_IRQ_HANDLER(RTC_IRQHandler)
DEFAULT_IRQ_HANDLER(TAMP_S_IRQHandler)
DEFAULT_IRQ_HANDLER(RTC_S_IRQHandler)
DEFAULT_IRQ_HANDLER(RCC_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI0_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI1_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI2_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI3_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI4_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI5_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI6_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI7_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI8_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI9_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI10_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI11_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI12_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI13_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI14_IRQHandler)
DEFAULT_IRQ_HANDLER(EXTI15_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel0_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel1_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel2_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel3_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel4_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel5_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel6_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel7_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel8_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel9_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel10_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel11_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel12_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel13_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel14_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_Channel15_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel0_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel1_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel2_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel3_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel4_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel5_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel6_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel7_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel8_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel9_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel10_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel11_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel12_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel13_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel14_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_Channel15_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel0_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel1_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel2_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel3_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel4_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel5_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel6_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel7_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel8_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel9_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel10_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel11_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel12_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel13_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel14_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_Channel15_IRQHandler)
DEFAULT_IRQ_HANDLER(LPDMA_Channel0_IRQHandler)
DEFAULT_IRQ_HANDLER(LPDMA_Channel1_IRQHandler)
DEFAULT_IRQ_HANDLER(LPDMA_Channel2_IRQHandler)
DEFAULT_IRQ_HANDLER(LPDMA_Channel3_IRQHandler)
DEFAULT_IRQ_HANDLER(ICACHE_IRQHandler)
DEFAULT_IRQ_HANDLER(DCACHE_IRQHandler)
DEFAULT_IRQ_HANDLER(ADC1_IRQHandler)
DEFAULT_IRQ_HANDLER(ADC2_IRQHandler)
DEFAULT_IRQ_HANDLER(ADC3_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN_CAL_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN1_IT0_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN2_IT0_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN3_IT0_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN1_IT1_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN2_IT1_IRQHandler)
DEFAULT_IRQ_HANDLER(FDCAN3_IT1_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM1_BRK_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM1_UP_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM1_TRG_COM_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM1_CC_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM20_BRK_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM20_UP_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM20_TRG_COM_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM20_CC_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM2_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM3_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM4_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(I2C1_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C1_ER_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C2_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C2_ER_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(I2C1_IRQHandler)
DEFAULT_IRQ_HANDLER(I3C1_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C2_IRQHandler)
DEFAULT_IRQ_HANDLER(I3C2_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(SPI1_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI2_IRQHandler)
DEFAULT_IRQ_HANDLER(USART1_IRQHandler)
DEFAULT_IRQ_HANDLER(USART2_IRQHandler)
DEFAULT_IRQ_HANDLER(USART3_IRQHandler)
#if !defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER( VDEC_IRQHandler)
#endif /* ! STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(TIM8_BRK_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM8_UP_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM8_TRG_COM_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM8_CC_IRQHandler)
DEFAULT_IRQ_HANDLER(FMC_IRQHandler)
DEFAULT_IRQ_HANDLER(SDMMC1_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM5_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI3_IRQHandler)
DEFAULT_IRQ_HANDLER(UART4_IRQHandler)
DEFAULT_IRQ_HANDLER(UART5_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM6_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM7_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(ETH1_IRQHandler)
DEFAULT_IRQ_HANDLER(ETH1_WKUP_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(ETH1_SBD_IRQHandler)
DEFAULT_IRQ_HANDLER(ETH1_PMT_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(ETH1_LPI_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(ETH2_IRQHandler)
DEFAULT_IRQ_HANDLER(ETH2_WKUP_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(ETH2_SBD_IRQHandler)
DEFAULT_IRQ_HANDLER(ETH2_PMT_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(ETH2_LPI_IRQHandler)
DEFAULT_IRQ_HANDLER(USART6_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(I2C3_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C3_ER_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(I2C3_IRQHandler)
DEFAULT_IRQ_HANDLER(I3C3_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(USBH_OHCI_IRQHandler)
DEFAULT_IRQ_HANDLER(USBH_EHCI_IRQHandler)
DEFAULT_IRQ_HANDLER(DCMI_PSSI_IRQHandler)
DEFAULT_IRQ_HANDLER(CSI2HOST_IRQHandler)
DEFAULT_IRQ_HANDLER(DSI_IRQHandler)
#if defined(STM32MP257Cxx)
DEFAULT_IRQ_HANDLER(CRYP1_IRQHandler)
#endif /* STM32MP257Cxx */
DEFAULT_IRQ_HANDLER(HASH_IRQHandler)
DEFAULT_IRQ_HANDLER(PKA_IRQHandler)
DEFAULT_IRQ_HANDLER(FPU_IRQHandler)
DEFAULT_IRQ_HANDLER(UART7_IRQHandler)
DEFAULT_IRQ_HANDLER(UART8_IRQHandler)
DEFAULT_IRQ_HANDLER(UART9_IRQHandler)
DEFAULT_IRQ_HANDLER(LPUART1_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI4_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI5_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI6_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI7_IRQHandler)
DEFAULT_IRQ_HANDLER(SPI8_IRQHandler)
DEFAULT_IRQ_HANDLER(SAI1_IRQHandler)
DEFAULT_IRQ_HANDLER(LTDC_IRQHandler)
DEFAULT_IRQ_HANDLER(LTDC_ER_IRQHandler)
DEFAULT_IRQ_HANDLER(LTDC_L2_IRQHandler)
DEFAULT_IRQ_HANDLER(LTDC_L2_ER_IRQHandler)
DEFAULT_IRQ_HANDLER(SAI2_IRQHandler)
DEFAULT_IRQ_HANDLER(OCTOSPI1_IRQHandler)
DEFAULT_IRQ_HANDLER(OCTOSPI2_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(OTFDEC_IRQHandler)
#else /*STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(OTFDEC1_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(LPTIM1_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(CEC_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C4_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C4_ER_IRQHandler)
#else /*STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(VENC_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C4_IRQHandler)
DEFAULT_IRQ_HANDLER(USBH_WAKEUP_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(SPDIFRX_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC1_RX1_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC1_TX1_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC1_RX1_S_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC1_TX1_S_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC2_RX1_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC2_TX1_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC2_RX1_S_IRQHandler)
DEFAULT_IRQ_HANDLER(IPCC2_TX1_S_IRQHandler)
DEFAULT_IRQ_HANDLER(SAES_IRQHandler)
#if defined(STM32MP257Cxx)
DEFAULT_IRQ_HANDLER(CRYP2_IRQHandler)
#endif /* STM32MP257Cxx */
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(I2C5_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C5_ER_IRQHandler)
#else /*STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(I2C5_IRQHandler)
DEFAULT_IRQ_HANDLER(USB3DR_WAKEUP_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(GPU_IT_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT0_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT1_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT2_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT3_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT4_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT5_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT6_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF1_FLT7_IRQHandler)
DEFAULT_IRQ_HANDLER(SAI3_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM15_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM16_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM17_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM12_IRQHandler)
DEFAULT_IRQ_HANDLER(SDMMC2_IRQHandler)
DEFAULT_IRQ_HANDLER(DCMIPP_IRQHandler)
DEFAULT_IRQ_HANDLER(HSEM1_IRQHandler)
DEFAULT_IRQ_HANDLER(HSEM1_S_IRQHandler)
DEFAULT_IRQ_HANDLER(nCTIIRQ1_IRQHandler)
DEFAULT_IRQ_HANDLER(nCTIIRQ2_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM13_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM14_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM10_IRQHandler)
DEFAULT_IRQ_HANDLER(RNG_IRQHandler)
DEFAULT_IRQ_HANDLER(MDF2_FLT_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(I2C6_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C6_ER_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C7_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C7_ER_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C8_EV_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C8_ER_IRQHandler)
#else /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(I2C6_IRQHandler)
DEFAULT_IRQ_HANDLER(SERDES_WAKEUP_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C7_IRQHandler)
DEFAULT_IRQ_HANDLER(I2C8_IRQHandler)
DEFAULT_IRQ_HANDLER(I3C4_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(SDMMC3_IRQHandler)
DEFAULT_IRQ_HANDLER(LPTIM2_IRQHandler)
DEFAULT_IRQ_HANDLER(LPTIM3_IRQHandler)
DEFAULT_IRQ_HANDLER(LPTIM4_IRQHandler)
DEFAULT_IRQ_HANDLER(LPTIM5_IRQHandler)
#if ! defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(OTFDEC2_IRQHandler)
#endif /* else STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(CPU1_SEV_IRQHandler)
DEFAULT_IRQ_HANDLER(CPU3_SEV_IRQHandler)
DEFAULT_IRQ_HANDLER(RCC_WAKEUP_IRQHandler)
DEFAULT_IRQ_HANDLER(SAI4_IRQHandler)
DEFAULT_IRQ_HANDLER(DTS_IRQHandler)
DEFAULT_IRQ_HANDLER(TIM11_IRQHandler)
DEFAULT_IRQ_HANDLER(CPU2_WAKEUP_PIN_IRQHandler)
DEFAULT_IRQ_HANDLER(USB3DR_BC_IRQHandler)
DEFAULT_IRQ_HANDLER(USB3DR_IRQHandler)
DEFAULT_IRQ_HANDLER(USBPD_IRQHandler)
#if ! defined(STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(SERF_IRQHandler)
#endif /* ! STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(BUSPERFM_IRQHandler)
DEFAULT_IRQ_HANDLER(RAMCFG_IRQHandler)
DEFAULT_IRQ_HANDLER(UMCTL2_IRQHandler)
DEFAULT_IRQ_HANDLER(DDRPHY_IRQHandler)
#if defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(RSIMW1_IRQHandler)
#endif  /* STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(IAC_IRQHandler)
DEFAULT_IRQ_HANDLER(VDDCPU_VD_IT_IRQHandler)
DEFAULT_IRQ_HANDLER(VDDCORE_VD_IT_IRQHandler)
#if ! defined (STM32MP2XX_ASSY2_2_1)
DEFAULT_IRQ_HANDLER(ETHSW_IRQHandler)
DEFAULT_IRQ_HANDLER(ETHSW_MSG_BUF_IRQHandler)
DEFAULT_IRQ_HANDLER(ETHSW_FSC_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA1_WKUP_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA2_WKUP_IRQHandler)
DEFAULT_IRQ_HANDLER(HPDMA3_WKUP_IRQHandler)
DEFAULT_IRQ_HANDLER(LPDMA_WKUP_IRQHandler)
#endif /* ! STM32MP2XX_ASSY2_2_1 */
DEFAULT_IRQ_HANDLER(IS2M_IRQHandler)
DEFAULT_IRQ_HANDLER(DDRPERFM_IRQHandler)


/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

extern const VECTOR_TABLE_Type __VECTOR_TABLE[];
       const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
  (VECTOR_TABLE_Type)(&__INITIAL_SP),	/* Initial Stack Pointer */
  Reset_Handler,			/* Reset Handler */
  NMI_Handler,				/* NMI Handler */
  HardFault_Handler,			/* Hard Fault Handler */
  MemManage_Handler,			/* MPU Fault Handler */
  BusFault_Handler,			/* Bus Fault Handler */
  UsageFault_Handler,			/* Usage Fault Handler */
  SecureFault_Handler,			/* Secure Fault Handler */
  0,					/* Reserved */
  0,					/* Reserved */
  0,					/* Reserved */
  SVC_Handler,				/* SVCall Handler */
  DebugMon_Handler,			/* Debug Monitor Handler */
  0,					/* Reserved */
  PendSV_Handler,			/* PendSV Handler */
  SysTick_Handler,			/* SysTick Handler */

  /* Core interrupts */
  /***********************************************************/
  /* External interrupts according to "stm32mp2 M33 NVIC"    */
  /* (ASSY 2.2.x : version v0.18 - June 28th, 2019)          */
  /*     under STM32MP2XX_ASSY2_2_1 compilation flag         */
  /* (ASSY 2.3.0 : version v0.23 - October 30th, 2019)       */
  /***********************************************************/
#if defined (STM32MP2XX_ASSY2_2_1)
  PVD_AVD_IRQHandler,         /* PVD & AVD detector through EXTI */
  0,
#else /* STM32MP2XX_ASSY2_2_1 */
  PVD_IRQHandler,             /* PVD detector through EXTI */
  PVM_IRQHandler,             /* PVM detector through EXTI */
#endif /* STM32MP2XX_ASSY2_2_1 */
  IWDG3_IRQHandler,           /* Independent Watchdog 3 Early wake interrupt */
  IWDG4_IRQHandler,           /* Independent Watchdog 4 Early wake interrupt */
  IWDG1_RST_IRQHandler,       /* Independent Watchdog 1 Reset through EXTI */
  IWDG2_RST_IRQHandler,       /* Independent Watchdog 2 Reset through EXTI */
  IWDG4_RST_IRQHandler,       /* Independent Watchdog 4 Reset through EXTI */
  IWDG5_RST_IRQHandler,       /* Independent Watchdog 5 Reset through EXTI */
  WWDG1_IRQHandler,           /* Window Watchdog 1 Early Wakeup interrupt */
  WWDG2_IRQHandler,           /* Window Watchdog 2 Early Wakeup interrupt */
  0,
  WWDG2_RST_IRQHandler,       /* Window Watchdog 2 Reset through EXTI */
  TAMP_IRQHandler,            /* Tamper interrupt (include LSECSS interrupts) */
  RTC_IRQHandler,             /* RTC global interrupt */
  TAMP_S_IRQHandler,          /* Tamper secure interrupt (include LSECSS interrupts) */
  RTC_S_IRQHandler,           /* RTC global secure interrupt */
  RCC_IRQHandler,             /* RCC global interrupt */
  EXTI0_IRQHandler,           /* EXTI Line 0 interrupt */
  EXTI1_IRQHandler,           /* EXTI Line 1 interrupt */
  EXTI2_IRQHandler,           /* EXTI Line 2 interrupt */
  EXTI3_IRQHandler,           /* EXTI Line 3 interrupt */
  EXTI4_IRQHandler,           /* EXTI Line 4 interrupt */
  EXTI5_IRQHandler,           /* EXTI Line 5 interrupt */
  EXTI6_IRQHandler,           /* EXTI Line 6 interrupt */
  EXTI7_IRQHandler,           /* EXTI Line 7 interrupt */
  EXTI8_IRQHandler,           /* EXTI Line 8 interrupt */
  EXTI9_IRQHandler,           /* EXTI Line 9 interrupt */
  EXTI10_IRQHandler,          /* EXTI Line 10 interrupt */
  EXTI11_IRQHandler,          /* EXTI Line 11 interrupt */
  EXTI12_IRQHandler,          /* EXTI Line 12 interrupt */
  EXTI13_IRQHandler,          /* EXTI Line 13 interrupt */
  EXTI14_IRQHandler,          /* EXTI Line 14 interrupt */
  EXTI15_IRQHandler,          /* EXTI Line 15 interrupt */
  HPDMA1_Channel0_IRQHandler, /* HPDMA1 Channel0 interrupt */
  HPDMA1_Channel1_IRQHandler, /* HPDMA1 Channel1 interrupt */
  HPDMA1_Channel2_IRQHandler, /* HPDMA1 Channel2 interrupt */
  HPDMA1_Channel3_IRQHandler, /* HPDMA1 Channel3 interrupt */
  HPDMA1_Channel4_IRQHandler, /* HPDMA1 Channel4 interrupt */
  HPDMA1_Channel5_IRQHandler, /* HPDMA1 Channel5 interrupt */
  HPDMA1_Channel6_IRQHandler, /* HPDMA1 Channel6 interrupt */
  HPDMA1_Channel7_IRQHandler, /* HPDMA1 Channel7 interrupt */
  HPDMA1_Channel8_IRQHandler, /* HPDMA1 Channel8 interrupt */
  HPDMA1_Channel9_IRQHandler, /* HPDMA1 Channel9 interrupt */
  HPDMA1_Channel10_IRQHandler,/* HPDMA1 Channel10 interrupt */
  HPDMA1_Channel11_IRQHandler,/* HPDMA1 Channel11 interrupt */
  HPDMA1_Channel12_IRQHandler,/* HPDMA1 Channel12 interrupt */
  HPDMA1_Channel13_IRQHandler,/* HPDMA1 Channel13 interrupt */
  HPDMA1_Channel14_IRQHandler,/* HPDMA1 Channel14 interrupt */
  HPDMA1_Channel15_IRQHandler,/* HPDMA1 Channel15 interrupt */
  HPDMA2_Channel0_IRQHandler, /* HPDMA2 Channel0 interrupt */
  HPDMA2_Channel1_IRQHandler, /* HPDMA2 Channel1 interrupt */
  HPDMA2_Channel2_IRQHandler, /* HPDMA2 Channel2 interrupt */
  HPDMA2_Channel3_IRQHandler, /* HPDMA2 Channel3 interrupt */
  HPDMA2_Channel4_IRQHandler, /* HPDMA2 Channel4 interrupt */
  HPDMA2_Channel5_IRQHandler, /* HPDMA2 Channel5 interrupt */
  HPDMA2_Channel6_IRQHandler, /* HPDMA2 Channel6 interrupt */
  HPDMA2_Channel7_IRQHandler, /* HPDMA2 Channel7 interrupt */
  HPDMA2_Channel8_IRQHandler, /* HPDMA2 Channel8 interrupt */
  HPDMA2_Channel9_IRQHandler, /* HPDMA2 Channel9 interrupt */
  HPDMA2_Channel10_IRQHandler,/* HPDMA2 Channel10 interrupt */
  HPDMA2_Channel11_IRQHandler,/* HPDMA2 Channel11 interrupt */
  HPDMA2_Channel12_IRQHandler,/* HPDMA2 Channel12 interrupt */
  HPDMA2_Channel13_IRQHandler,/* HPDMA2 Channel13 interrupt */
  HPDMA2_Channel14_IRQHandler,/* HPDMA2 Channel14 interrupt */
  HPDMA2_Channel15_IRQHandler,/* HPDMA2 Channel15 interrupt */
  HPDMA3_Channel0_IRQHandler, /* HPDMA3 Channel0 interrupt */
  HPDMA3_Channel1_IRQHandler, /* HPDMA3 Channel1 interrupt */
  HPDMA3_Channel2_IRQHandler, /* HPDMA3 Channel2 interrupt */
  HPDMA3_Channel3_IRQHandler, /* HPDMA3 Channel3 interrupt */
  HPDMA3_Channel4_IRQHandler, /* HPDMA3 Channel4 interrupt */
  HPDMA3_Channel5_IRQHandler, /* HPDMA3 Channel5 interrupt */
  HPDMA3_Channel6_IRQHandler, /* HPDMA3 Channel6 interrupt */
  HPDMA3_Channel7_IRQHandler, /* HPDMA3 Channel7 interrupt */
  HPDMA3_Channel8_IRQHandler, /* HPDMA3 Channel8 interrupt */
  HPDMA3_Channel9_IRQHandler, /* HPDMA3 Channel9 interrupt */
  HPDMA3_Channel10_IRQHandler,/* HPDMA3 Channel10 interrupt */
  HPDMA3_Channel11_IRQHandler,/* HPDMA3 Channel11 interrupt */
  HPDMA3_Channel12_IRQHandler,/* HPDMA3 Channel12 interrupt */
  HPDMA3_Channel13_IRQHandler,/* HPDMA3 Channel13 interrupt */
  HPDMA3_Channel14_IRQHandler,/* HPDMA3 Channel14 interrupt */
  HPDMA3_Channel15_IRQHandler,/* HPDMA3 Channel15 interrupt */
  LPDMA_Channel0_IRQHandler,  /* LPDMA Channel0 interrupt */
  LPDMA_Channel1_IRQHandler,  /* LPDMA Channel1 interrupt */
  LPDMA_Channel2_IRQHandler,  /* LPDMA Channel2 interrupt */
  LPDMA_Channel3_IRQHandler,  /* LPDMA Channel3 interrupt */
  ICACHE_IRQHandler,          /* ICACHE interrupt */
  DCACHE_IRQHandler,          /* DCACHE interrupt */
  ADC1_IRQHandler,            /* ADC1 interrupt */
  ADC2_IRQHandler,            /* ADC2  interrupt */
  ADC3_IRQHandler,            /* ADC3 interrupt */
  FDCAN_CAL_IRQHandler,       /* FDCAN CCU interrupt */
  FDCAN1_IT0_IRQHandler,      /* FDCAN1 interrupt 0 */
  FDCAN2_IT0_IRQHandler,      /* FDCAN2 interrupt 0 */
  FDCAN3_IT0_IRQHandler,      /* FDCAN3 interrupt 0 */
  FDCAN1_IT1_IRQHandler,      /* FDCAN1 interrupt 1 */
  FDCAN2_IT1_IRQHandler,      /* FDCAN2 interrupt 1 */
  FDCAN3_IT1_IRQHandler,      /* FDCAN3 interrupt 1 */
  TIM1_BRK_IRQHandler,        /* TIM1 Break interrupt */
  TIM1_UP_IRQHandler,         /* TIM1 Update interrupt */
  TIM1_TRG_COM_IRQHandler,    /* TIM1 Trigger and Commutation interrupts */
  TIM1_CC_IRQHandler,         /* TIM1 Capture Compare interrupt */
  TIM20_BRK_IRQHandler,       /* TIM20 Break interrupt */
  TIM20_UP_IRQHandler,        /* TIM20 Update interrupt */
  TIM20_TRG_COM_IRQHandler,   /* TIM20 Trigger and Commutation interrupts */
  TIM20_CC_IRQHandler,        /* TIM20 Capture Compare interrupt */
  TIM2_IRQHandler,            /* TIM2 global interrupt */
  TIM3_IRQHandler,            /* TIM3 global interrupt */
  TIM4_IRQHandler,            /* TIM4 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  I2C1_EV_IRQHandler,         /* I2C1 event interrupt */
  I2C1_ER_IRQHandler,         /* I2C1 global error interrupt */
  I2C2_EV_IRQHandler,         /* I2C2 event interrupt */
  I2C2_ER_IRQHandler,         /* I2C2 global error interrupt */
#else /* STM32MP2XX_ASSY2_2_1 */
  I2C1_IRQHandler,            /* I2C1 global interrupt */
  I3C1_IRQHandler,            /* I3C1 global interrupt */
  I2C2_IRQHandler,            /* I2C2 global interrupt */
  I3C2_IRQHandler,            /* I3C2 global interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  SPI1_IRQHandler,            /* SPI1 global interrupt */
  SPI2_IRQHandler,            /* SPI2 global interrupt */
  USART1_IRQHandler,          /* USART1 global interrupt */
  USART2_IRQHandler,          /* USART2 global interrupt */
  USART3_IRQHandler,          /* USART3 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  0,
#else /* STM32MP2XX_ASSY2_2_1 */
  VDEC_IRQHandler,            /* VDEC global interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  TIM8_BRK_IRQHandler,        /* TIM8 Break interrupt */
  TIM8_UP_IRQHandler,         /* TIM8 Update interrupt */
  TIM8_TRG_COM_IRQHandler,    /* TIM8 Trigger & Commutation interrupt */
  TIM8_CC_IRQHandler,         /* TIM8 Capture Compare interrupt */
  FMC_IRQHandler,             /* FMC global interrupt */
  SDMMC1_IRQHandler,          /* SDMMC1 global interrupt */
  TIM5_IRQHandler,            /* TIM5 global interrupt */
  SPI3_IRQHandler,            /* SPI3 global interrupt */
  UART4_IRQHandler,           /* UART4 global interrupt */
  UART5_IRQHandler,           /* UART5 global interrupt */
  TIM6_IRQHandler,            /* TIM6 global interrupt */
  TIM7_IRQHandler,            /* TIM7 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  ETH1_IRQHandler,            /* ETH1 global interrupt */
  ETH1_WKUP_IRQHandler,       /* ETH1 wake-up interrupt (PMT) */
#else /* STM32MP2XX_ASSY2_2_1 */
  ETH1_SBD_IRQHandler,        /* ETH1 global interrupt */
  ETH1_PMT_IRQHandler,        /* ETH1 wake-up interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  ETH1_LPI_IRQHandler,        /* ETH1 LPI interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  ETH2_IRQHandler,            /* ETH2 global interrupt */
  ETH2_WKUP_IRQHandler,       /* ETH2 wake-up interrupt (PMT) */
#else /* STM32MP2XX_ASSY2_2_1 */
  ETH2_SBD_IRQHandler,        /* ETH2 global interrupt */
  ETH2_PMT_IRQHandler,        /* ETH2 wake-up interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  ETH2_LPI_IRQHandler,        /* ETH2 LPI interrupt */
  USART6_IRQHandler,          /* USART6 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  I2C3_EV_IRQHandler,         /* I2C3 event interrupt */
  I2C3_ER_IRQHandler,         /* I2C3 global error interrupt */
#else /* STM32MP2XX_ASSY2_2_1 */
  I2C3_IRQHandler,            /* I2C3 global interrupt */
  I3C3_IRQHandler,            /* I3C3 global interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  USBH_OHCI_IRQHandler,       /* USB Host OHCI interrupt */
  USBH_EHCI_IRQHandler,       /* USB Host EHCI interrupt */
  DCMI_PSSI_IRQHandler,       /* DCMI & PSSI global interrupt */
  CSI2HOST_IRQHandler,        /* CSI2HOST interrupt */
  DSI_IRQHandler,             /* DSI Host controller global interrupt */
#if defined(STM32MP257Cxx)
  CRYP1_IRQHandler,           /* Crypto1 interrupt */
#else /* STM32MP257Cxx */
  0,
#endif /* else STM32MP257Cxx */
  HASH_IRQHandler,            /* Hash interrupt */
  PKA_IRQHandler,             /* PKA interrupt */
  FPU_IRQHandler,             /* FPU global interrupt */
  UART7_IRQHandler,           /* UART7 global interrupt */
  UART8_IRQHandler,           /* UART8 global interrupt */
  UART9_IRQHandler,           /* UART9 global interrupt */
  LPUART1_IRQHandler,         /* LPUART1 global interrupt */
  SPI4_IRQHandler,            /* SPI4 global interrupt */
  SPI5_IRQHandler,            /* SPI5 global interrupt */
  SPI6_IRQHandler,            /* SPI6 global interrupt */
  SPI7_IRQHandler,            /* SPI7 global interrupt */
  SPI8_IRQHandler,            /* SPI8 global interrupt */
  SAI1_IRQHandler,            /* SAI1 global interrupt */
  LTDC_IRQHandler,            /* LTDC layer01 global interrupt */
  LTDC_ER_IRQHandler,         /* LTDC layer01 global error interrupt */
  LTDC_L2_IRQHandler,         /* LTDC layer2 global interrupt */
  LTDC_L2_ER_IRQHandler,      /* LTDC layer2 global error interrupt */
  SAI2_IRQHandler,            /* SAI2 global interrupt */
  OCTOSPI1_IRQHandler,        /* OCTOSPI1 global interrupt */
  OCTOSPI2_IRQHandler,        /* OCTOSPI2 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  OTFDEC_IRQHandler,          /* OTFDEC interrupt */
#else /*STM32MP2XX_ASSY2_2_1 */
  OTFDEC1_IRQHandler,         /* OTFDEC1 interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  LPTIM1_IRQHandler,          /* LPTIMER1 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  CEC_IRQHandler,             /* HDMI-CEC global interrupt */
  I2C4_EV_IRQHandler,         /* I2C4 event interrupt */
  I2C4_ER_IRQHandler,         /* I2C4 global error interrupt */
#else /*STM32MP2XX_ASSY2_2_1 */
  VENC_IRQHandler,            /* VENC global interrupt */
  I2C4_IRQHandler,            /* I2C4 global interrupt */
  USBH_WAKEUP_IRQHandler,     /* USB Host remote wake up from USB2PHY1 interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  SPDIFRX_IRQHandler,         /* SPDIFRX global interrupt */
  IPCC1_RX1_IRQHandler,       /* Mailbox 1 RX1 Occupied cpu2 interrupt */
  IPCC1_TX1_IRQHandler,       /* Mailbox 1 TX1 Free cpu2 interrupt */
  IPCC1_RX1_S_IRQHandler,     /* Mailbox 1 RX1 Occupied cpu2 secure interrupt */
  IPCC1_TX1_S_IRQHandler,     /* Mailbox 1 TX1 Free cpu2 secure interrupt */
  IPCC2_RX1_IRQHandler,       /* Mailbox 2 RX1 Occupied cpu2 interrupt */
  IPCC2_TX1_IRQHandler,       /* Mailbox 2 TX1 Free cpu2 interrupt */
  IPCC2_RX1_S_IRQHandler,     /* Mailbox 2 RX1 Occupied cpu2 secure interrupt */
  IPCC2_TX1_S_IRQHandler,     /* Mailbox 2 TX1 Free cpu2 secure interrupt */
  SAES_IRQHandler,            /* Secure AES */
#if defined(STM32MP257Cxx)
  CRYP2_IRQHandler,           /* Crypto2 interrupt */
#else /* STM32MP257Cxx */
  0,
#endif /* else STM32MP257Cxx */
#if defined (STM32MP2XX_ASSY2_2_1)
  I2C5_EV_IRQHandler,         /* I2C5 event interrupt */
  I2C5_ER_IRQHandler,         /* I2C5 global error interrupt */
#else /*STM32MP2XX_ASSY2_2_1 */
  I2C5_IRQHandler,            /* I2C5 global interrupt */
  USB3DR_WAKEUP_IRQHandler,   /* USB3 remote wake up from USB2PHY1 interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  GPU_IT_IRQHandler,          /* GPU global Interrupt */
  MDF1_FLT0_IRQHandler,       /* MDF1 Filter0 interrupt */
  MDF1_FLT1_IRQHandler,       /* MDF1 Filter1 interrupt */
  MDF1_FLT2_IRQHandler,       /* MDF1 Filter2 interrupt */
  MDF1_FLT3_IRQHandler,       /* MDF1 Filter3 interrupt */
  MDF1_FLT4_IRQHandler,       /* MDF1 Filter4 interrupt */
  MDF1_FLT5_IRQHandler,       /* MDF1 Filter5 interrupt */
  MDF1_FLT6_IRQHandler,       /* MDF1 Filter6 interrupt */
  MDF1_FLT7_IRQHandler,       /* MDF1 Filter7 interrupt */
  SAI3_IRQHandler,            /* SAI3 global interrupt */
  TIM15_IRQHandler,           /* TIM15 global interrupt */
  TIM16_IRQHandler,           /* TIM16 global interrupt */
  TIM17_IRQHandler,           /* TIM17 global interrupt */
  TIM12_IRQHandler,           /* TIM12 global interrupt */
  SDMMC2_IRQHandler,          /* SDMMC2 global interrupt */
  DCMIPP_IRQHandler,          /* DCMIPP global interrupt */
  HSEM1_IRQHandler,           /* HSEM1 Semaphore nonsecure interrupt 1 */
  HSEM1_S_IRQHandler,         /* HSEM1 Semaphore secure interrupt 1 */
  nCTIIRQ1_IRQHandler,        /* Cortex-M33 CTI interrupt 1 */
  nCTIIRQ2_IRQHandler,        /* Cortex-M33 CTI interrupt 2 */
  TIM13_IRQHandler,           /* TIM13 global interrupt */
  TIM14_IRQHandler,           /* TIM14 global interrupt */
  TIM10_IRQHandler,           /* TIM10 global interrupt */
  RNG_IRQHandler,             /* RNG global interrupt */
  MDF2_FLT_IRQHandler,        /* MDF2 Filter interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  I2C6_EV_IRQHandler,         /* I2C6 event interrupt */
  I2C6_ER_IRQHandler,         /* I2C6 global error interrupt */
  I2C7_EV_IRQHandler,         /* I2C7 event interrupt */
  I2C7_ER_IRQHandler,         /* I2C7 global error interrupt */
  I2C8_EV_IRQHandler,         /* I2C8 event interrupt */
  I2C8_ER_IRQHandler,         /* I2C8 global error interrupt */
#else /* STM32MP2XX_ASSY2_2_1 */
  I2C6_IRQHandler,            /* I2C6 global interrupt */
  SERDES_WAKEUP_IRQHandler,   /* SERDES LFPS start request interrupt */
  I2C7_IRQHandler,            /* I2C7 global interrupt */
  0,
  I2C8_IRQHandler,            /* I2C8 global interrupt */
  I3C4_IRQHandler,            /* I3C4 global interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  SDMMC3_IRQHandler,          /* SDMMC3 global interrupt */
  LPTIM2_IRQHandler,          /* LPTIMER2 global interrupt */
  LPTIM3_IRQHandler,          /* LPTIMER3 global interrupt */
  LPTIM4_IRQHandler,          /* LPTIMER4 global interrupt */
  LPTIM5_IRQHandler,          /* LPTIMER5 global interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  0,
#else /* STM32MP2XX_ASSY2_2_1 */
  OTFDEC2_IRQHandler,         /* OTFDEC2 interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  CPU1_SEV_IRQHandler,        /* Cortex-A35 Send Event through EXTI */
  CPU3_SEV_IRQHandler,        /* Cortex-M0+ Send Event through EXTI */
  RCC_WAKEUP_IRQHandler,      /* RCC CPU2 Wake up interrupt */
  SAI4_IRQHandler,            /* SAI4 global interrupt */
  DTS_IRQHandler,             /* Temperature sensor interrupt */
  TIM11_IRQHandler,           /* TIMER11 global interrupt */
  CPU2_WAKEUP_PIN_IRQHandler, /* Interrupt for all 6 wake-up enabled by CPU2 */
  USB3DR_BC_IRQHandler,       /* USB3 BC interrupt */
  USB3DR_IRQHandler,          /* USB3 interrupt */
  USBPD_IRQHandler,           /* USB PD interrupt */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
#if defined (STM32MP2XX_ASSY2_2_1)
  0,
#else /* STM32MP2XX_ASSY2_2_1 */
  SERF_IRQHandler,            /* SERF global interrupt */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  BUSPERFM_IRQHandler,        /* BUS Performance Monitor interrupt */
  RAMCFG_IRQHandler,          /* RAM configuration global interrupt */
  UMCTL2_IRQHandler,          /* UMCTL2 interrupt */
  DDRPHY_IRQHandler,          /* DDRPHY interrupt */
#if defined (STM32MP2XX_ASSY2_2_1)
  RSIMW1_IRQHandler,          /* DDR RIF access filter interrupt */
#else /* STM32MP2XX_ASSY2_2_1 */
  0,
#endif /* else STM32MP2XX_ASSY2_2_1 */
  IAC_IRQHandler,             /* RIF Illegal access Controller interrupt */
  VDDCPU_VD_IT_IRQHandler,    /* VDDCPU voltage detector */
  VDDCORE_VD_IT_IRQHandler,   /* VDDCORE voltage detector */
  0,
#if defined (STM32MP2XX_ASSY2_2_1)
  0,
  0,
  0,
  0,
  0,
  0,
  0,
#else /* STM32MP2XX_ASSY2_2_1 */
  ETHSW_IRQHandler,           /* ETHSW global interrupt */
  ETHSW_MSG_BUF_IRQHandler,   /* ETHSW ACM Message buffer interrupt */
  ETHSW_FSC_IRQHandler,       /* ETHSW ACM Scheduler interrupt */
  HPDMA1_WKUP_IRQHandler,     /* HPDMA1 channel 0 to 15 wake up */
  HPDMA2_WKUP_IRQHandler,     /* HPDMA2 channel 0 to 15 wake up */
  HPDMA3_WKUP_IRQHandler,     /* HPDMA3 channel 0 to 15 wake up */
  LPDMA_WKUP_IRQHandler,      /* LPDMA channel 0 to 3 wake up */
#endif /* else STM32MP2XX_ASSY2_2_1 */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  IS2M_IRQHandler,            /* IS2M fault detection interrupt */
  0,
  DDRPERFM_IRQHandler,        /* DDR Performance Monitor interrupt */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
void Reset_Handler(void)
{
	__set_MSPLIM((uint32_t)(&__STACK_LIMIT));

	__set_PSP((uint32_t)(&__INITIAL_SP));
	__set_PSPLIM((uint32_t)(&__STACK_LIMIT));

	/* not switch to unprivilage for systick platform init */
	/* else add "ORR     R0, R0, #1\n" */

/*        __ASM volatile("MRS     R0, control\n"    |+ Get control value +|*/
/*                       "ORR     R0, R0, #2\n"     |+ Select switch to PSP +|*/
/*                       "MSR     control, R0\n"    |+ Load control register +|*/
/*                       :*/
/*                       :*/
/*                       : "r0");*/

	SystemInit();		/* CMSIS System Initialization */
	__PROGRAM_START();      /* Enter PreMain (C library entry point) */
}
