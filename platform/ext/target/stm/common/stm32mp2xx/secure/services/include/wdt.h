/*
 * Copyright (c) 2024, STMicroelectronics
 *
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 */
#ifndef  WDT_H
#define  WDT_H
#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_wdt_api.h>

enum tfm_platform_err_t watchdog_service(const psa_invec *in_vec, const psa_outvec *out_vec);

#endif /* WDT_H */
