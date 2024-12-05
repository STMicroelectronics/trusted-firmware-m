/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2022-2024, STMicroelectronics
 */

/**
 * stm32_tamp_bkpreg_write - Write a 32 bits value to a backup register
 * identified by its ID.
 *
 * @param dev TAMP device.
 * @param reg_id Backup register ID.
 * @param value 32 bits value to write in the backup register.
 */
int stm32_tamp_bkpreg_write(const struct device *dev, unsigned int reg_id,
			    uint32_t value);

/**
 * stm32_tamp_bkpreg_read - Read a 32 bits value from a backup register
 * identified by its ID.
 *
 * @param dev TAMP device.
 * @param reg_id Backup register ID.
 * @param value 32 bits value to be read from the backup register.
 */
int stm32_tamp_bkpreg_read(const struct device *dev, unsigned int reg_id,
			   uint32_t *value);
