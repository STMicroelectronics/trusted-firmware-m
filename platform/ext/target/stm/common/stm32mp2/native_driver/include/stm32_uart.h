/*
 * Copyright (c) 2015-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_UART_H
#define STM32_UART_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef TFM_ENV
#include <stm32_gpio.h>
#else
#include <drivers/st/stm32_gpio.h>
#endif

/* Return status */
#define UART_OK					0U
#define UART_ERROR				0xFFFFFFFFU
#define UART_BUSY				0xFFFFFFFEU
#define UART_TIMEOUT				0xFFFFFFFDU

/* UART word length */
#define UART_WORDLENGTH_7B			USART_CR1_M1
#define UART_WORDLENGTH_8B			0x00000000U
#define UART_WORDLENGTH_9B			USART_CR1_M0

/* UART number of stop bits */
#define UART_STOPBITS_0_5			USART_CR2_STOP_0
#define UART_STOPBITS_1				0x00000000U
#define UART_STOPBITS_1_5			(USART_CR2_STOP_0 | \
						 USART_CR2_STOP_1)
#define UART_STOPBITS_2				USART_CR2_STOP_1

/* UART parity */
#define UART_PARITY_NONE			0x00000000U
#define UART_PARITY_EVEN			USART_CR1_PCE
#define UART_PARITY_ODD				(USART_CR1_PCE | USART_CR1_PS)

/* UART transfer mode */
#define UART_MODE_RX				USART_CR1_RE
#define UART_MODE_TX				USART_CR1_TE
#define UART_MODE_TX_RX				(USART_CR1_TE | USART_CR1_RE)

/* UART hardware flow control */
#define UART_HWCONTROL_NONE			0x00000000U
#define UART_HWCONTROL_RTS			USART_CR3_RTSE
#define UART_HWCONTROL_CTS			USART_CR3_CTSE
#define UART_HWCONTROL_RTS_CTS			(USART_CR3_RTSE | \
						 USART_CR3_CTSE)

/* UART over sampling */
#define UART_OVERSAMPLING_16			0x00000000U
#define UART_OVERSAMPLING_8			USART_CR1_OVER8

/* UART One Bit Sampling Method */
#define UART_ONE_BIT_SAMPLE_DISABLE         0x00000000U
#define UART_ONE_BIT_SAMPLE_ENABLE          USART_CR3_ONEBIT

/* UART prescaler */
#define UART_PRESCALER_DIV1			0x00000000U
#define UART_PRESCALER_DIV2			0x00000001U
#define UART_PRESCALER_DIV4			0x00000002U
#define UART_PRESCALER_DIV6			0x00000003U
#define UART_PRESCALER_DIV8			0x00000004U
#define UART_PRESCALER_DIV10			0x00000005U
#define UART_PRESCALER_DIV12			0x00000006U
#define UART_PRESCALER_DIV16			0x00000007U
#define UART_PRESCALER_DIV32			0x00000008U
#define UART_PRESCALER_DIV64			0x00000009U
#define UART_PRESCALER_DIV128			0x0000000AU
#define UART_PRESCALER_DIV256			0x0000000BU
#define UART_PRESCALER_MAX			UART_PRESCALER_DIV256

/* UART TXFIFO threshold level */
#define UART_TXFIFO_THRESHOLD_1EIGHTHFULL	0x00000000U
#define UART_TXFIFO_THRESHOLD_1QUARTERFUL	USART_CR3_TXFTCFG_0
#define UART_TXFIFO_THRESHOLD_HALFFULL		USART_CR3_TXFTCFG_1
#define UART_TXFIFO_THRESHOLD_3QUARTERSFULL	(USART_CR3_TXFTCFG_0 | \
						 USART_CR3_TXFTCFG_1)
#define UART_TXFIFO_THRESHOLD_7EIGHTHFULL	USART_CR3_TXFTCFG_2
#define UART_TXFIFO_THRESHOLD_EMPTY		(USART_CR3_TXFTCFG_2 | \
						 USART_CR3_TXFTCFG_0)

/* UART RXFIFO threshold level */
#define UART_RXFIFO_THRESHOLD_1EIGHTHFULL	0x00000000U
#define UART_RXFIFO_THRESHOLD_1QUARTERFULL	USART_CR3_RXFTCFG_0
#define UART_RXFIFO_THRESHOLD_HALFFULL		USART_CR3_RXFTCFG_1
#define UART_RXFIFO_THRESHOLD_3QUARTERSFULL	(USART_CR3_RXFTCFG_0 | \
						 USART_CR3_RXFTCFG_1)
#define UART_RXFIFO_THRESHOLD_7EIGHTHFULL	USART_CR3_RXFTCFG_2
#define UART_RXFIFO_THRESHOLD_FULL		(USART_CR3_RXFTCFG_2 | \
						 USART_CR3_RXFTCFG_0)

/* UART advanced feature initialization type */
#define UART_ADVFEATURE_NO_INIT			0U
#define UART_ADVFEATURE_TXINVERT_INIT		BIT(0)
#define UART_ADVFEATURE_RXINVERT_INIT		BIT(1)
#define UART_ADVFEATURE_DATAINVERT_INIT		BIT(2)
#define UART_ADVFEATURE_SWAP_INIT		BIT(3)
#define UART_ADVFEATURE_RXOVERRUNDISABLE_INIT	BIT(4)
#define UART_ADVFEATURE_DMADISABLEONERROR_INIT	BIT(5)
#define UART_ADVFEATURE_AUTOBAUDRATE_INIT	BIT(6)
#define UART_ADVFEATURE_MSBFIRST_INIT		BIT(7)

/* UART advanced feature autobaud rate mode */
#define UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT		0x00000000U
#define UART_ADVFEATURE_AUTOBAUDRATE_ONFALLINGEDGE	USART_CR2_ABRMODE_0
#define UART_ADVFEATURE_AUTOBAUDRATE_ON0X7FFRAME	USART_CR2_ABRMODE_1
#define UART_ADVFEATURE_AUTOBAUDRATE_ON0X55FRAME	USART_CR2_ABRMODE

/* UART polling-based communications time-out value */
#define UART_TIMEOUT_US				20000U

struct stm32_uart_init_s {
	uint32_t baud_rate;		/*
					 * Configures the UART communication
					 * baud rate.
					 */

	uint32_t word_length;		/*
					 * Specifies the number of data bits
					 * transmitted or received in a frame.
					 * This parameter can be a value of
					 * @ref UART_WORDLENGTH_*.
					 */

	uint32_t stop_bits;		/*
					 * Specifies the number of stop bits
					 * transmitted. This parameter can be
					 * a value of @ref UART_STOPBITS_*.
					 */

	uint32_t parity;		/*
					 * Specifies the parity mode.
					 * This parameter can be a value of
					 * @ref UART_PARITY_*.
					 */

	uint32_t mode;			/*
					 * Specifies whether the receive or
					 * transmit mode is enabled or
					 * disabled. This parameter can be a
					 * value of @ref @ref UART_MODE_*.
					 */

	uint32_t hw_flow_control;	/*
					 * Specifies whether the hardware flow
					 * control mode is enabled or
					 * disabled. This parameter can be a
					 * value of @ref UART_HWCONTROL_*.
					 */

	uint32_t over_sampling;		/*
					 * Specifies whether the over sampling
					 * 8 is enabled or disabled.
					 * This parameter can be a value of
					 * @ref UART_OVERSAMPLING_*.
					 */

	uint32_t one_bit_sampling;	/*
					 * Specifies whether a single sample
					 * or three samples' majority vote is
					 * selected. This parameter can be 0
					 * or UART_ONE_BIT_SAMPLE_*.
					 */

	uint32_t prescaler;		/*
					 * Specifies the prescaler value used
					 * to divide the UART clock source.
					 * This parameter can be a value of
					 * @ref UART_PRESCALER_*.
					 */

	uint32_t fifo_mode;		/*
					 * Specifies if the FIFO mode will be
					 * used. This parameter can be a value
					 * of @ref UART_FIFOMODE_*.
					 */

	uint32_t tx_fifo_threshold;	/*
					 * Specifies the TXFIFO threshold
					 * level. This parameter can be a
					 * value of @ref
					 * UART_TXFIFO_THRESHOLD_*.
					 */

	uint32_t rx_fifo_threshold;	/*
					 * Specifies the RXFIFO threshold
					 * level. This parameter can be a
					 * value of @ref
					 * UART_RXFIFO_THRESHOLD_*.
					 */
};

struct stm32_uart_advanced_init_s {
	uint32_t adv_feature_init;	/*
					 * Specifies which advanced UART
					 * features are initialized.
					 * This parameter can be a value of
					 * @ref UART_ADVFEATURE_*_INIT.
					 */

	uint32_t tx_pin_level_invert;	/*
					 * Specifies whether the TX pin active
					 * level is inverted.
					 * This parameter can be 0 or
					 * USART_CR2_TXINV.
					 */

	uint32_t rx_pin_level_invert;	/*
					 * Specifies whether the RX pin active
					 * level is inverted.
					 * This parameter can be 0 or
					 * USART_CR2_RXINV.
					 */

	uint32_t data_invert;		/*
					 * Specifies whether data are inverted
					 * (positive/direct logic vs
					 * negative/inverted logic).
					 * This parameter can be 0 or
					 * USART_CR2_DATAINV.
					 */

	uint32_t swap;			/*
					 * Specifies whether TX and RX pins
					 * are swapped. This parameter can be
					 * 0 or USART_CR2_SWAP.
					 */

	uint32_t overrun_disable;	/*
					 * Specifies whether the reception
					 * overrun detection is disabled.
					 * This parameter can be 0 or
					 * USART_CR3_OVRDIS.
					 */

	uint32_t dma_disable_on_rx_error;
					/*
					 * Specifies whether the DMA is
					 * disabled in case of reception
					 * error. This parameter can be 0 or
					 * USART_CR3_DDRE.
					 */

	uint32_t auto_baud_rate_enable;	/*
					 * Specifies whether auto baud rate
					 * detection is enabled. This
					 * parameter can be 0 or
					 * USART_CR2_ABREN.
					 */

	uint32_t auto_baud_rate_mode;	/*
					 * If auto baud rate detection is
					 * enabled, specifies how the rate
					 * detection is carried out. This
					 * parameter can be a value of @ref
					 * UART_ADVFEATURE_AUTOBAUDRATE_ON*.
					 */

	uint32_t msb_first;		/*
					 * Specifies whether MSB is sent first
					 * on UART line.
					 * This parameter can be 0 or
					 * USART_CR2_MSBFIRST.
					 */
};

struct stm32_uart_handle_s {
	struct stm32_uart_init_s init;
	struct stm32_uart_advanced_init_s advanced_init;

	uint8_t *tx_buffer_ptr;
	uint16_t tx_xfer_size;
	volatile uint16_t tx_xfer_count;

	uint8_t *rx_buffer_ptr;
	uint16_t rx_xfer_size;
	volatile uint16_t rx_xfer_count;
};

struct stm32_uart_platdata {
	uintptr_t base;

	unsigned long clk_id;
	unsigned int rst_id;

	struct pinctrl_cfg *pinctrl;

	struct stm32_uart_handle_s *huart;
};

int stm32_uart_init(void);
int stm32_uart_set_config(struct stm32_uart_handle_s *huart);
int stm32_uart_transmit(uint8_t *buf, uint16_t size, uint32_t timeout_ms);
int stm32_uart_receive(uint8_t *buf, uint16_t size, uint32_t timeout_ms);
bool stm32_uart_error_detected(void);
int stm32_uart_flush_rx_fifo(uint32_t timeout_us);
#endif /* STM32_UART_H */

