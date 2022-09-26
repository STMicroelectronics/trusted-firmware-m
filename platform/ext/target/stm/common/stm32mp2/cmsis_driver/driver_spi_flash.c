/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>

#include <Driver_Flash.h>
#include <flash_layout.h>

#include <stm32_ospi.h>
#include <spi_nor.h>

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

/* Driver version */
#define ARM_FLASH_DRV_VERSION		ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0)

static const ARM_DRIVER_VERSION DriverVersion = {
	ARM_FLASH_API_VERSION,  /* Defined in the CMSIS Flash Driver header file */
	ARM_FLASH_DRV_VERSION
};

/**
 * \brief Flash driver capability macro definitions \ref ARM_FLASH_CAPABILITIES
 */
/* Flash Ready event generation capability values */
#define EVENT_READY_NOT_AVAILABLE	(0u)
#define EVENT_READY_AVAILABLE		(1u)
/* Chip erase capability values */
#define CHIP_ERASE_NOT_SUPPORTED	(0u)
#define CHIP_ERASE_SUPPORTED		(1u)

/**
 * Data width values for ARM_FLASH_CAPABILITIES::data_width
 * \ref ARM_FLASH_CAPABILITIES
 */
enum {
	DATA_WIDTH_8BIT   = 0u,
	DATA_WIDTH_16BIT,
	DATA_WIDTH_32BIT,
	DATA_WIDTH_ENUM_SIZE
};

static const uint32_t data_width_byte[DATA_WIDTH_ENUM_SIZE] = {
	sizeof(uint8_t),
	sizeof(uint16_t),
	sizeof(uint32_t),
};

/* Driver Capabilities */
static const ARM_FLASH_CAPABILITIES DriverCapabilities = {
	EVENT_READY_NOT_AVAILABLE,
	DATA_WIDTH_8BIT,
	CHIP_ERASE_SUPPORTED
};

/**
 * \brief Flash status macro definitions \ref ARM_FLASH_STATUS
 */
/* Busy status values of the Flash driver */
#define DRIVER_STATUS_IDLE		(0u)
#define DRIVER_STATUS_BUSY		(1u)
/* Error status values of the Flash driver */
#define DRIVER_STATUS_NO_ERROR		(0u)
#define DRIVER_STATUS_ERROR		(1u)

static ARM_FLASH_STATUS FlashStatus = {0, 0, 0};

static ARM_FLASH_INFO FlashInfo = {
	.sector_info    = NULL,     /* Uniform sector layout */
	.sector_count   = SPI_NOR_FLASH_SIZE / SPI_NOR_FLASH_SECTOR_SIZE,
	.sector_size    = SPI_NOR_FLASH_SECTOR_SIZE,
	.page_size      = SPI_NOR_FLASH_PAGE_SIZE,
	.program_unit   = 1u,       /* Minimum write size in bytes */
	.erased_value   = 0xFF
};

static bool is_range_valid(uint32_t offset)
{
	return offset < SPI_NOR_FLASH_SIZE ? true : false;
}

static bool is_write_aligned(uint32_t value)
{
	return value % FlashInfo.program_unit ? false : true;
}

static ARM_DRIVER_VERSION ARM_Flash_GetVersion(void)
{
	return DriverVersion;
}

static ARM_FLASH_CAPABILITIES ARM_Flash_GetCapabilities(void)
{
	return DriverCapabilities;
}

static int32_t ARM_Flash_Initialize(ARM_Flash_SignalEvent_t cb_event)
{
	unsigned long long size;
	unsigned int erase_size;

	if (stm32_ospi_init())
		return ARM_DRIVER_ERROR;

	if (spi_nor_init(&size, &erase_size))
		return ARM_DRIVER_ERROR;

	return ARM_DRIVER_OK;
}

static int32_t ARM_Flash_Uninitialize(void)
{
	if (stm32_ospi_deinit())
		return ARM_DRIVER_ERROR;

	return ARM_DRIVER_OK;
}

static int32_t ARM_Flash_PowerControl(ARM_POWER_STATE state)
{
	switch(state) {
	case ARM_POWER_FULL:
		/* Nothing to be done */
		return ARM_DRIVER_OK;
	case ARM_POWER_OFF:
	case ARM_POWER_LOW:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	default:
		return ARM_DRIVER_ERROR_PARAMETER;
	}
}

static int32_t ARM_Flash_ReadData(uint32_t addr, void *data, uint32_t cnt)
{
	uint32_t bytes = cnt * data_width_byte[DriverCapabilities.data_width];
	size_t length_read;
	int ret;

	FlashStatus.error = DRIVER_STATUS_NO_ERROR;

	/* Check Flash memory boundaries */
	if (!is_range_valid(addr + bytes - 1)) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	FlashStatus.busy = DRIVER_STATUS_BUSY;

	ret = spi_nor_read(addr, (uintptr_t)data, bytes, &length_read);

	FlashStatus.busy = DRIVER_STATUS_IDLE;

	if (ret || (length_read != bytes)) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR;
	}

	return cnt;
}

static int32_t ARM_Flash_ProgramData(uint32_t addr, const void *data,
				     uint32_t cnt)
{
	uint32_t bytes = cnt * data_width_byte[DriverCapabilities.data_width];
	size_t length_write;
	int ret;

	FlashStatus.error = DRIVER_STATUS_NO_ERROR;

	/* Check Flash memory boundaries */
	if (!(is_range_valid(addr + bytes - 1) &&
	    is_write_aligned(addr) &&
	    is_write_aligned(bytes))) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	FlashStatus.busy = DRIVER_STATUS_BUSY;

	ret = spi_nor_write(addr, (uintptr_t)data, bytes, &length_write);

	FlashStatus.busy = DRIVER_STATUS_IDLE;

	if (ret || (length_write != bytes)) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR;
	}

	return cnt;
}

static int32_t ARM_Flash_EraseSector(uint32_t addr)
{
	int ret;

	FlashStatus.error = DRIVER_STATUS_NO_ERROR;
	FlashStatus.busy = DRIVER_STATUS_BUSY;

	ret = spi_nor_erase(addr);

	FlashStatus.busy = DRIVER_STATUS_IDLE;

	if (ret) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR;
	}

	return ARM_DRIVER_OK;
}

static int32_t ARM_Flash_EraseChip(void)
{
	uint32_t i;
	int ret = 0;

	FlashStatus.error = DRIVER_STATUS_NO_ERROR;
        FlashStatus.busy = DRIVER_STATUS_BUSY;

	for (i = 0; i < FlashInfo.sector_count; i++) {
		ret = spi_nor_erase(FlashInfo.sector_size * i);
		if (ret)
			break;
	}

	FlashStatus.busy = DRIVER_STATUS_IDLE;

	if (ret) {
		FlashStatus.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR;
	}

	return ARM_DRIVER_OK;
}

static ARM_FLASH_STATUS ARM_Flash_GetStatus(void)
{
	return FlashStatus;
}

static ARM_FLASH_INFO * ARM_Flash_GetInfo(void)
{
	return &FlashInfo;
}

ARM_DRIVER_FLASH Driver_OSPI_SPI_NOR_FLASH = {
	ARM_Flash_GetVersion,
	ARM_Flash_GetCapabilities,
	ARM_Flash_Initialize,
	ARM_Flash_Uninitialize,
	ARM_Flash_PowerControl,
	ARM_Flash_ReadData,
	ARM_Flash_ProgramData,
	ARM_Flash_EraseSector,
	ARM_Flash_EraseChip,
	ARM_Flash_GetStatus,
	ARM_Flash_GetInfo
};
