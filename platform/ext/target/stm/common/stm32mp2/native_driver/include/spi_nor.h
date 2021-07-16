/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRIVERS_SPI_NOR_H
#define DRIVERS_SPI_NOR_H

#include <spi_mem.h>

/* OPCODE */
#define SPI_NOR_OP_WREN		0x06U	/* Write enable */
#define SPI_NOR_OP_WRDI		0x04U	/* Write disable */
#define SPI_NOR_OP_WRSR		0x01U	/* Write status register 1 byte */
#define SPI_NOR_OP_READ_ID	0x9FU	/* Read JEDEC ID */
#define SPI_NOR_OP_READ_CR	0x35U	/* Read configuration register */
#define SPI_NOR_OP_READ_SR	0x05U	/* Read status register */
#define SPI_NOR_OP_READ_FSR	0x70U	/* Read flag status register */
#define SPINOR_OP_RDEAR		0xC8U	/* Read Extended Address Register */
#define SPINOR_OP_WREAR		0xC5U	/* Write Extended Address Register */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR		0x17U	/* Bank register write */
#define SPINOR_OP_BRRD		0x16U	/* Bank register read */

#define SPI_NOR_OP_READ		0x03U	/* Read data bytes (low frequency) */
#define SPI_NOR_OP_READ_FAST	0x0BU	/* Read data bytes (high frequency) */
#define SPI_NOR_OP_READ_1_1_2	0x3BU	/* Read data bytes (Dual Output SPI) */
#define SPI_NOR_OP_READ_1_2_2	0xBBU	/* Read data bytes (Dual I/O SPI) */
#define SPI_NOR_OP_READ_1_1_4	0x6BU	/* Read data bytes (Quad Output SPI) */
#define SPI_NOR_OP_READ_1_4_4	0xEBU	/* Read data bytes (Quad I/O SPI) */
#define SPI_NOR_OP_READ_1_1_8	0x8BU	/* Read data bytes (Octal Output SPI) */
#define SPI_NOR_OP_READ_1_8_8	0xCBU	/* Read data bytes (Octal I/O SPI) */

#define SPI_NOR_OP_WRITE	0x02U	/* Write data bytes */
#define SPI_NOR_OP_WRITE_1_1_4	0x32U	/* Write data bytes (Quad page program) */
#define SPI_NOR_OP_WRITE_1_4_4	0x38U	/* Write data bytes (Quad page program) */
#define SPI_NOR_OP_WRITE_1_1_8	0x82U	/* Write data bytes (Octal page program) */
#define SPI_NOR_OP_WRITE_1_8_8	0xC2U	/* Write data bytes (Octal page program) */

#define SPI_NOR_OP_SE		0x20U	/* Erase a sector */
#define SPI_NOR_OP_BE		0xD8U	/* Erase a block */

/* Flags for NOR specific configuration */
#define SPI_NOR_USE_FSR		BIT(0)
#define SPI_NOR_USE_BANK	BIT(1)

struct nor_device {
	struct spi_mem_op read_op;
	struct spi_mem_op write_op;
	struct spi_mem_op erase_op;
	uint32_t size;
	uint32_t erase_size;
	uint32_t write_size;
	uint32_t flags;
	uint8_t selected_bank;
	uint8_t bank_write_cmd;
	uint8_t bank_read_cmd;
};

int spi_nor_read(unsigned int offset, uintptr_t buffer, size_t length,
		 size_t *length_read);
int spi_nor_write(unsigned int offset, uintptr_t buffer, size_t length,
		  size_t *length_write);
int spi_nor_erase(unsigned int offset);
int spi_nor_init(unsigned long long *device_size, unsigned int *erase_size);

/*
 * Platform can implement this to override default NOR instance configuration.
 *
 * @device: target NOR instance.
 * Return 0 on success, negative value otherwise.
 */
int spi_nor_get_config(struct nor_device *device);

#endif /* DRIVERS_SPI_NOR_H */
