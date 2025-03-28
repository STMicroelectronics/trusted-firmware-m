/*
 * Copyright (c) 2025, STMicroelectronics All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __SCP_MBOX_H__
#define __SCP_MBOX_H__
/*  api use by scmi agent to notify the respond to a service */
int scp_mbox_raise(void *mbx_chan);
void scp_com_handle(int type);

#endif /* __SCP_MBOX_H__ */
