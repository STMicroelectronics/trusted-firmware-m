/*
 * Copyright (c) 2017-2021, Arm Limited. All rights reserved.
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* This file provides implementation of TF-M NS os wrapper functions for the
 * RTOS use case. This implementation provides multithread safety, so it
 * can be used in RTOS environment.
 */

#include <stdint.h>
#include <stdio.h>
#include "os_wrapper/mutex.h"
#include "tfm_ns_interface.h"

#if PLATFORM_HAS_NS_NOTIF
#include "os_wrapper/semaphore.h"
#include "tfm_plat_ns.h"
#endif

/**
 * \brief the ns_lock ID
 */
static void *ns_lock_handle = NULL;

int32_t tfm_ns_interface_dispatch(veneer_fn fn,
                                  uint32_t arg0, uint32_t arg1,
                                  uint32_t arg2, uint32_t arg3)
{
    int32_t result;

    /* TFM request protected by NS lock */
    while (os_wrapper_mutex_acquire(ns_lock_handle, OS_WRAPPER_WAIT_FOREVER)
            != OS_WRAPPER_SUCCESS) {
    }

    result = fn(arg0, arg1, arg2, arg3);

    while (os_wrapper_mutex_release(ns_lock_handle) != OS_WRAPPER_SUCCESS) {
    }

    return result;
}

uint32_t tfm_ns_interface_init(void)
{
    void *handle;

    handle = os_wrapper_mutex_create();
    if (!handle) {
        return OS_WRAPPER_ERROR;
    }

    ns_lock_handle = handle;
    return OS_WRAPPER_SUCCESS;
}

#if PLATFORM_HAS_NS_NOTIF
#define MAX_NS_CTX_QUEUE 1
#define NS_SEC_CTX_STACK 0x2000
static void *semaphore_ns_sec_ctx;

/*
 * function called in non secure handler executed following non secure
 * interruption raised by secure .
 */
void tfm_ns_post_sem_sec_ctx(void)
{
    os_wrapper_semaphore_release(semaphore_ns_sec_ctx);
}

void tfm_ns_sec_process(void *arg)
{
    uint32_t err;

    if (tfm_ns_platform_post_init()) {
        printf("\r\nns plat post init failed\r\n");
        /* Avoid undefined behavior if platform init failed */
        while (1);
    }

    /*  Call the test service , that triggers the 1st interrupt */
    do {

        err = os_wrapper_semaphore_acquire(semaphore_ns_sec_ctx, OS_WRAPPER_WAIT_FOREVER);

        /*  psa api call */
        if (err == OS_WRAPPER_SUCCESS) {
            if (tfm_ns_platform_sec_ctx_call())
                printf("\r\nSec ctx Call failed\r\n");
        }
        else {
            printf("\r\nSemaphore %x Error\r\n", err);
	}
    } while (1);
}

uint32_t tfm_ns_init_secure_event(void)
{
    semaphore_ns_sec_ctx = os_wrapper_semaphore_create(MAX_NS_CTX_QUEUE, 0, "ns_sec_ctx");
    return OS_WRAPPER_SUCCESS;
}
#endif
