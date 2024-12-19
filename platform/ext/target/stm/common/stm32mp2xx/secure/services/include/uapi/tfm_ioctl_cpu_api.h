/*
 * Copyright (C) 2024, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef  TFM_IOCTL_CPU_API_H
#define  TFM_IOCTL_CPU_API_H

/**
 * enum cpu_state - processor states
 * @CPU_OFFLINE:	device is powered off
 * @CPU_SUSPENDED:	device is suspended; needs to be woken up
 *			to receive a message.
 * @CPU_RUNNING:	device is up and running
 * @CPU_CRASHED:	device has crashed; need to start recovery
 * @CPU_LAST:		just keep this one at the end
 */
enum cpu_state_t {
	CPU_OFFLINE = 0,
	CPU_SUSPENDED,
	CPU_RUNNING,
	CPU_CRASHED,
	CPU_LAST,
};

/**
 * enum ctrl_method - cpu control method
 * @ENABLE_METHOD_NONE:		no method defined
 * @ENABLE_METHOD_REMOTEPROC:	use remoteproc to control cpu
 * @ENABLE_METHOD_INVAL:	invalid method.
 */
enum ctrl_method_t {
	ENABLE_METHOD_NONE = 0,
	ENABLE_METHOD_REMOTEPROC,
	ENABLE_METHOD_INVAL,
};

/*
 * cpu service
 *  - nb_cpu:	get number of cpu
 *  - info:	get info on cpu id X
 *  - start:	start cpu id X
 *  - stop:	stop cpu id X
 */
enum tfm_cpu_service_type_t {
	TFM_CPU_SERVICE_TYPE_SERV_INFO = 0,
	TFM_CPU_SERVICE_TYPE_INFO,
	TFM_CPU_SERVICE_TYPE_START,
	TFM_CPU_SERVICE_TYPE_STOP,
};

struct tfm_cpu_service_args_t {
	enum tfm_cpu_service_type_t type;
	union {
		/*
		 * TFM_CPU_SERVICE_TYPE_INFO
		 * TFM_CPU_SERVICE_TYPE_START
		 * TFM_CPU_SERVICE_TYPE_STOP
		 */
		struct cpu_id_args {
			uint32_t id;
		} cpu;
	};
};

//devicetree specification DEVICE_MAX_NAME_LEN 31
#define CPU_MAX_NAME_LEN 31

struct tfm_cpu_service_out_t {
	union {
		struct cpu_serv_info {
			int32_t nb_cpu;
		} service_info;
		struct cpu_info_res {
			int32_t status;
			int32_t method;
			char name[CPU_MAX_NAME_LEN];
		} cpu_info;
		struct cpu_cmd_res {
			int32_t status;
		} cpu_cmd;
	};
};

/**
 * @brief get information on cpu service, like number of cpu managed.
 *
 * @param[out] serv_info   Pointer on service information structure
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_cpu_service_info(struct cpu_serv_info *serv_info);

/**
 * @brief get information on cpu id X.
 *
 * @param[in]  cpu_id	   cpu id
 * @param[out] cpu_info    Pointer on cpu information structure
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_cpu_info(uint32_t cpu_id, struct cpu_info_res *cpu_info);

/**
 * @brief send start command on cpu id X.
 *
 * @param[in]  cpu_id	   cpu id
 * @param[out] status      Pointer, return status after command.
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_cpu_start(uint32_t cpu_id, int32_t *status);

/**
 * @brief send stop command on cpu id X.
 *
 * @param[in]  cpu_id	   cpu id
 * @param[out] status      Pointer, return status after command.
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_cpu_stop(uint32_t cpu_id, int32_t *status);
#endif /* TFM_IOCTL_CPU_API_H */
