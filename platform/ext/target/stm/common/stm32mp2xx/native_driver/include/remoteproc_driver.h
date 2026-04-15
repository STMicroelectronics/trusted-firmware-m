/*
 * Copyright (c) 2026, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REMOTEPROC_DRIVER_H
#define REMOTEPROC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * @brief  Callback type for remote processor crash events.
 */
typedef void (*remoteproc_crash_cb)(uint32_t cpu_id);

/*
 * @brief Remote processor driver interface.
 * Project provides one concrete driver by defining the global instance:
 * RemoteProcDriverTypeDef remoteproc_driver;`
 */
typedef struct {
	int (*init)(void);
	int (*deinit)(void);
	int (*register_crash_callback)(remoteproc_crash_cb cb);
	int (*unregister_crash_callback)(remoteproc_crash_cb cb);
} remoteprocdriver_t;

/*
 * @brief Global remote processor driver instance.
 * @note  Project must define this symbol in exactly one C file.
 */
extern remoteprocdriver_t remoteproc_driver;

#ifdef __cplusplus
}
#endif

#endif /* REMOTEPROC_DRIVER_H */
