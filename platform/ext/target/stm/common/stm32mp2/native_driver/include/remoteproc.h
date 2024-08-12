// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * inspired by the linux remoteproc framework
 */
#ifndef  INCLUDE_REMOTEPROC_H_
#define  INCLUDE_REMOTEPROC_H_

#include <device.h>
#include <stdatomic.h>

/**
 * struct rproc_spec - Specification on remoteproc reference.
 *
 * @dev:	Reference on a remoteproc device.
 * @state:	state of the device.
 * @use_count:	Atomic counter on number of user.
 */
struct rproc_spec {
	const struct device *dev;
	int state;
	atomic_int use_count;
};

/**
 * struct remoteproc_driver_api - platform-specific device handlers
 * @get_rproc	return remoteproc specification of device
 * @start:	power on the device and boot it
 * @stop:	power off the device
 */
struct remoteproc_driver_api {
	struct rproc_spec *(*get_rproc)(const struct device *dev);
	int (*start)(struct rproc_spec *rproc);
	int (*stop)(struct rproc_spec *rproc);
};

void rproc_init(const struct device *dev, struct rproc_spec *rproc);

/* consumer */
#define DT_NUM_RPROCS(node_id) \
	DT_PROP_LEN(node_id, remoteprocs)

#define DT_INST_NUM_RPROCS(inst) \
	DT_NUM_RPROCS(DT_DRV_INST(inst))
/**
 * @brief Get the node identifier for the controller phandle from a
 *        "remoteprocs" phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     rproc1: remoteproc-controller@... { ... };
 *
 *     rproc2: remoteproc-controller@... { ... };
 *
 *     n: node {
 *             remoteprocs = <&rproc1>, <&rproc2>;
 *     };
 *
 * Example usage:
 *
 *     DT_RPROCS_CTLR_BY_IDX(DT_NODELABEL(n), 0)) // DT_NODELABEL(rproc1)
 *     DT_RPROCS_CTLR_BY_IDX(DT_NODELABEL(n), 1)) // DT_NODELABEL(rproc2)
 *
 * @param node_id node identifier
 * @param idx logical index into "remoteprocs"
 * @return the node identifier for the remoteproc controller referenced at
 *         index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_RPROCS_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, remoteprocs, idx)

#define DT_INST_RPROCS_CTLR_BY_IDX(inst, idx) \
	DT_RPROCS_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_RPROCS_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return a node identifier for the remoteprocs controller at index 0
 *         in "remoteprocs"
 * @see DT_RPROCS_CTLR_BY_IDX()
 */
#define DT_RPROCS_CTLR(node_id) DT_RPROCS_CTLR_BY_IDX(node_id, 0)
#define DT_INST_RPROCS_CTLR(inst) DT_RPROCS_CTLR(DT_DRV_INST(inst))

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "remoteprocs" phandle-array property by name
 *
 * Example devicetree fragment:
 *
 *     rproc1: remoteproc-controller@... { ... };
 *
 *     rproc2: remoteproc-controller@... { ... };
 *
 *     n: node {
 *             remoteprocs = <&rproc1>, <&rproc2>;
 *             remoteproc-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_RPROCS_CTLR_BY_NAME(DT_NODELABEL(n), beta) // DT_NODELABEL(rproc2)
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores name of a remoteprocs element
 *             as defined by the node's remoteproc-names property
 * @return the node identifier for the remoteprocs controller referenced by name
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_RPROCS_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, remoteprocs, name)
#define DT_INST_RPROCS_CTLR_BY_NAME(inst, name) \
	DT_RPROCS_CTLR_BY_NAME(DT_DRV_INST(inst), name)

int rproc_boot(const struct device *dev);
int rproc_shutdown(const struct device *dev);
int rproc_status(const struct device *dev);
#endif   /* ----- #ifndef INCLUDE_REMOTEPROC_H_  ----- */
