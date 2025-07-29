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

#ifndef __DEVICE_CFG_H__
#define __DEVICE_CFG_H__

/**
 * \file device_cfg.h
 * \brief Configuration file driver re-targeting
 *
 * \details This file can be used to add driver specific macro
 *          definitions to select which peripherals are available in the build.
 *
 * This is a default device configuration file with all peripherals enabled.
 */
/* storage */
#define STM32_FLASH_DDR
#define FLASH_DDR_SIZE			(0x4000000)	/* limit to 64MB */
#define FLASH_DDR_SECTOR_SIZE		(0x00001000)	/* 4 kB */
#define FLASH_DDR_PROGRAM_UNIT		(0x1)		/* Minimum write size */

#undef STM32_FLASH_RETRAM

#define STM32_FLASH_BKPSRAM

#define STM32_FLASH_BKPREG

#endif  /* __DEVICE_CFG_H__ */
