/*
 * Copyright (c) 2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>
#include <lib/utils_def.h>
#include <debug.h>
#include <spi_mem.h>

#define SPI_MEM_DEFAULT_SPEED_HZ 100000U

static struct spi_slave spi_slave;

static bool spi_mem_check_buswidth_req(uint8_t buswidth, bool tx)
{
	unsigned int mode = spi_slave.mode;

	switch (buswidth) {
	case 1U:
		return true;

	case 2U:
		if ((tx &&
		     (mode & (SPI_TX_DUAL | SPI_TX_QUAD | SPI_TX_OCTAL)) != 0U) ||
		    (!tx &&
		     (mode & (SPI_RX_DUAL | SPI_RX_QUAD | SPI_RX_OCTAL)) != 0U)) {
			return true;
		}
		break;

	case 4U:
		if ((tx && (mode & (SPI_TX_QUAD | SPI_TX_OCTAL)) != 0U) ||
		    (!tx && (mode & (SPI_RX_QUAD | SPI_RX_OCTAL)) != 0U)) {
			return true;
		}
		break;

	case 8U:
		if ((tx && (mode & SPI_TX_OCTAL) != 0U) ||
		    (!tx && (mode & SPI_RX_OCTAL) != 0U)) {
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

static bool spi_mem_supports_op(const struct spi_mem_op *op)
{
	if (!spi_mem_check_buswidth_req(op->cmd.buswidth, true)) {
		return false;
	}

	if ((op->addr.nbytes != 0U) &&
	    !spi_mem_check_buswidth_req(op->addr.buswidth, true)) {
		return false;
	}

	if ((op->dummy.nbytes != 0U) &&
	    !spi_mem_check_buswidth_req(op->dummy.buswidth, true)) {
		return false;
	}

	if ((op->data.nbytes != 0U) &&
	    !spi_mem_check_buswidth_req(op->data.buswidth,
					op->data.dir == SPI_MEM_DATA_OUT)) {
		return false;
	}

	return true;
}

static int spi_mem_set_speed_mode(void)
{
	const struct spi_bus_ops *ops = spi_slave.ops;
	int ret;

	ret = ops->set_speed(spi_slave.max_hz);
	if (ret != 0) {
		VERBOSE("Cannot set speed (err=%d)\n", ret);
		return ret;
	}

	ret = ops->set_mode(spi_slave.mode);
	if (ret != 0) {
		VERBOSE("Cannot set mode (err=%d)\n", ret);
		return ret;
	}

	return 0;
}

static int spi_mem_check_bus_ops(const struct spi_bus_ops *ops)
{
	bool error = false;

	if (ops->claim_bus == NULL) {
		VERBOSE("Ops claim bus is not defined\n");
		error = true;
	}

	if (ops->release_bus == NULL) {
		VERBOSE("Ops release bus is not defined\n");
		error = true;
	}

	if (ops->exec_op == NULL) {
		VERBOSE("Ops exec op is not defined\n");
		error = true;
	}

	if (ops->set_speed == NULL) {
		VERBOSE("Ops set speed is not defined\n");
		error = true;
	}

	if (ops->set_mode == NULL) {
		VERBOSE("Ops set mode is not defined\n");
		error = true;
	}

	return error ? -EINVAL : 0;
}

/*
 * spi_mem_exec_op() - Execute a memory operation.
 * @op: The memory operation to execute.
 *
 * This function first checks that @op is supported and then tries to execute
 * it.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int spi_mem_exec_op(const struct spi_mem_op *op)
{
	const struct spi_bus_ops *ops = spi_slave.ops;
	int ret;

	VERBOSE("%s: cmd:%x mode:%d.%d.%d.%d addqr:%x len:%x\n",
		__func__, op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	if (!spi_mem_supports_op(op)) {
		WARN("Error in spi_mem_support\n");
		return -ENOTSUP;
	}

	ret = ops->claim_bus(spi_slave.cs);
	if (ret != 0) {
		WARN("Error claim_bus\n");
		return ret;
	}

	ret = ops->exec_op(op);

	ops->release_bus();

	return ret;
}

/*
 * spi_mem_dirmap_read() - Read data through a direct mapping
 * @op: The memory operation to execute.
 *
 * This function reads data from a memory device using a direct mapping.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int spi_mem_dirmap_read(const struct spi_mem_op *op)
{
	const struct spi_bus_ops *ops = spi_slave.ops;
	int ret;

	VERBOSE("%s: cmd:%x mode:%d.%d.%d.%d addr:%x len:%x\n",
		__func__, op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	if (op->data.dir != SPI_MEM_DATA_IN) {
		return -EINVAL;
	}

	if (op->data.nbytes == 0U) {
		return 0;
	}

	if (ops->dirmap_read == NULL) {
		return spi_mem_exec_op(op);
	}

	if (!spi_mem_supports_op(op)) {
		WARN("Error in spi_mem_support\n");
		return -ENOTSUP;
	}

	ret = ops->claim_bus(spi_slave.cs);
	if (ret != 0) {
		WARN("Error claim_bus\n");
		return ret;
	}

	ret = ops->dirmap_read(op);

	ops->release_bus();

	return ret;
}

/*
 * spi_mem_init_slave() - SPI slave device initialization.
 * @cfg: Pointer to spi slave config.
 * @ops: The SPI bus ops defined.
 *
 * This function first checks that @ops are supported and then tries to find
 * a SPI slave device.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int spi_mem_init_slave(struct spi_slave *cfg, const struct spi_bus_ops *ops)
{
	int err;

	err = spi_mem_check_bus_ops(ops);
	if (err)
		return err;

	if (!cfg)
		return -ENODEV;

	memcpy(&spi_slave, cfg, sizeof(struct spi_slave));

	spi_slave.ops = ops;

	return spi_mem_set_speed_mode();
}
