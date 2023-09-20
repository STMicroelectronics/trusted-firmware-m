/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define CREATE_OBJ_LEVEL(object, level)				\
		__##object##_##level##_start = .;		\
		KEEP(*(SORT(._##object##_##level?_*)));	\
		KEEP(*(SORT(._##object##_##level??_*)));


	initlevel :
	{
		/*
		 * link in initialization objects for all objects that are
		 * automatically initialized by the kernel; the objects are
		 * sorted in the order they will be initialized (i.e. ordered
		 * by level, sorted by priority within a level)
		 */
		__init_start = .;
		CREATE_OBJ_LEVEL(init, EARLY)
		CREATE_OBJ_LEVEL(init, ARCH)
		CREATE_OBJ_LEVEL(init, PRE_CORE)
		CREATE_OBJ_LEVEL(init, CORE)
		CREATE_OBJ_LEVEL(init, POST_CORE)
		CREATE_OBJ_LEVEL(init, REST)
		__init_end = .;
	} > FLASH

	device_area : ALIGN(4)
	{
		_device_list_start = .;
		KEEP(*(SORT(._device.static.*_?_*)));
		KEEP(*(SORT(._device.static.*_??_*)));
		_device_list_end = .;
	} > FLASH
