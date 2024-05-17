/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019-2022, Linaro Limited
 * Copyright (c) 2023-2024, STMicroelectronics
 *
 */
#ifndef SCMI_SERVER_H
#define SCMI_SERVER_H

/*
 * Request processing of an incoming event in the SCMI server for a target
 * MHU/SMT mailbox.
 *
 * @channel_id: SCMI channel handler
 */
int32_t scmi_server_smt_process_thread(unsigned int channel_id);

/*
 * Request processing of an incoming event in the SCMI server for a target
 * MHU/MSG mailbox.
 *
 * @id: SCMI channel handler
 * @in_buf: Input message MSG buffer
 * @in_size: Input message MSG buffer size
 * @out_buf: Output message MSG buffer
 * @out_size: Reference to output message MSG buffer size
 */
int32_t scmi_server_msg_process_thread(unsigned int channel_id, void *in_buf,
					  size_t in_size, void *out_buf,
					  size_t *out_size);

/*
 * Get SCP-firmware channel device ID from the client channel ID.
 *
 * @channel_id: SCMI channel handler
 * @handle: Output SCP-firmware device ID for the target SCMI mailbox
 */
int32_t scmi_server_get_channel(unsigned int channel_id, int *handle);

/* Get number of channels supported by the SCMI platform/server */
int scmi_server_get_channels_count(void);


#endif /* SCMI_SERVER_H */
