/*
 * Copyright (C) 2022-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP_DDR_TEST_H
#define STM32MP_DDR_TEST_H

#include <stdint.h>

uintptr_t stm32mp_ddr_test_rw_access(struct stm32mp_ddr_size *info);
uintptr_t stm32mp_ddr_test_data_bus(struct stm32mp_ddr_size *info);
uintptr_t stm32mp_ddr_test_addr_bus(struct stm32mp_ddr_size *info);
size_t stm32mp_ddr_check_size(struct stm32mp_ddr_size *info, size_t size);

#endif /* STM32MP_DDR_TEST_H */
