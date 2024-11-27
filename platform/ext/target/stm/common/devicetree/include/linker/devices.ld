/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "region_defs.h"

#define CREATE_OBJ_LEVEL(object, level)				\
		__##object##_##level##_start = .;		\
		KEEP(*(SORT(._##object##_##level?_*)));	\
		KEEP(*(SORT(._##object##_##level??_*)));

    /*
     * To support firmware image encryption by OP-TEE (TF-M running on a coprocessor),
     * we need to align all sections to have a modulo 16 size. This size is the atomic block size to
     * encrypt. Use TFM_LINKER_DEFAULT_ALIGNMENT that is set to 32 bytes.
     * Each section declared should respect this template:
     *
     * section_name : ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT)
     * {
     *     ...
     *     ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT)
     * }
     */

	initlevel : ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT)
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
        	. = ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT);
	} > FLASH

	device_area : ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT)
	{
		_device_list_start = .;
		KEEP(*(SORT(._device.static.*_?_*)));
		KEEP(*(SORT(._device.static.*_??_*)));
		_device_list_end = .;
		. = ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT);
	} > FLASH

	.__device_handles_pass1 : ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT)
	{
		KEEP (*(.__device_handles_pass1));
		. = ALIGN(TFM_LINKER_DEFAULT_ALIGNMENT);
	} > FLASH