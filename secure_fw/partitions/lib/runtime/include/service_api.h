/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __SERVICE_API_H__
#define __SERVICE_API_H__

#include <stdint.h>
#include "tfm_boot_status.h"
#include "psa/error.h"

/**
 * \brief Retrieve secure partition related data from shared memory area, which
 *        stores shared data between bootloader and runtime firmware.
 *
 * \param[in]  major_type  Major type.
 * \param[out] boot_data   Pointer to boot data.
 * \param[in]  len         The length of the boot data.
 */
psa_status_t tfm_core_get_boot_data(uint8_t major_type,
                                    struct tfm_boot_data *boot_data,
                                    uint32_t len);

#if PLATFORM_HAS_NS_NOTIF

psa_status_t tfm_ns_notif(uint32_t event);

#if TFM_ISOLATION_LEVEL > 1
/**
 * \brief Raise ns notif from a PSA ROT or APP rot execution context

 * \param[in]  Event value.
 */
psa_status_t tfm_ns_notif_flih_unpriv(uint32_t event);
psa_status_t tfm_ns_notif_flih_priv(uint32_t event);

/**
 * \brief Raise ns notif from a FLIH execution context

 * \param[in]  Event value.
 */
#define tfm_ns_notif_flih(A) do {\
    if (A##_FLIH_FLAG == 1) {\
        tfm_ns_notif_flih_priv(A);\
    }\
    else \
    tfm_ns_notif_flih_unpriv(A);\
} while(0)

#else /* TFM_ISOLATION_LEVEL == 1 */

#define tfm_ns_notif_flih(A) tfm_ns_notif((A));

#endif /* TFM_ISOLATION_LEVEL > 1 */
#endif /* PLATFORM_HAS_NS_NOTIF */

#endif /* __SERVICE_API_H__ */
