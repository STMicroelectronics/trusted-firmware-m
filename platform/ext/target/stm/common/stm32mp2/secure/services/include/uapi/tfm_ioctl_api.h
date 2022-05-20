/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  TFM_IOCTL_API_H
#define  TFM_IOCTL_API_H

#include <stdint.h>
#include <limits.h>

/*
 * Supported request types.
 */
enum tfm_platform_ioctl_id_t {
	TFM_PLATFORM_IOCTL_COPRO_SERVICE,
};

/*
 * Copro service
 *  - get state: running, reset, off
 *  - set state: running, reset
 */
enum tfm_copro_service_type_t {
    TFM_COPRO_SERVICE_TYPE_GET = 0,
    TFM_COPRO_SERVICE_TYPE_SET,
};

enum stm32_copro_state_t {
    COPRO_ERR = INT_MIN,
    COPRO_RUNNING = 0,
    COPRO_RESET = 1,
    COPRO_OFF = 2,
};

struct tfm_copro_service_args_t {
	enum tfm_copro_service_type_t type;
	enum stm32_copro_state_t state;
};

struct tfm_copro_service_out_t {
	enum stm32_copro_state_t state;
};

#endif /* TFM_IOCTL_API_H */
