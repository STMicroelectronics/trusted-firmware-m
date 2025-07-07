/*
 ******************************************************************************
 * @file    stl_util.c
 * @author  MCD Application Team
 * @brief   STL Utility
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <device.h>
#include <stl_util.h>
#include <stl_stm32_hw_config.h>
#include <syscon.h>
#include <tfm_arch.h>
/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#ifndef STL_ENABLE_IT
static uint32_t ExcepMaskReg;
#endif
#ifndef STL_ENABLE_FPU_IXC_IT
static uint32_t SysCfgCfgr1BitIxcValue;
#endif
#ifndef STL_SW_CRC
static uint32_t CrcClkStatus;
#endif

/* Private function prototypes -----------------------------------------------*/
#ifdef STL_SW_CRC
uint32_t CRC_Handle_32_SW(const uint32_t *pBuffer, uint32_t BufferLength);
#else
uint32_t CRC_Handle_32_HW(const uint32_t *pBuffer, uint32_t BufferLength, uint32_t CrcDr);
#endif /* STL_SW_CRC */
static uint32_t ChecksumCompute(void *pData, uint32_t Length);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  This function makes CRC initialisation (nothing done in case of SW CRC)
  * @note   The function stores user RCC CRC enable
  * @param  None
  * @retval None
  */
void STL_UTIL_CRC_Init(void)
{
	/* STL target a IEEE 802.3 compliant 32 bit CRC */
#ifdef STL_SW_CRC
#else
	/* Save CRC clock status */
	CrcClkStatus = (*(uint32_t *)(STL_WRP_RCC_BASE + STL_WRP_RCC_AHB1ENR_OFFSET))
		& STL_WRP_RCC_AHB1ENR_CRCEN;
	/* Enable CRC clock */
	*(uint32_t *)(STL_WRP_RCC_BASE + STL_WRP_RCC_AHB1ENR_OFFSET) |= STL_WRP_RCC_AHB1ENR_CRCEN;

	/* CRC reset value to fit with PC CRC32 configuration */
	/* Input data bit reversed by byte, Output data bit reversed */
	STL_WRP_CRC->CR |= STL_WRP_CRC_CR_DR_RESET;
	STL_WRP_CRC->IDR = STL_WRP_CRC_IDR_DEFAULT_VALUE;
	STL_WRP_CRC->CR = STL_WRP_CRC_CR_OUTPUT_DATA_REVERSE
		| STL_WRP_CRC_CR_INPUT_DATA_REVERSE_WORD
		| STL_WRP_CRC_CR_INPUT_DATA_REVERSE_HALFWORD
		| STL_WRP_CRC_CR_POLYLENGTH_32B;
	STL_WRP_CRC->INIT = STL_WRP_CRC_INIT_DEFAULT_VALUE;
	STL_WRP_CRC->POL = STL_WRP_CRC_POL_DEFAULT_CRC32_POLY;
#endif /* STL_SW_CRC */
}

/**
 * @brief  This function makes CRC deinitialisation (nothing done in case of SW CRC)
 * @note   The function restores user RCC CRC enable
 * @param  None
 * @retval None
 */
void STL_UTIL_CRC_DeInit(void)
{
#ifdef STL_SW_CRC
#else
	STL_WRP_CRC->CR |= STL_WRP_CRC_CR_DR_RESET;
	STL_WRP_CRC->IDR = STL_WRP_CRC_IDR_DEFAULT_VALUE;

	/* Restore CRC clock */
	if (CrcClkStatus != STL_WRP_RCC_AHB1ENR_CRCEN)
	{
		*(uint32_t *)(STL_WRP_RCC_BASE + STL_WRP_RCC_AHB1ENR_OFFSET)
			&= ~STL_WRP_RCC_AHB1ENR_CRCEN;
	}
#endif /* STL_SW_CRC */
}

/**
  * @brief  This function makes CRC reset (nothing done in case of SW CRC)
  * @note   None
  * @param  None
  * @retval None
  */
void STL_UTIL_CRC_Reset(void)
{
#ifdef STL_SW_CRC
#else
	STL_WRP_CRC->CR |= STL_WRP_CRC_CR_DR_RESET;
#endif /* STL_SW_CRC */
}

/**
 * @brief  This function calculates a CRC
 * @note   None
 * @param  *pBuffer     CRC value to be calculated on pBuffer content
 * @note   None
 * @param  BufferLength Length of pBuffer on which CRC is calculated
 * @note   BufferLength is words
 * @retval uint32_t CRC calculated/output value
 */
uint32_t STL_UTIL_CRC_Calculate(const uint32_t *pBuffer, uint32_t BufferLength)
{
	uint32_t crc_out;/* CRC output */

#ifdef STL_SW_CRC
	crc_out = CRC_Handle_32_SW(pBuffer, BufferLength);
#else
	STL_UTIL_CRC_Reset();
	crc_out = CRC_Handle_32_HW(pBuffer, BufferLength, (uint32_t)&(STL_WRP_CRC->DR));
#endif /* STL_SW_CRC */

	/* Return the CRC computed value */
	return crc_out;
}

/**
 * @brief  This function makes comparison of between previous Checksum and current value
 * @note   It is for Defensive programming
 * @param  *pData     1st parameter of ChecksumCompute
 * @note   None
 * @param  Length     2nd parameter of ChecksumCompute
 * @note   Length in bytes
 * @param  Checksum   previous checksum value to be compared with
 * @note   None
 * @retval STL_UTIL_ChecksumComp_t checksum comparison value
 */
STL_UTIL_ChecksumComp_t STL_UTIL_ChecksumCompare(void *pData, uint32_t Length,
						 uint32_t Checksum)
{
	STL_UTIL_ChecksumComp_t ChecksumComp;

	if (Checksum == ChecksumCompute(pData, Length))
		ChecksumComp = STL_UTIL_CHECKSUM_COMP_OK;
	else
		ChecksumComp = STL_UTIL_CHECKSUM_COMP_KO;

	return ChecksumComp;
}

/**
 * @brief  This function updates the Checksum reference value (no more CRC one)
 * @note   None
 * @param  *pData Values to be calculated on pData content
 * @note   None
 * @param  Length       Length of pData on which checksum is calculated
 * @note   Length in bytes
 * @retval uint32_t     Calculated value
 */
uint32_t STL_UTIL_ChecksumUpdate(void *pData, uint32_t Length)
{
	return ChecksumCompute(pData, Length);
}

/**
 * @brief  This function stores the exception mask register value and disables IT
 * @note   None
 * @param  None
 * @retval None
 */
void STL_UTIL_Disable_IT(void)
{
#ifndef STL_ENABLE_IT
	/* save exception mask register */
	ExcepMaskReg = __save_disable_irq();
#else
#endif /* STL_ENABLE_IT */
}

/**
 * @brief  This function restores IT with exception mask register value previously saved
 * @note   None
 * @param  None
 * @retval None
 */
void STL_UTIL_Enable_IT(void)
{
#ifndef STL_ENABLE_IT
	/* restore exception mask register */
	__restore_irq(ExcepMaskReg);
#endif
}

/**
 * @brief  This function calculates a checksum with Exclusive OR (replace CRC calculation)
 * @note   None
 * @param  *pData       values to be calculated on pData content
 * @note   None
 * @param  Length       Length of pData on which checksum is calculated
 * @note   Length in bytes
 * @retval uint32_t     Calculated value
 */
static uint32_t ChecksumCompute(void *pData, uint32_t Length)
{
	uint32_t i, *pTmpData, checksum = 0x00000000;

	/* Length in byte to word */
	/* pData content converted to 32-bit words for computation */
	/* no need to check modulo of Lentgh due its definition */
	pTmpData = (uint32_t *)pData;
	for (i = 0UL; i < Length; i = i + sizeof(uint32_t))
	{
		checksum ^= *pTmpData;
		pTmpData++;
	}

	return checksum;
}

/**
 * @brief  This function stores SYSCFG_FPUIMR to disable the FPU bit for inexact
 * interrupt to avoid interrupt in case IXC in FPSCR is set
 * @note   None
 * @param  None
 * @retval None
 */
void STL_UTIL_Disable_FPU_IXC_IT(void)
{
#ifndef STL_ENABLE_FPU_IXC_IT
	const struct device *syscfg = DEVICE_DT_GET(DT_NODELABEL(syscfg));
	uint32_t tmp = 0;
	/* Save FPU IXC interrupt enable status */
	syscon_read(syscfg, STL_WRP_SYSCFG_FPUIMR_OFFSET, &tmp);
	SysCfgCfgr1BitIxcValue = tmp & STL_WRP_SYSCFG_FPUIMR_FPU_IE_4;
	if (SysCfgCfgr1BitIxcValue == STL_WRP_SYSCFG_FPUIMR_FPU_IE_4)
		syscon_clrbits(syscfg, STL_WRP_SYSCFG_FPUIMR_OFFSET,
			       STL_WRP_SYSCFG_FPUIMR_FPU_IE_4);
#endif /* STL_ENABLE_FPU_IXC_IT */
}

/**
 * @brief  This function restores IXC bit of SYSCFG_FPUIMR.
 * @note   None
 * @param  None
 * @retval None
 */
void STL_UTIL_Restore_FPU_IXC_IT(void)
{
#ifndef STL_ENABLE_FPU_IXC_IT
	const struct device *syscfg = DEVICE_DT_GET(DT_NODELABEL(syscfg));
	/* Restore FPU IXC interrupt enable status */
	if (SysCfgCfgr1BitIxcValue == STL_WRP_SYSCFG_FPUIMR_FPU_IE_4)
		syscon_setbits(syscfg, STL_WRP_SYSCFG_FPUIMR_OFFSET,
			       STL_WRP_SYSCFG_FPUIMR_FPU_IE_4);
#endif
}
