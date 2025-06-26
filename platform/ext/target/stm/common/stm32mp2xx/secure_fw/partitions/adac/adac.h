/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __ADAC_H__
#define __ADAC_H__

#include <stdbool.h>
#include <psa/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initialise ADAC secure partition service.
 *
 * \param[out]  is_service_enabled       Whether authenticated debug service is
 *                                       enabled or not.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval #PSA_SUCCESS
 *         Success.
 * \retval #PSA_ERROR_SERVICE_FAILURE
 *         Service is not able to read LCS state or key from OTP.
 */
psa_status_t adac_sp_init(bool *is_service_enabled);

/*!
 * \brief  Authenticates the requested debug service.
 *
 * \return Returns PSA_SUCCESS on success,
 *         otherwise error as specified in \ref psa_status_t
 */
psa_status_t adac_service_request(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADAC_H__ */
