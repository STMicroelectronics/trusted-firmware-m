/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * This driver allows to read/write/erase on devices which are directly
 * memory mapped like ddr/backupram/sram ...
 */
#include <string.h>
#include <stdint.h>
#include <lib/utils_def.h>
#include <Driver_Flash.h>
#include <flash_layout.h>
#include <device_cfg.h>

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

/* Driver version */
#define ARM_FLASH_DRV_VERSION      ARM_DRIVER_VERSION_MAJOR_MINOR(1, 1)

/* Flash busy values flash status  \ref ARM_FLASH_STATUS */
enum {
	DRIVER_STATUS_IDLE = 0u,
	DRIVER_STATUS_BUSY
};

/* Flash error values flash status  \ref ARM_FLASH_STATUS */
enum {
	DRIVER_STATUS_NO_ERROR = 0u,
	DRIVER_STATUS_ERROR
};

/*
 * ARM FLASH device structure
 */
typedef struct {
	const uint32_t memory_base;   /*!< FLASH memory base address */
	ARM_FLASH_INFO *data;         /*!< FLASH data */
	ARM_FLASH_STATUS status;
} arm_flash_dev_t;

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
	ARM_FLASH_API_VERSION,
	ARM_FLASH_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_FLASH_CAPABILITIES DriverCapabilities = {
	0, /* event_ready */
	2, /* data_width = 0:8-bit, 1:16-bit, 2:32-bit */
	1  /* erase_chip */
};

static __maybe_unused bool is_range_valid(arm_flash_dev_t *flash_dev,
					  uint32_t offset)
{
	uint32_t flash_sz = (flash_dev->data->sector_count * flash_dev->data->sector_size);

	return offset < flash_sz ? true : false;
}

static __maybe_unused ARM_DRIVER_VERSION ARM_Flash_GetVersion(void)
{
	return DriverVersion;
}

static __maybe_unused ARM_FLASH_CAPABILITIES ARM_Flash_GetCapabilities(void)
{
	return DriverCapabilities;
}

static __maybe_unused int32_t ARM_Flash_X_Initialize(arm_flash_dev_t *flash_dev,
						     ARM_Flash_SignalEvent_t cb_event)
{
	ARG_UNUSED(cb_event);
	ARG_UNUSED(flash_dev);

	/* Nothing to be done */
	return ARM_DRIVER_OK;
}

static __maybe_unused int32_t ARM_Flash_X_Uninitialize(arm_flash_dev_t *flash_dev)
{
	ARG_UNUSED(flash_dev);

	/* Nothing to be done */
	return ARM_DRIVER_OK;
}

static __maybe_unused int32_t ARM_Flash_X_PowerControl(arm_flash_dev_t *flash_dev,
						       ARM_POWER_STATE state)
{
	ARG_UNUSED(flash_dev);

	switch (state) {
	case ARM_POWER_FULL:
		/* Nothing to be done */
		return ARM_DRIVER_OK;
		break;

	case ARM_POWER_OFF:
	case ARM_POWER_LOW:
	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}
}

static __maybe_unused int32_t ARM_Flash_X_ReadData(arm_flash_dev_t *flash_dev,
						   uint32_t addr, void *data, uint32_t cnt)
{
	uint32_t start_addr = flash_dev->memory_base + addr;
	uint32_t last_ofst = addr + cnt - 1;

	flash_dev->status.error = DRIVER_STATUS_NO_ERROR;

	/* Check flash memory boundaries */
	if (!is_range_valid(flash_dev, last_ofst)) {
		flash_dev->status.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	flash_dev->status.busy = DRIVER_STATUS_BUSY;

	/* Flash interface just emulated over ddr, use memcpy */
	memcpy(data, (void *)start_addr, cnt);

	flash_dev->status.busy = DRIVER_STATUS_IDLE;

	return ARM_DRIVER_OK;
}

static __maybe_unused int32_t ARM_Flash_X_ProgramData(arm_flash_dev_t *flash_dev,
						      uint32_t addr, const void *data, uint32_t cnt)
{
	uint32_t start_addr = flash_dev->memory_base + addr;
	uint32_t last_ofst = addr + cnt - 1;

	flash_dev->status.error = DRIVER_STATUS_NO_ERROR;

	/* Check flash memory boundaries */
	if (!is_range_valid(flash_dev, last_ofst)) {
		flash_dev->status.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	flash_dev->status.busy = DRIVER_STATUS_BUSY;

	/* Flash interface just emulated over ddr, use memcpy */
	memcpy((void *)start_addr, data, cnt);

	flash_dev->status.busy = DRIVER_STATUS_IDLE;

	return ARM_DRIVER_OK;
}

static __maybe_unused int32_t ARM_Flash_X_EraseSector(arm_flash_dev_t *flash_dev,
						      uint32_t addr)
{
	uint32_t start_addr = flash_dev->memory_base + addr;
	uint32_t last_ofst = addr + flash_dev->data->sector_size - 1;

	flash_dev->status.error = DRIVER_STATUS_NO_ERROR;

	if (!is_range_valid(flash_dev, last_ofst)) {
		flash_dev->status.error = DRIVER_STATUS_ERROR;
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	flash_dev->status.busy = DRIVER_STATUS_BUSY;

	/* Flash interface just emulated over ddr, use memset */
	memset((void *)start_addr,
	       flash_dev->data->erased_value,
	       flash_dev->data->sector_size);

	flash_dev->status.busy = DRIVER_STATUS_IDLE;

	return ARM_DRIVER_OK;
}

static __maybe_unused int32_t ARM_Flash_X_EraseChip(arm_flash_dev_t *flash_dev)
{
	uint32_t i;
	uint32_t addr = flash_dev->memory_base;
	int32_t rc = ARM_DRIVER_ERROR_UNSUPPORTED;

	flash_dev->status.busy = DRIVER_STATUS_BUSY;
	flash_dev->status.error = DRIVER_STATUS_NO_ERROR;

	/* Check driver capability erase_chip bit */
	if (DriverCapabilities.erase_chip == 1) {
		for (i = 0; i < flash_dev->data->sector_count; i++) {
			/* Flash interface just emulated over MRAM, use memset */
			memset((void *)addr,
			       flash_dev->data->erased_value,
			       flash_dev->data->sector_size);

			addr += flash_dev->data->sector_size;
			rc = ARM_DRIVER_OK;
		}
	}

	flash_dev->status.busy = DRIVER_STATUS_IDLE;

	return rc;
}

static __maybe_unused ARM_FLASH_STATUS ARM_Flash_X_GetStatus(arm_flash_dev_t *flash_dev)
{
	return flash_dev->status;
}

static __maybe_unused ARM_FLASH_INFO * ARM_Flash_X_GetInfo(arm_flash_dev_t *flash_dev)
{
	return flash_dev->data;
}

/* Per-FLASH macros */
/* Each instance must define:
 * - Driver_FLASH_XX
 * - FLASH_XX_SIZE
 * - FLASH_XX_SECTOR_SIZE
 * - FLASH_XX_PROGRAM_UNIT
 */
#define DRIVER_FLASH_X(devname)									\
static ARM_FLASH_INFO FLASH_##devname##_DATA = {						\
	.sector_info  = NULL,                  /* Uniform sector layout */			\
	.sector_count = FLASH_##devname##_SIZE / FLASH_##devname##_SECTOR_SIZE,			\
	.sector_size  = FLASH_##devname##_SECTOR_SIZE,						\
	.program_unit = FLASH_##devname##_PROGRAM_UNIT,						\
	.erased_value = 0x0,									\
};												\
												\
static arm_flash_dev_t FLASH_##devname##_DEV = {						\
	.memory_base = FLASH_##devname##_BASE,							\
	.data = &(FLASH_##devname##_DATA),							\
	.status = {										\
		.busy = DRIVER_STATUS_IDLE,							\
		.error = DRIVER_STATUS_NO_ERROR,						\
		.reserved = 0,									\
	},											\
};												\
												\
static int32_t ARM_Flash_##devname##_Initialize(ARM_Flash_SignalEvent_t cb_event)		\
{												\
	return ARM_Flash_X_Initialize(&FLASH_##devname##_DEV, cb_event);			\
}												\
												\
static int32_t ARM_Flash_##devname##_Uninitialize(void)						\
{												\
	return ARM_Flash_X_Uninitialize(&FLASH_##devname##_DEV);				\
}												\
												\
static int32_t ARM_Flash_##devname##_PowerControl(ARM_POWER_STATE state)			\
{												\
	return ARM_Flash_X_PowerControl(&FLASH_##devname##_DEV, state);				\
}												\
												\
static int32_t ARM_Flash_##devname##_ReadData(uint32_t addr, void *data, uint32_t cnt)		\
{												\
	return ARM_Flash_X_ReadData(&FLASH_##devname##_DEV, addr, data, cnt);			\
}												\
												\
static int32_t ARM_Flash_##devname##_ProgramData(uint32_t addr,					\
						 const void *data, uint32_t cnt)		\
{												\
	return ARM_Flash_X_ProgramData(&FLASH_##devname##_DEV, addr, data, cnt);		\
}												\
												\
static int32_t ARM_Flash_##devname##_EraseSector(uint32_t addr)					\
{												\
	return ARM_Flash_X_EraseSector(&FLASH_##devname##_DEV, addr);				\
}												\
												\
static int32_t ARM_Flash_##devname##_EraseChip(void)						\
{												\
	return ARM_Flash_X_EraseChip(&FLASH_##devname##_DEV);					\
}												\
												\
static ARM_FLASH_STATUS ARM_Flash_##devname##_GetStatus(void)					\
{												\
	return ARM_Flash_X_GetStatus(&FLASH_##devname##_DEV);					\
}												\
												\
static ARM_FLASH_INFO * ARM_Flash_##devname##_GetInfo(void)					\
{												\
	return ARM_Flash_X_GetInfo(&FLASH_##devname##_DEV);					\
}												\
												\
extern ARM_DRIVER_FLASH Driver_FLASH_##devname;							\
ARM_DRIVER_FLASH Driver_FLASH_##devname = {							\
	ARM_Flash_GetVersion,									\
	ARM_Flash_GetCapabilities,								\
	ARM_Flash_##devname##_Initialize,							\
	ARM_Flash_##devname##_Uninitialize,							\
	ARM_Flash_##devname##_PowerControl,							\
	ARM_Flash_##devname##_ReadData,								\
	ARM_Flash_##devname##_ProgramData,							\
	ARM_Flash_##devname##_EraseSector,							\
	ARM_Flash_##devname##_EraseChip,							\
	ARM_Flash_##devname##_GetStatus,							\
	ARM_Flash_##devname##_GetInfo								\
}

#ifdef STM32_FLASH_DDR
DRIVER_FLASH_X(DDR);
#endif
