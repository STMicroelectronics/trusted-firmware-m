/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __CONFIG_PARTITION_SCP_H__
#define __CONFIG_PARTITION_SCP_H__

#include "config_tfm.h"

/* Size of input buffer in SCP service */
#ifndef SCP_SERVICE_INPUT_BUFFER_SIZE
#pragma message("SCP_SERVICE_INPUT_BUFFER_SIZE is defaulted to 64. Please check and set it explicitly.")
#define SCP_SERVICE_INPUT_BUFFER_SIZE     64
#endif

/* Size of output buffer in SCP service */
#ifndef SCP_SERVICE_OUTPUT_BUFFER_SIZE
#pragma message("SCP_SERVICE_OUTPUT_BUFFER_SIZE is defaulted to 64. Please check and set it explicitly.")
#define SCP_SERVICE_OUTPUT_BUFFER_SIZE    64
#endif

#endif /* __CONFIG_PARTITION_SCP_H__ */
