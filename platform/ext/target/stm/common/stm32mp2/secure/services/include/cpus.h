/*
 * Copyright (c) 2024, STMicroelectronics
 *
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 */
#ifndef  CPUS_H
#define  CPUS_H
#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_cpu_api.h>

enum tfm_platform_err_t cpus_service(const psa_invec *in_vec, const psa_outvec *out_vec);

#endif /* CPUS_H */
