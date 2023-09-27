/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdio.h>

#include <device.h>
#include <uart.h>
#include <uart_stdout.h>

static const struct device *dev_console = DEVICE_DT_GET(DT_CHOSEN(stdout_device));

const struct uart_config uart_console_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

int stdio_output_string(const unsigned char *str, uint32_t len)
{
	int32_t err;

	if (!device_is_ready(dev_console))
		return -ENODEV;

	err = uart_tx(dev_console, str, len, 0);
	if (err)
		return err;

	return len;
}

/* Redirects printf to STDIO_DRIVER in case of ARMCLANG*/
#if defined(__ARMCC_VERSION)
/* Struct FILE is implemented in stdio.h. Used to redirect printf to
 * STDIO_DRIVER
 */
FILE __stdout;
/* __ARMCC_VERSION is only defined starting from Arm compiler version 6 */
int fputc(int ch, FILE *f)
{
    (void)f;

    /* Send byte to USART */
    (void)stdio_output_string((const unsigned char *)&ch, 1);

    /* Return character written */
    return ch;
}
#elif defined(__GNUC__)
/* Redirects printf to STDIO_DRIVER in case of GNUARM */
int _write(int fd, char *str, int len)
{
    (void)fd;

    /* Send string and return the number of characters written */
    return stdio_output_string((const unsigned char *)str, (uint32_t)len);
}
#elif defined(__ICCARM__)
int putchar(int ch)
{
    /* Send byte to USART */
    (void)stdio_output_string((const unsigned char *)&ch, 1);

    /* Return character written */
    return ch;
}
#endif

/* stdio_init is called by TFM core,
 * but this device depend on platform ressources and their init
 */
void stdio_init(void)
{
}

void stdio_uninit(void)
{
}

static int uart_console_init(void)
{
	if (!device_is_ready(dev_console))
		return -ENODEV;

	uart_configure(dev_console, &uart_console_cfg);

	return 0;
}
SYS_INIT(uart_console_init, CORE, 11);
