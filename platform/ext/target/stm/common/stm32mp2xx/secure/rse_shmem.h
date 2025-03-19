/*
 * Copyright (c) 2025 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef RSE_SHMEM_H
#define RSE_SHMEM_H

#include <lib/utils_def.h>

typedef uint32_t  rse_shmem_size_t;

#define RSE_SHMEM_FLAGS_SIZE_NO_NOTIFICATION  BIT(31)

#define RSE_SHMEM_SIZE_OFFSET (0U)
#define RSE_SHMEM_PAYLOAD_OFFSET (RSE_SHMEM_SIZE_OFFSET + sizeof(rse_shmem_size_t))

#endif /* RSE_SHMEM_H */

