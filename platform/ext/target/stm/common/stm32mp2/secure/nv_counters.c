/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 * Copyright (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
 * Copyright (c) 2021, STMicroelectronics.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <limits.h>
#include <stddef.h>

#include <tfm_plat_nv_counters.h>
#include <Driver_Flash.h>
#include <flash_layout.h>
#include "nv_counters.h"

/* Compilation time checks to be sure the defines are well defined */
#ifndef NV_COUNTERS_AREA_SIZE
#error "NV_COUNTERS_AREA_SIZE must be defined"
#endif

#ifndef NV_COUNTERS_AREA_ADDR
#error "NV_COUNTERS_AREA_ADDR must be defined"
#endif

#ifndef NV_COUNTERS_BACKUP_AREA_ADDR
#error "NV_COUNTERS_BACKUP_AREA_ADDR must be defined"
#endif

#ifndef NV_COUNTERS_FLASH_DRIVER
#error "NV_COUNTERS_FLASH_DRIVER must be defined"
#endif

/* End of compilation time checks to be sure the defines are well defined */

#define SECTOR_OFFSET    0
#define NV_COUNTER_SIZE  sizeof(uint32_t)
#define INIT_VALUE_SIZE  sizeof(uint32_t)
#define CHECKSUM_SIZE    sizeof(uint32_t)
#define NUM_NV_COUNTERS  ((NV_COUNTERS_AREA_SIZE - INIT_VALUE_SIZE - CHECKSUM_SIZE) \
			  / NV_COUNTER_SIZE)

#define BACKUP_ADDRESS (NV_COUNTERS_BACKUP_AREA_ADDR)
#define VALID_ADDRESS  (NV_COUNTERS_AREA_ADDR)

/* Import the CMSIS flash device driver */
extern ARM_DRIVER_FLASH NV_COUNTERS_FLASH_DRIVER;

static uint32_t calc_checksum(const uint32_t *data, size_t len)
{
	uint32_t sum = 0;

	for (uint32_t i = 0; i < len/sizeof(uint32_t); i++) {
		sum ^= data[i];
	}
	return sum;
}

static bool is_valid(const struct nv_counters_t *nv_counters)
{
	return ((nv_counters->init_value == NV_COUNTERS_INITIALIZED) &&
		(!calc_checksum(&nv_counters->checksum, sizeof(*nv_counters))));
}

static void set_checksum(struct nv_counters_t *nv_counters)
{
	uint32_t sum = calc_checksum(&nv_counters->init_value,
				     sizeof(*nv_counters)
				     - sizeof(nv_counters->checksum));

	nv_counters->checksum = sum;
}

volatile int once=0;
enum tfm_plat_err_t tfm_plat_init_nv_counter(void)
{
	int32_t  err;
	struct nv_counters_t nv_counters;

	err = NV_COUNTERS_FLASH_DRIVER.Initialize(NULL);
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	/* Read the whole sector so we can write it back to flash later */
	err = NV_COUNTERS_FLASH_DRIVER.ReadData(VALID_ADDRESS,
						&nv_counters,
						sizeof(struct nv_counters_t));
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	if (is_valid(&nv_counters)) {
		return TFM_PLAT_ERR_SUCCESS;
	}

	/* Check the backup watermark */
	err = NV_COUNTERS_FLASH_DRIVER.ReadData(BACKUP_ADDRESS,
						&nv_counters,
						sizeof(struct nv_counters_t));
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	/* Erase sector before writing to it */
	err = NV_COUNTERS_FLASH_DRIVER.EraseSector(VALID_ADDRESS);
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	if (!is_valid(&nv_counters)) {
		uint32_t i;

		/* Add watermark to indicate that NV counters have been initialized */
		nv_counters.init_value = NV_COUNTERS_INITIALIZED;

		/* Initialize all counters to 0 */
		for (i = 0; i < NUM_NV_COUNTERS; i++) {
			nv_counters.counters[i] = 0;
		}
		set_checksum(&nv_counters);
	}

	/* Write the in-memory block content after modification to flash */
	err = NV_COUNTERS_FLASH_DRIVER.ProgramData(VALID_ADDRESS,
						   &nv_counters,
						   sizeof(struct nv_counters_t));

	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_read_nv_counter(enum tfm_nv_counter_t counter_id,
					     uint32_t size, uint8_t *val)
{
	int32_t  err;
	uint32_t flash_addr = VALID_ADDRESS
		+ offsetof(struct nv_counters_t, counters)
		+ (counter_id * NV_COUNTER_SIZE);

	if (size != NV_COUNTER_SIZE) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	err = NV_COUNTERS_FLASH_DRIVER.ReadData(flash_addr, val, NV_COUNTER_SIZE);
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_set_nv_counter(enum tfm_nv_counter_t counter_id,
					    uint32_t value)
{
	int32_t  err;
	struct nv_counters_t nv_counters;

	/* Read the whole sector so we can write it back to flash later */
	err = NV_COUNTERS_FLASH_DRIVER.ReadData(VALID_ADDRESS,
						&nv_counters,
						sizeof(struct nv_counters_t));
	if (err != ARM_DRIVER_OK) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	if (value != nv_counters.counters[counter_id]) {

		if (value < nv_counters.counters[counter_id]) {
			return TFM_PLAT_ERR_INVALID_INPUT;
		}

		/* Erase backup sector */
		err = NV_COUNTERS_FLASH_DRIVER.EraseSector(BACKUP_ADDRESS);
		if (err != ARM_DRIVER_OK) {
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}

		nv_counters.counters[counter_id] = value;

		set_checksum(&nv_counters);

		/* write sector data to backup sector */
		err = NV_COUNTERS_FLASH_DRIVER.ProgramData(BACKUP_ADDRESS,
							   &nv_counters,
							   sizeof(struct nv_counters_t));
		if (err != ARM_DRIVER_OK) {
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}

		/* Erase sector before writing to it */
		err = NV_COUNTERS_FLASH_DRIVER.EraseSector(VALID_ADDRESS);
		if (err != ARM_DRIVER_OK) {
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}

		/* Write the in-memory block content after modification to flash */
		err = NV_COUNTERS_FLASH_DRIVER.ProgramData(VALID_ADDRESS,
							   &nv_counters,
							   sizeof(struct nv_counters_t));
		if (err != ARM_DRIVER_OK) {
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_increment_nv_counter(
						  enum tfm_nv_counter_t counter_id)
{
	uint32_t security_cnt;
	enum tfm_plat_err_t err;

	err = tfm_plat_read_nv_counter(counter_id,
				       sizeof(security_cnt),
				       (uint8_t *)&security_cnt);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	if (security_cnt == UINT32_MAX) {
		return TFM_PLAT_ERR_MAX_VALUE;
	}

	return tfm_plat_set_nv_counter(counter_id, security_cnt + 1u);
}
