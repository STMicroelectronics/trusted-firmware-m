/*
 * Copyright (c) 2019-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_MULTI_CORE_H__
#define __TFM_MULTI_CORE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "psa/error.h"

/* Follow CMSE flag definitions */
#define MEM_CHECK_MPU_READWRITE         (1 << 0x0)
#define MEM_CHECK_AU_NONSECURE          (1 << 0x1)
#define MEM_CHECK_MPU_UNPRIV            (1 << 0x2)
#define MEM_CHECK_MPU_READ              (1 << 0x3)
#define MEM_CHECK_MPU_NONSECURE         (1 << 0x4)
#define MEM_CHECK_NONSECURE             (MEM_CHECK_AU_NONSECURE | \
                                         MEM_CHECK_MPU_NONSECURE)

#define CLIENT_ID_OWNER_MAGIC           (void *)0xFFFFFFFF

/* Security attributes of target memory region in memory access check. */
struct security_attr_info_t {
    bool is_valid;             /* Whether the target memory region is valid */
    bool is_secure;            /* Secure memory or non-secure memory */
};

/* Memory access attributes of target memory region in memory access check. */
struct mem_attr_info_t {
    bool is_mpu_enabled;       /* Whether memory protection unit(s) enabled */
    bool is_valid;             /* Whether the target memory region is valid */
    bool is_xn;                /* Execute Never or not */
    bool is_priv_rd_allow;     /* Privileged read is allowed or not */
    bool is_priv_wr_allow;     /* Privileged write is allowed or not */
    bool is_unpriv_rd_allow;   /* Unprivileged read is allowed or not */
    bool is_unpriv_wr_allow;   /* Unprivileged write is allowed or not */
};

/**
 * \brief Retrieve general security isolation configuration information of the
 *        target memory region according to the system memory region layout and
 *        fill the \ref security_attr_info_t.
 *
 * \param[in]  p               Base address of target memory region
 * \param[in]  s               Size of target memory region
 * \param[out] p_attr          Address of \ref security_attr_info_t to be filled
 *
 * \return void
 *
 * \note This function doesn't access any hardware security isolation unit.
 */
void tfm_get_mem_region_security_attr(const void *p, size_t s,
                                      struct security_attr_info_t *p_attr);

/**
 * \brief Retrieve general secure memory protection configuration information of
 *        the target memory region according to the system memory region layout
 *        and symbol addresses and fill the \ref mem_attr_info_t.
 *
 * \param[in]  p               Base address of target memory region
 * \param[in]  s               Size of target memory region
 * \param[out] p_attr          Address of \ref mem_attr_info_t to be filled
 *
 * \return void
 *
 * \note This function doesn't access any hardware memory protection unit.
 *       The \ref is_mpu_enabled field is set to false by default.
 */
void tfm_get_secure_mem_region_attr(const void *p, size_t s,
                                    struct mem_attr_info_t *p_attr);

/**
 * \brief Retrieve general non-secure memory protection configuration
 *        information of the target memory region according to the system memory
 *        region layout and fill the \ref mem_attr_info_t.
 *
 * \param[in]  p               Base address of target memory region
 * \param[in]  s               Size of target memory region
 * \param[out] p_attr          Address of \ref mem_attr_info_t to be filled
 *
 * \return void
 *
 * \note This function doesn't access any hardware memory protection unit.
 *       The \ref is_mpu_enabled field is set to false by default.
 */
void tfm_get_ns_mem_region_attr(const void *p, size_t s,
                                struct mem_attr_info_t *p_attr);

/**
 * \brief Check whether a memory access is allowed to access to a memory range
 *
 * \param[in] p               The start address of the range to check
 * \param[in] s               The size of the range to check
 * \param[in] flags           The memory access types to be checked between
 *                            given memory and boundaries.
 *
 * \return SPM_SUCCESS if the access is allowed,
 *         SPM_ERROR_GENERIC otherwise.
 */
int32_t tfm_has_access_to_region(const void *p, size_t s, uint32_t flags);

/**
 * \brief Initialization of the multi core communication.
 *
 * \retval 0                    Operation succeeded.
 * \retval Other return code    Operation failed with an error code.
 */
int32_t tfm_inter_core_comm_init(void);

/**
 * \brief SFN backend: mailbox agent partition init.
 *
 * Boots the non-secure core and sets up the inter-core communication. Intended
 * to be used as the "entry_init" of the mailbox agent partition when the SFN
 * backend is selected, since the agent has no dedicated thread to run an entry
 * loop. Only defined when CONFIG_TFM_SPM_BACKEND_IPC != 1.
 *
 * \param[in] param           Unused init parameter.
 *
 * \retval PSA_SUCCESS        Initialization succeeded (never returns on error).
 */
psa_status_t ns_agent_mailbox_init(void *param);

/**
 * \brief SFN backend: process pending mailbox request(s).
 *
 * Generic mailbox request pump for the SFN model. A platform's mailbox agent
 * secure service SFN handler must call this function to dispatch the queued
 * request(s) and reply, synchronously, from Thread mode. Only defined when
 * CONFIG_TFM_SPM_BACKEND_IPC != 1.
 *
 * \retval PSA_SUCCESS        The pending request(s) were processed.
 */
psa_status_t ns_agent_mailbox_process_request(void);

/**
 * \brief Mailbox agent deferred-receive hook (optional, platform-defined).
 *
 * Called by the mailbox agent pump (both the IPC entry loop and the SFN
 * process-request function) right before dispatching queued requests, i.e.
 * from Thread mode. It lets a platform move the reception/deserialization of
 * an inter-core request OUT of the interrupt (FLIH) context and perform it
 * here instead, keeping the interrupt handler minimal.
 *
 * The default implementation is empty (weak): platforms that receive and
 * enqueue the request directly in their interrupt handler do not need to
 * override it and keep their existing behavior.
 *
 * A platform overriding this hook is responsible for filling the request
 * queue (typically by calling tfm_multi_core_hal_receive()) so that the
 * subsequent dispatch finds the deserialized request(s). This is only safe
 * for protocols with a single outstanding request whose shared buffer stays
 * stable until the reply is produced (e.g. RSE comms).
 */
void tfm_mailbox_hal_pending_receive(void);

/**
 * \brief Check whether a memory range is inside a memory region.
 *
 * \param[in] p             The start address of the range to check
 * \param[in] s             The size of the range to check
 * \param[in] region_start  The start address of the region, which should
 *                          contain the range
 * \param[in] region_limit  The end address of the region, which should contain
 *                          the range
 *
 * \return SPM_SUCCESS if the region contains the range,
 *         SPM_ERROR_GENERIC otherwise.
 */
int32_t check_address_range(const void *p, size_t s,
                            uintptr_t region_start,
                            uintptr_t region_limit);

/**
 * \brief Register a non-secure client ID range.
 *
 * \param[in] owner           Identifier of the non-secure client.
 * \param[in] client_id_base  The minimum client ID for this client.
 * \param[in] client_id_limit The maximum client ID for this client.
 *
 * \return SPM_SUCCESS if the registration is successful, SPM_ERROR_GENERIC
 *         if owner is null, or no free slots left.
 */
int32_t tfm_multi_core_register_client_id_range(void *owner,
                                                int32_t client_id_base,
                                                int32_t client_id_limit);

/**
 * \brief Translate a non-secure client ID range.
 *
 * \param[in]  owner         Identifier of the non-secure client.
 * \param[in]  client_id_in  The input client ID.
 * \param[out] client_id_out The translated client ID. Undefined if
 *                           SPM_ERROR_GENERIC is returned by the function
 *
 * \return SPM_SUCCESS if the translation is successful, SPM_ERROR_GENERIC
 *         otherwise.
 */
int32_t tfm_multi_core_hal_client_id_translate(void *owner,
                                               int32_t client_id_in,
                                               int32_t *client_id_out);

#endif /* __TFM_MULTI_CORE_H__ */
