/*
 * Copyright (C) 2024, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INCLUDE_FIREWALL_H_
#define INCLUDE_FIREWALL_H_

#include <errno.h>
#include <stdint.h>
#include <lib/utils_def.h>

#include <device.h>

/**
 * struct firewall_spec - Specification on firewall reference.
 *
 * @dev:	Reference on a firewall controller.
 * @args:	Such data pointed is opaque and relevant only to
 *		the firewall controller driver instance being used.
 * @nargs:	Number of arguments.
 */
struct firewall_spec {
	const struct device *dev;
	uint32_t nargs;
	const uint32_t *args;
};

/*
 * Loop over each firewall_spec controllers of table
 */
#define for_each_firewall(firewall_tbl, firewall, n_firewall, i)	\
	for (i = 0, firewall = ((struct firewall_spec *)firewall_tbl);	\
	     i < (n_firewall);						\
	     i++, firewall++)

typedef int (*firewall_controller_set_conf_t)(const struct firewall_spec *spec);
typedef int (*firewall_controller_check_access_t)(const struct firewall_spec *spec);
typedef int (*firewall_controller_acquire_access_t)(const struct firewall_spec *spec);
typedef int (*firewall_controller_release_access_t)(const struct firewall_spec *spec);

struct firewall_controller_api {
	firewall_controller_set_conf_t		set_conf;
	firewall_controller_check_access_t	check_access;
	firewall_controller_acquire_access_t	acquire_access;
	firewall_controller_release_access_t	release_access;
};

/**
 * @brief Set the config of access controlled by the device
 *
 * On success, the access is set and ready when this function
 * returns.
 *
 * @param spec reference on firewall specification.
 * @return 0 on success, negative errno on failure.
 */
static inline int firewall_set_configuration(const struct firewall_spec *spec)
{
	const struct firewall_controller_api *api;

	if ((!spec) || !device_is_ready(spec->dev))
		return -ENODEV;

	api = spec->dev->api;

	if (!api || !api->set_conf)
		return -ENOTSUP;

	return api->set_conf(spec);
}

/**
 * @brief Check if the access is authorized for the consumer
 * and the given firewall settings according to the
 * configuration of its firewall controller.
 *
 * @param spec reference on firewall specification.
 * @return 0 on success, negative errno on failure.
 */
static inline int firewall_check_access(const struct firewall_spec *spec)
{
	const struct firewall_controller_api *api;

	if ((!spec) || !device_is_ready(spec->dev))
		return -ENODEV;

	api = spec->dev->api;

	if (!api || !api->check_access)
		return -ENOTSUP;

	return api->check_access(spec);
}

/**
 * @brief Check if consumer can access to resource and
 * acquire potential access
 *
 * @param spec reference on firewall specification.
 * @return 0 on success, negative errno on failure.
 */
static inline int firewall_acquire_access(const struct firewall_spec *spec)
{
	const struct firewall_controller_api *api;

	if ((!spec) || !device_is_ready(spec->dev))
		return -ENODEV;

	api = spec->dev->api;

	if (!api || !api->acquire_access)
		return -ENOTSUP;

	return api->acquire_access(spec);
}

/**
 * @brief Release resource obtained by a call to
 * firewall_acquire_access()
 *
 * @param spec reference on firewall specification.
 * @return 0 on success, negative errno on failure.
 */
static inline int firewall_release_access(const struct firewall_spec *spec)
{
	const struct firewall_controller_api *api;

	if ((!spec) || !device_is_ready(spec->dev))
		return -ENODEV;

	api = spec->dev->api;

	if (!api || !api->release_access)
		return -ENOTSUP;

	return api->release_access(spec);
}

/* Not used directly */
#define _ACCESS_CTRLS_NAME(node_id) \
	_CONCAT(_access_ctrls, DEVICE_DT_NAME_GET(node_id))

#define _ACCESS_CTRL_IDX_BY_NAME(node_id, name) \
	DT_PHA_IDX_BY_NAME(node_id, access_controllers, name)

#define _ACCESS_CTRL_ARRAY_NAME(node_id, idx) \
	_CONCAT(_CONCAT(_ACCESS_CTRLS_NAME(node_id), _array_), idx)

#define _ACCESS_CTRL_ARRAY_NAME_BY_NAME(node_id, name) \
	_ACCESS_CTRL_ARRAY_NAME(node_id, _ACCESS_CTRL_IDX_BY_NAME(node_id, name))

#define _ACCESS_CTRL_PHA_NAME(node_id, idx) \
	_CONCAT(_CONCAT(_ACCESS_CTRLS_NAME(node_id), _pha_), idx)

#define _ACCESS_CTRL_PHA_NAME_BY_NAME(node_id, idx) \
	_ACCESS_CTRL_PHA_NAME(node_id, _ACCESS_CTRL_IDX_BY_NAME(node_id, name))

#define _ACCESS_CTRL_ITEM(idx, node_id)						\
	{									\
		.dev = DEVICE_DT_GET(DT_ACCESS_CTRL_PH_BY_IDX(node_id, idx)),	\
		.args = DT_ACCESS_CTRL_ARRAY_GET(node_id, idx),			\
		.nargs = DT_ACCESS_CTRL_ARRAY_LEN_BY_IDX(node_id, idx),		\
	}

#define _ACCESS_CTRL_ARRAY_DEFINE(idx, node_id)					\
	COND_CODE_1(DT_PHA_HAS_ARRAY_BY_IDX(node_id, access_controllers, idx),	\
	(static const uint32_t _ACCESS_CTRL_ARRAY_NAME(node_id, idx)[] =	\
		DT_PHA_ARRAY_BY_IDX(node_id, access_controllers, idx)), ())

#define _ACCESS_CTRL_SPEC_DEFINE(idx, node_id)						\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, access_controllers, idx),			\
	(static const struct firewall_spec _ACCESS_CTRL_PHA_NAME(node_id, idx) =	\
			_ACCESS_CTRL_ITEM(idx, node_id)), ())

#define _ACCESS_CTRLS_SPEC_DEFNE(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, access_controllers),		\
	(static const struct firewall_spec _ACCESS_CTRLS_NAME(node_id)[] =	\
	 {									\
		LISTIFY(DT_ACCESS_CTRLS_NUM(node_id),				\
			_ACCESS_CTRL_ITEM,					\
			(,), node_id)						\
	 }),())

/**
 * Macros to define and get specific Device tree information
 * of access-controllers property by index or name
 */
#define DT_ACCESS_CTRL_PH_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, access_controllers, idx)

#define DT_ACCESS_CTRL_PH_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, access_controllers, name)

#define DT_ACCESS_CTRL_ARRAY_BY_IDX(node_id, idx) \
	DT_PHA_ARRAY_BY_IDX(node_id, access_controllers, idx)

#define DT_ACCESS_CTRL_ARRAY_BY_NAME(node_id, name) \
	DT_PHA_ARRAY_BY_NAME(node_id, access_controllers, name)

#define DT_ACCESS_CTRL_ARRAY_LEN_BY_IDX(node_id, idx) \
	DT_PHA_ARRAY_LEN_BY_IDX(node_id, access_controllers, idx)

#define DT_INST_ACCESS_CTRL_ARRAY_LEN_BY_IDX(inst, idx) \
	DT_ACCESS_CTRL_ARRAY_LEN_BY_IDX(DT_DRV_INST(inst), idx)

#define DT_ACCESS_CTRL_ARRAY_LEN_BY_NAME(node_id, name) \
	DT_PHA_ARRAY_LEN_BY_NAME(node_id, access_controllers, name)

#define DT_ACCESS_CTRL_ARRAY_DEFINE(node_id, idx) \
	_ACCESS_CTRL_ARRAY_DEFINE(idx, node_id)

#define DT_INST_ACCESS_CTRL_ARRAY_DEFINE(inst, idx) \
	DT_ACCESS_CTRL_ARRAY_DEFINE(DT_DRV_INST(inst), idx)

#define DT_ACCESS_CTRL_ARRAY_DEFINE_BY_NAME(node_id, name)			\
	_ACCESS_CTRL_ARRAY_DEFINE(_ACCESS_CTRL_IDX_BY_NAME(node_id, name),	\
				  node_id)

#define DT_INST_ACCESS_CTRL_ARRAY_DEFINE_BY_NAME(inst, name) \
	DT_ACCESS_CTRL_ARRAY_DEFINE_BY_NAME(DT_DRV_INST(inst), name)

#define DT_ACCESS_CTRL_ARRAY_GET(node_id, idx) \
	_ACCESS_CTRL_ARRAY_NAME(node_id, idx)

#define DT_INST_ACCESS_CTRL_ARRAY_GET(inst, idx) \
	_ACCESS_CTRL_ARRAY_NAME(DT_DRV_INST(inst), idx)

#define DT_ACCESS_CTRL_ARRAY_GET_BY_NAME(node_id, name)				\
	_ACCESS_CTRL_ARRAY_NAME(node_id,					\
				_ACCESS_CTRL_IDX_BY_NAME(node_id, name))

#define DT_INST_ACCESS_CTRL_ARRAY_GET_BY_NAME(inst, name)			\
	DT_ACCESS_CTRL_ARRAY_GET_BY_NAME(DT_DRV_INST(inst), name)

#define DT_ACCESS_CTRL_DEFINE_BY_IDX(node_id, idx) \
	_ACCESS_CTRL_ARRAY_DEFINE(idx, node_id); \
	_ACCESS_CTRL_SPEC_DEFINE(idx, node_id)

#define DT_INST_ACCESS_CTRL_DEFINE_BY_IDX(node_id, idx) \
	DT_ACCESS_CTRL_DEFINE_BY_IDX(DT_DRV_INST(inst), idx)

#define DT_ACCESS_CTRL_DEFINE_BY_NAME(node_id, name)				\
	DT_ACCESS_CTRL_DEFINE_BY_IDX(node_id,					\
				     _ACCESS_CTRL_IDX_BY_NAME(node_id, name))	\

#define DT_INST_ACCESS_CTRL_DEFINE_BY_NAME(inst, name) \
	DT_ACCESS_CTRL_DEFINE_BY_NAME(DT_DRV_INST(inst), name)

#define DT_ACCESS_CTRL_GET_BY_IDX(node_id, idx)				\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, access_controllers, idx),	\
		    (&_ACCESS_CTRL_PHA_NAME(node_id, idx)),		\
		    (NULL))

#define DT_INST_ACCESS_CTRL_GET_BY_IDX(inst, idx) \
	DT_ACCESS_CTRL_GET_BY_IDX(DT_DRV_INST(inst), idx)

#define DT_ACCESS_CTRL_GET_BY_NAME(node_id, name)				\
	DT_ACCESS_CTRL_GET_BY_IDX(node_id,					\
				  _ACCESS_CTRL_IDX_BY_NAME(node_id, name))

#define DT_INST_ACCESS_CTRL_GET_BY_NAME(inst, name) \
	DT_ACCESS_CTRL_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * Macros to define and get all references of access-controllers property
 */
#define DT_ACCESS_CTRLS_NUM(node_id) \
	DT_PROP_LEN_OR(node_id, access_controllers, 0)

#define DT_INST_ACCESS_CTRLS_NUM(inst) \
	DT_ACCESS_CTRLS_NUM(DT_DRV_INST(inst))

#define DT_ACCESS_CTRLS_DEFINE(node_id)					\
	LISTIFY(DT_ACCESS_CTRLS_NUM(node_id),				\
		_ACCESS_CTRL_ARRAY_DEFINE, (;), node_id);		\
	_ACCESS_CTRLS_SPEC_DEFNE(node_id)

#define DT_INST_ACCESS_CTRLS_DEFINE(inst) \
	DT_ACCESS_CTRLS_DEFINE(DT_DRV_INST(inst))

#define DT_ACCESS_CTRLS_GET(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, access_controllers),	\
		    (_ACCESS_CTRLS_NAME(node_id)), (NULL))

#define DT_INST_ACCESS_CTRLS_GET(inst) \
	DT_ACCESS_CTRLS_GET(DT_DRV_INST(inst))

/** @} */

#endif /* INCLUDE_FIREWALL_H_ */
