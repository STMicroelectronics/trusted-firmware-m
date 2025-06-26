/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_ADAC_API_H__
#define __TFM_ADAC_API_H__

#include <stdint.h>
#include <psa/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Authenticates and process input debug request.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval #PSA_SUCCESS                   The operation completed successfully
 * \retval #PSA_ERROR_PROGRAMMER_ERROR    The operation failed because failure
 *                                        to provided arguments are incorrect
 * \retval #PSA_ERROR_INVALID_ARGUMENT    The operation failed because debug
 *                                        request identifier is invalid
 * \retval #PSA_ERROR_NOT_PERMITTED       The operation failed because the
 *                                        conditions for providing secure
 *                                        debug service are not valid.
 *                                        for e.g. if the LCS is not in
 *                                        required state OR the service failed
 *                                        to authenticate the host.
 */
psa_status_t tfm_adac_service(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_ADAC_API_H__ */
