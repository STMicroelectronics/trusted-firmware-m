/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This is based on psoc64 driver
 * platform/ext/target/cypress/psoc64/CMSIS_Driver/Driver_USART.c
 */

#include <Driver_USART.h>
#include <cmsis.h>
#include <device_cfg.h>

#include <stm32_uart.h>

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  (void)arg
#endif

/* Driver version */
#define ARM_USART_DRV_VERSION  ARM_DRIVER_VERSION_MAJOR_MINOR(2, 2)

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
	ARM_USART_API_VERSION,
	ARM_USART_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_USART_CAPABILITIES DriverCapabilities = {
	1, /* supports UART (Asynchronous) mode */
	0, /* supports Synchronous Master mode */
	0, /* supports Synchronous Slave mode */
	0, /* supports UART Single-wire mode */
	0, /* supports UART IrDA mode */
	0, /* supports UART Smart Card mode */
	0, /* Smart Card Clock generator available */
	0, /* RTS Flow Control available */
	0, /* CTS Flow Control available */
	0, /* Transmit completed event: \ref ARM_USARTx_EVENT_TX_COMPLETE */
	0, /* Signal receive character timeout event: \ref ARM_USARTx_EVENT_RX_TIMEOUT */
	0, /* RTS Line: 0=not available, 1=available */
	0, /* CTS Line: 0=not available, 1=available */
	0, /* DTR Line: 0=not available, 1=available */
	0, /* DSR Line: 0=not available, 1=available */
	0, /* DCD Line: 0=not available, 1=available */
	0, /* RI Line: 0=not available, 1=available */
	0, /* Signal CTS change event: \ref ARM_USARTx_EVENT_CTS */
	0, /* Signal DSR change event: \ref ARM_USARTx_EVENT_DSR */
	0, /* Signal DCD change event: \ref ARM_USARTx_EVENT_DCD */
	0, /* Signal RI change event: \ref ARM_USARTx_EVENT_RI */
	0  /* Reserved */
};

static ARM_DRIVER_VERSION ARM_USART_GetVersion(void)
{
	return DriverVersion;
}

static ARM_USART_CAPABILITIES ARM_USART_GetCapabilities(void)
{
	return DriverCapabilities;
}

struct stm32_uart_handle_s stm32_uart_dft_cfg = {
	.init.baud_rate = 115200,
	.init.word_length = UART_WORDLENGTH_8B,
	.init.stop_bits = UART_STOPBITS_1,
	.init.parity = UART_PARITY_NONE,
	.init.mode = UART_MODE_TX_RX,
	.init.hw_flow_control = UART_HWCONTROL_NONE,
	.init.over_sampling = UART_OVERSAMPLING_8,
	.init.one_bit_sampling = UART_ONE_BIT_SAMPLE_DISABLE,
	.init.prescaler = UART_PRESCALER_DIV1,
};

typedef struct {
	struct stm32_uart_handle_s *huart;  /* UART device structure */
	uint32_t tx_nbr_bytes;				/* Number of bytes transfered */
	uint32_t rx_nbr_bytes;				/* Number of bytes recevied */
	ARM_USART_SignalEvent_t cb_event;	/* Callback function for events */
	bool initialized;					/* init flag */
} UARTx_Resources;

static int32_t ARM_USARTx_Initialize(UARTx_Resources* uart_dev)
{
	if (stm32_uart_init())
		return ARM_DRIVER_ERROR;

	uart_dev->initialized = true;

	return ARM_DRIVER_OK;
}

static uint32_t ARM_USARTx_Uninitialize(UARTx_Resources* uart_dev)
{
	uart_dev->initialized = false;

	return ARM_DRIVER_OK;
}

static int32_t ARM_USARTx_PowerControl(UARTx_Resources* uart_dev,
		ARM_POWER_STATE state)
{
	return ARM_DRIVER_OK;
}

static int32_t ARM_USARTx_Send(UARTx_Resources* uart_dev, const void *data,
		uint32_t num)
{
	if (!uart_dev->initialized)
		return ARM_DRIVER_ERROR;

	if ((data == NULL) || (num == 0U))
		return ARM_DRIVER_ERROR_PARAMETER;

	if (stm32_uart_transmit((uint8_t *)data, num, 0) != UART_OK)
		return ARM_DRIVER_ERROR;

	uart_dev->tx_nbr_bytes = num;

	return ARM_DRIVER_OK;
}

static int32_t ARM_USARTx_Receive(UARTx_Resources* uart_dev,
		void *data, uint32_t num)
{
	if (!uart_dev->initialized)
		return ARM_DRIVER_ERROR;

	if ((data == NULL) || (num == 0U))
		return ARM_DRIVER_ERROR_PARAMETER;

	if (stm32_uart_receive((uint8_t *)data, num, 0) != UART_OK)
		return ARM_DRIVER_ERROR;

	uart_dev->rx_nbr_bytes = num;

	return ARM_DRIVER_OK;
}

static int32_t ARM_USARTx_Transfer(UARTx_Resources* uart_dev,
		const void *data_out, void *data_in,
		uint32_t num)
{
	ARG_UNUSED(uart_dev);
	ARG_UNUSED(data_out);
	ARG_UNUSED(data_in);
	ARG_UNUSED(num);

	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static uint32_t ARM_USARTx_GetTxCount(UARTx_Resources* uart_dev)
{
	return uart_dev->tx_nbr_bytes;
}

static uint32_t ARM_USARTx_GetRxCount(UARTx_Resources* uart_dev)
{
	return uart_dev->rx_nbr_bytes;
}

static uint32_t USARTx_SetDataBits(struct stm32_uart_handle_s *huart, int32_t control)
{
	switch (control & ARM_USART_DATA_BITS_Msk) {
	case ARM_USART_DATA_BITS_7:
		huart->init.word_length = UART_WORDLENGTH_7B;
		break;

	case ARM_USART_DATA_BITS_8:
		huart->init.word_length = UART_WORDLENGTH_8B;
		break;

	case ARM_USART_DATA_BITS_9:
		huart->init.word_length = UART_WORDLENGTH_9B;
		break;

	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}

	return ARM_DRIVER_OK;
}


static uint32_t USARTx_SetParity(struct stm32_uart_handle_s *huart, int32_t control)
{
	switch (control & ARM_USART_PARITY_Msk) {
	case ARM_USART_PARITY_NONE:
		huart->init.parity = UART_PARITY_NONE;
		break;

	case ARM_USART_PARITY_EVEN:
		huart->init.parity = UART_PARITY_EVEN;
		break;

	case ARM_USART_PARITY_ODD:
		huart->init.parity = UART_PARITY_ODD;
		break;

	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}

	return ARM_DRIVER_OK;
}

static uint32_t USARTx_SetStopBits(struct stm32_uart_handle_s *huart, int32_t control)
{
	switch (control & ARM_USART_STOP_BITS_Msk) {
	case ARM_USART_STOP_BITS_0_5:
		huart->init.stop_bits = UART_STOPBITS_0_5;
		break;

	case ARM_USART_STOP_BITS_1:
		huart->init.stop_bits = UART_STOPBITS_1;
		break;

	case ARM_USART_STOP_BITS_1_5:
		huart->init.stop_bits = UART_STOPBITS_1_5;
		break;

	case ARM_USART_STOP_BITS_2:
		huart->init.stop_bits = UART_STOPBITS_2;
		break;

	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}

	return ARM_DRIVER_OK;
}

static uint32_t USARTx_SetFlowControl(struct stm32_uart_handle_s *huart, int32_t control)
{
	switch (control & ARM_USART_FLOW_CONTROL_Msk) {
	case ARM_USART_FLOW_CONTROL_NONE:
		huart->init.hw_flow_control = UART_HWCONTROL_NONE;
		break;

	case ARM_USART_FLOW_CONTROL_RTS:
		huart->init.hw_flow_control = UART_HWCONTROL_RTS;
		break;

	case ARM_USART_FLOW_CONTROL_CTS:
		huart->init.hw_flow_control = UART_HWCONTROL_CTS;
		break;

	case ARM_USART_FLOW_CONTROL_RTS_CTS:
		huart->init.hw_flow_control = UART_HWCONTROL_RTS_CTS;
		break;

	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}

	return ARM_DRIVER_OK;
}

static int32_t ARM_USARTx_Control(UARTx_Resources* uart_dev, uint32_t control,
		uint32_t arg)
{
	struct stm32_uart_handle_s *huart = uart_dev->huart;
	uint32_t ret;

	ret = USARTx_SetDataBits(huart, control);
	if (ret != ARM_DRIVER_OK)
		return ret;

	ret = USARTx_SetParity(huart, control);
	if (ret != ARM_DRIVER_OK)
		return ret;

	ret = USARTx_SetStopBits(huart, control);
	if (ret != ARM_DRIVER_OK)
		return ret;

	ret = USARTx_SetFlowControl(huart, control);
	if (ret != ARM_DRIVER_OK)
		return ret;

	stm32_uart_set_config(huart);

	return ARM_DRIVER_OK;
}

static ARM_USART_STATUS ARM_USARTx_GetStatus(UARTx_Resources* uart_dev)
{
	ARM_USART_STATUS status = {0, 0, 0, 0, 0, 0, 0, 0};
	return status;
}

static int32_t ARM_USARTx_SetModemControl(UARTx_Resources* uart_dev,
		ARM_USART_MODEM_CONTROL control)
{
	ARG_UNUSED(control);
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static ARM_USART_MODEM_STATUS ARM_USARTx_GetModemStatus(UARTx_Resources* uart_dev)
{
	ARM_USART_MODEM_STATUS modem_status = {0, 0, 0, 0, 0};
	return modem_status;
}

/* Per-UART macros */
/*    .huart.clk = PeripheralClock, \ */
#define DEFINE_UARTX(N) static UARTx_Resources USART##N##_DEV = { \
	.huart = &stm32_uart_dft_cfg, \
	.tx_nbr_bytes = 0, \
	.rx_nbr_bytes = 0, \
	.cb_event = NULL, \
}; \
\
static int32_t ARM_USART##N##_Initialize(ARM_USART_SignalEvent_t cb_event) \
{ \
	USART##N##_DEV.cb_event = cb_event; \
	return ARM_USARTx_Initialize(&USART##N##_DEV); \
} \
\
static int32_t ARM_USART##N##_Uninitialize(void) \
{ \
	return ARM_USARTx_Uninitialize(&USART##N##_DEV); \
} \
\
static int32_t ARM_USART##N##_PowerControl(ARM_POWER_STATE state) \
{ \
	return ARM_USARTx_PowerControl(&USART##N##_DEV, state); \
} \
\
static int32_t ARM_USART##N##_Send(const void *data, uint32_t num) \
{ \
	return ARM_USARTx_Send(&USART##N##_DEV, data, num); \
} \
\
static int32_t ARM_USART##N##_Receive(void *data, uint32_t num) \
{ \
	return ARM_USARTx_Receive(&USART##N##_DEV, data, num); \
} \
\
static int32_t ARM_USART##N##_Transfer(const void *data_out, void *data_in, \
		uint32_t num) \
{ \
	return ARM_USARTx_Transfer(&USART##N##_DEV, data_out, data_in, num); \
} \
\
static uint32_t ARM_USART##N##_GetTxCount(void) \
{ \
	return ARM_USARTx_GetTxCount(&USART##N##_DEV); \
} \
\
static uint32_t ARM_USART##N##_GetRxCount(void) \
{ \
	return ARM_USARTx_GetRxCount(&USART##N##_DEV); \
} \
static int32_t ARM_USART##N##_Control(uint32_t control, uint32_t arg) \
{ \
	return ARM_USARTx_Control(&USART##N##_DEV, control, arg); \
} \
\
static ARM_USART_STATUS ARM_USART##N##_GetStatus(void) \
{ \
	return ARM_USARTx_GetStatus(&USART##N##_DEV); \
} \
\
static int32_t ARM_USART##N##_SetModemControl(ARM_USART_MODEM_CONTROL control) \
{ \
	return ARM_USARTx_SetModemControl(&USART##N##_DEV, control); \
} \
\
static ARM_USART_MODEM_STATUS ARM_USART##N##_GetModemStatus(void) \
{ \
	return ARM_USARTx_GetModemStatus(&USART##N##_DEV); \
} \
\
extern ARM_DRIVER_USART Driver_USART##N; \
ARM_DRIVER_USART Driver_USART##N = { \
	ARM_USART_GetVersion, \
	ARM_USART_GetCapabilities, \
	ARM_USART##N##_Initialize, \
	ARM_USART##N##_Uninitialize, \
	ARM_USART##N##_PowerControl, \
	ARM_USART##N##_Send, \
	ARM_USART##N##_Receive, \
	ARM_USART##N##_Transfer, \
	ARM_USART##N##_GetTxCount, \
	ARM_USART##N##_GetRxCount, \
	ARM_USART##N##_Control, \
	ARM_USART##N##_GetStatus, \
	ARM_USART##N##_SetModemControl, \
	ARM_USART##N##_GetModemStatus \
};

#if (STM_USART1)
DEFINE_UARTX(1)
#endif

#if (STM_USART2)
DEFINE_UARTX(2)
#endif

#if (STM_USART3)
DEFINE_UARTX(3)
#endif

#if (STM_USART4)
DEFINE_UARTX(4)
#endif

#if (STM_USART5)
DEFINE_UARTX(5)
#endif
