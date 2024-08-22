/*
 * Copyright (C) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_PERIPHERALS_DEF_H__
#define __TFM_PERIPHERALS_DEF_H__

struct tfm_spm_partition_platform_data_t;

#define TFM_PERIPHERAL_STD_UART     (0)
#define TFM_PERIPHERAL_FPGA_IO      (0)


struct platform_data_t;
extern const struct platform_data_t tfm_peripheral_timer2;


#define TFM_PERIPHERAL_STD_UART     (0)
#define TFM_PERIPHERAL_TIMER0       (&tfm_peripheral_timer2)
#define TFM_TIMER0_IRQ              DT_IRQ(DT_NODELABEL(tim2), irq)
#define TFM_PERIPHERAL_FPGA_IO      (0)


#endif /* __TFM_PERIPHERALS_DEF_H__ */
