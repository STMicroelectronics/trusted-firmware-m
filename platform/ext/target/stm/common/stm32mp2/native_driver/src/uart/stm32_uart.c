/*
 * Copyright (c) 2015-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lib/mmio.h>
#include <lib/timeout.h>

#include <stm32_clk.h>

#include "stm32_uart_regs.h"
#include <stm32_uart.h>

#define DT_UART_COMPAT		"st,stm32h7-uart"

#define UART_CR1_FIELDS \
		(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | \
		 USART_CR1_RE | USART_CR1_OVER8 | USART_CR1_FIFOEN)

#define USART_CR3_FIELDS \
		(USART_CR3_RTSE | USART_CR3_CTSE | USART_CR3_ONEBIT | \
		 USART_CR3_TXFTCFG | USART_CR3_RXFTCFG)

#define UART_ISR_ERRORS	 \
		(USART_ISR_ORE | USART_ISR_NE |  USART_ISR_FE | USART_ISR_PE)

/*
 * @brief  Return the UART clock frequency.
 * @param  huart: UART handle.
 * @retval Frequency value in Hz.
 */
static unsigned long uart_get_clock_freq(struct stm32_uart_handle_s *huart)
{
	return huart->clk;
}

/*
 * @brief  Configure the UART peripheral.
 * @param  huart: UART handle.
 * @retval UART status.
 */
static uint32_t uart_set_config(struct stm32_uart_handle_s *huart)
{
	uint32_t tmpreg;
	unsigned long clockfreq;
	uint16_t brrtemp;

	/*
	 * ---------------------- USART CR1 Configuration --------------------
	 * Clear M, PCE, PS, TE, RE and OVER8 bits and configure
	 * the UART word length, parity, mode and oversampling:
	 * - set the M bits according to huart->init.word_length value,
	 * - set PCE and PS bits according to huart->init.parity value,
	 * - set TE and RE bits according to huart->init.mode value,
	 * - set OVER8 bit according to huart->init.over_sampling value.
	 */
	tmpreg = huart->init.word_length |
		 huart->init.parity |
		 huart->init.mode |
		 huart->init.over_sampling |
		 huart->init.fifo_mode;
	mmio_clrsetbits_32(huart->base + USART_CR1, UART_CR1_FIELDS, tmpreg);

	/*
	 * --------------------- USART CR2 Configuration ---------------------
	 * Configure the UART Stop Bits: Set STOP[13:12] bits according
	 * to huart->init.stop_bits value.
	 */
	mmio_clrsetbits_32(huart->base + USART_CR2, USART_CR2_STOP,
			   huart->init.stop_bits);

	/*
	 * --------------------- USART CR3 Configuration ---------------------
	 * Configure:
	 * - UART HardWare Flow Control: set CTSE and RTSE bits according
	 *   to huart->init.hw_flow_control value,
	 * - one-bit sampling method versus three samples' majority rule
	 *   according to huart->init.one_bit_sampling (not applicable to
	 *   LPUART),
	 * - set TXFTCFG bit according to huart->init.tx_fifo_threshold value,
	 * - set RXFTCFG bit according to huart->init.rx_fifo_threshold value.
	 */
	tmpreg = huart->init.hw_flow_control | huart->init.one_bit_sampling;

	if (huart->init.fifo_mode == USART_CR1_FIFOEN) {
		tmpreg |= huart->init.tx_fifo_threshold |
			  huart->init.rx_fifo_threshold;
	}

	mmio_clrsetbits_32(huart->base + USART_CR3, USART_CR3_FIELDS, tmpreg);

	/*
	 * --------------------- USART PRESC Configuration -------------------
	 * Configure UART Clock Prescaler : set PRESCALER according to
	 * huart->init.prescaler value.
	 */
	assert(huart->init.prescaler <= UART_PRESCALER_MAX);
	mmio_clrsetbits_32(huart->base + USART_PRESC, USART_PRESC_PRESCALER,
			   huart->init.prescaler);

	/*---------------------- USART BRR configuration --------------------*/
	clockfreq = uart_get_clock_freq(huart);
	if (clockfreq == 0UL) {
		return UART_ERROR;
	}

	if (huart->init.over_sampling == UART_OVERSAMPLING_8) {
		uint16_t usartdiv =
			(uint16_t)uart_div_sampling8(clockfreq,
						     huart->init.baud_rate,
						     huart->init.prescaler);

		brrtemp = (usartdiv & GENMASK(15, 4)) |
			  ((usartdiv & GENMASK(3, 0)) >> 1);
	} else {
		brrtemp = (uint16_t)uart_div_sampling16(clockfreq,
							huart->init.baud_rate,
							huart->init.prescaler);
	}

	mmio_write_16(huart->base + USART_BRR, brrtemp);

	return UART_OK;
}

/*
 * @brief  Configure the UART peripheralnumanced features.
 * @param  huart: UART handle.
 * @retval None.
 */
static void uart_config_advanced_features(struct stm32_uart_handle_s *huart)
{
	uint32_t cr2_clr_value = 0U;
	uint32_t cr2_set_value = 0U;
	uint32_t cr3_clr_value = 0U;
	uint32_t cr3_set_value = 0U;

	/* If required, configure TX pin active level inversion */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_TXINVERT_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_TXINV;
		cr2_set_value |= huart->advanced_init.tx_pin_level_invert;
	}

	/* If required, configure RX pin active level inversion */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_RXINVERT_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_RXINV;
		cr2_set_value |= huart->advanced_init.rx_pin_level_invert;
	}

	/* If required, configure data inversion */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_DATAINVERT_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_DATAINV;
		cr2_set_value |= huart->advanced_init.data_invert;
	}

	/* If required, configure RX/TX pins swap */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_SWAP_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_SWAP;
		cr2_set_value |= huart->advanced_init.swap;
	}

	/* If required, configure RX overrun detection disabling */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_RXOVERRUNDISABLE_INIT) != 0U) {
		cr3_clr_value |= USART_CR3_OVRDIS;
		cr3_set_value |= huart->advanced_init.overrun_disable;
	}

	/* If required, configure DMA disabling on reception error */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_DMADISABLEONERROR_INIT) != 0U) {
		cr3_clr_value |= USART_CR3_DDRE;
		cr3_set_value |= huart->advanced_init.dma_disable_on_rx_error;
	}

	/* If required, configure auto baud rate detection scheme */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_AUTOBAUDRATE_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_ABREN;
		cr2_set_value |= huart->advanced_init.auto_baud_rate_enable;

		/*
		 * Set auto baudrate detection parameters if detection is
		 * enabled.
		 */
		if (huart->advanced_init.auto_baud_rate_enable ==
		    USART_CR2_ABREN) {
			cr2_clr_value |= USART_CR2_ABRMODE;
			cr2_set_value |=
				huart->advanced_init.auto_baud_rate_mode;
		}
	}

	/* If required, configure MSB first on communication line */
	if ((huart->advanced_init.adv_feature_init &
	     UART_ADVFEATURE_MSBFIRST_INIT) != 0U) {
		cr2_clr_value |= USART_CR2_MSBFIRST;
		cr2_set_value |= huart->advanced_init.msb_first;
	}

	mmio_clrsetbits_32(huart->base + USART_CR2, cr2_clr_value,
						    cr2_set_value);
	mmio_clrsetbits_32(huart->base + USART_CR3, cr3_clr_value,
						    cr3_set_value);
}

/*
 * @brief  Handle UART communication timeout.
 * @param  huart: UART handle.
 * @param  flag: Specifies the UART flag to check.
 * @param  timeout_ref: Reference to target timeout.
 * @retval UART status.
 */
static uint32_t uart_wait_flag(struct stm32_uart_handle_s *huart, uint32_t flag,
			uint64_t timeout_ref)
{
	while ((mmio_read_32(huart->base + USART_ISR) & flag) == 0U) {
		if (timeout_elapsed(timeout_ref)) {
			/*
			 * Disable TXE, frame error, noise error, overrun
			 * error interrupts for the interrupt process.
			 */
			mmio_clrbits_32(huart->base + USART_CR1,
					USART_CR1_RXNEIE | USART_CR1_PEIE |
					USART_CR1_TXEIE);
			mmio_clrbits_32(huart->base + USART_CR3, USART_CR3_EIE);

			return UART_TIMEOUT;
		}
	}

	return UART_OK;
}

/*
 * @brief  Check the UART idle State.
 * @param  huart: UART handle.
 * @retval UART status.
 */
static uint32_t uart_check_idle_state(struct stm32_uart_handle_s *huart)
{
	uint64_t timeout_ref;

	timeout_ref = timeout_init_us(UART_TIMEOUT_US);

	/* Check if the transmitter is enabled */
	if (((mmio_read_32(huart->base + USART_CR1) & USART_CR1_TE) ==
	     USART_CR1_TE) &&
	    (uart_wait_flag(huart, USART_ISR_TEACK, timeout_ref) != UART_OK)) {
		return UART_TIMEOUT;
	}

	/* Check if the receiver is enabled */
	if (((mmio_read_32(huart->base + USART_CR1) & USART_CR1_RE) ==
	     USART_CR1_RE) &&
	    (uart_wait_flag(huart, USART_ISR_REACK, timeout_ref) != UART_OK)) {
		return UART_TIMEOUT;
	}

	return UART_OK;
}

/*
 * @brief  Initialize UART.
 * @param  huart: UART handle.
 * @retval UART status.
 */
uint32_t uart_init(struct stm32_uart_handle_s *huart)
{
	if (huart == NULL) {
		return UART_ERROR;
	}

	/* Disable the peripheral */
	mmio_clrbits_32(huart->base + USART_CR1, USART_CR1_UE);

	if (uart_set_config(huart) == UART_ERROR) {
		return UART_ERROR;
	}

	if (huart->advanced_init.adv_feature_init != UART_ADVFEATURE_NO_INIT) {
		uart_config_advanced_features(huart);
	}

	/*
	 * In asynchronous mode, the following bits must be kept cleared:
	 * - LINEN and CLKEN bits in the USART_CR2 register,
	 * - SCEN, HDSEL and IREN  bits in the USART_CR3 register.
	 */
	mmio_clrbits_32(huart->base + USART_CR2,
			USART_CR2_LINEN | USART_CR2_CLKEN);
	mmio_clrbits_32(huart->base + USART_CR3,
			USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN);

	/* Enable the peripheral */
	mmio_setbits_32(huart->base + USART_CR1, USART_CR1_UE);

	/* TEACK and/or REACK to check */
	return uart_check_idle_state(huart);
}

/*
 * @brief  Transmit an amount of data in blocking mode.
 * @param  huart: UART handle.
 * @param  pdata: pointer to data buffer.
 * @param  size: amount of data to transmit.
 * @param  timeout_us: Timeout duration in microseconds.
 * @retval UART status.
 */
uint32_t uart_transmit(struct stm32_uart_handle_s *huart, uint8_t *pdata,
		       uint16_t size, uint32_t timeout_us)
{
	uint64_t timeout_ref;
	unsigned int count = 0U;

	if ((pdata == NULL) || (size == 0U)) {
		return UART_ERROR;
	}

	timeout_ref = timeout_init_us(timeout_us);

	huart->tx_xfer_size = size;
	huart->tx_xfer_count = size;
	while (huart->tx_xfer_count > 0U) {
		huart->tx_xfer_count--;
		if (uart_wait_flag(huart, USART_ISR_TXE, timeout_ref) !=
		    UART_OK) {
			return UART_TIMEOUT;
		}

		if ((huart->init.word_length == UART_WORDLENGTH_9B) &&
		    (huart->init.parity == UART_PARITY_NONE)) {
			uint16_t data = (uint16_t)*(pdata + count) |
					(uint16_t)(*(pdata + count + 1) << 8);

			mmio_write_16(huart->base + USART_TDR,
				      data & GENMASK(8, 0));
			count += 2U;
		} else {
			mmio_write_8(huart->base + USART_TDR,
				     (*(pdata + count) & GENMASK(7, 0)));
			count++;
		}
	}

	if (uart_wait_flag(huart, USART_ISR_TC, timeout_ref) != UART_OK) {
		return UART_TIMEOUT;
	}

	return UART_OK;
}

/*
 * @brief  Compute RDR register mask depending on word length.
 * @param  huart: UART handle.
 * @retval Mask value.
 */
static unsigned int uart_mask_computation(struct stm32_uart_handle_s *huart)
{
	unsigned int mask;

	switch (huart->init.word_length) {
	case UART_WORDLENGTH_9B:
		mask = GENMASK(8, 0);
		break;
	case UART_WORDLENGTH_8B:
		mask = GENMASK(7, 0);
		break;
	case UART_WORDLENGTH_7B:
		mask = GENMASK(6, 0);
		break;
	default:
		return 0U;
	}

	if (huart->init.parity != UART_PARITY_NONE) {
		mask >>= 1;
	}

	return mask;
}

/*
 * @brief  Receive an amount of data in blocking mode.
 * @param  huart: UART handle.
 * @param  pdata: pointer to data buffer.
 * @param  size: amount of data to be received.
 * @param  timeout_us: Timeout duration in microseconds.
 * @retval UART status.
 */
uint32_t uart_receive(struct stm32_uart_handle_s *huart, uint8_t *pdata,
		      uint16_t size, uint32_t timeout_us)
{
	uint16_t uhmask;
	uint64_t timeout_ref;
	unsigned int count = 0U;

	if ((pdata == NULL) || (size == 0U)) {
		return  UART_ERROR;
	}

	timeout_ref = timeout_init_us(timeout_us);

	huart->rx_xfer_size = size;
	huart->rx_xfer_count = size;

	/* Computation of UART mask to apply to RDR register */
	uhmask = uart_mask_computation(huart);

	while (huart->rx_xfer_count > 0U) {
		huart->rx_xfer_count--;
		if (uart_wait_flag(huart, USART_ISR_RXNE, timeout_ref) !=
		    UART_OK) {
			return UART_TIMEOUT;
		}

		if ((huart->init.word_length == UART_WORDLENGTH_9B) &&
		    (huart->init.parity == UART_PARITY_NONE)) {
			uint16_t data = mmio_read_16(huart->base +
						     USART_RDR) & uhmask;

			*(pdata + count) = (uint8_t)data;
			*(pdata + count + 1) = (uint8_t)(data >> 8);

			count += 2U;
		} else {
			*(pdata + count) = mmio_read_8(huart->base +
						       USART_RDR) &
						       (uint8_t)uhmask;
			count++;
		}
	}

	return UART_OK;
}

/*
 * @brief  Check interrupt and status errors.
 * @retval True if error detected, false otherwise.
 */
bool uart_error_detected(struct stm32_uart_handle_s *huart)
{
	return (mmio_read_32(huart->base + USART_ISR) & UART_ISR_ERRORS) != 0U;
}

/*
 * @brief  Flush the UART RX FIFO.
 * @param  huart: UART handle.
 * @param  timeout_us: Timeout duration in microseconds.
 * @retval UART status.
 */
uint32_t uart_flush_rx_fifo(struct stm32_uart_handle_s *huart,
			    uint32_t timeout_us)
{
	uint8_t byte = 0U;
	uint64_t timeout_ref;
	uint32_t ret = UART_OK;

	timeout_ref = timeout_init_us(timeout_us);

	/* Clear all errors */
	mmio_write_32(huart->base + USART_ISR, UART_ISR_ERRORS);

	while (((mmio_read_32(huart->base + USART_ISR) & USART_ISR_RXNE) !=
		0U) && !timeout_elapsed(timeout_ref)) {
		ret = uart_receive(huart, &byte, 1U, timeout_us);

		if ((ret != UART_OK) || uart_error_detected(huart)) {
			return UART_ERROR;
		}
	}

	return ret;
}
