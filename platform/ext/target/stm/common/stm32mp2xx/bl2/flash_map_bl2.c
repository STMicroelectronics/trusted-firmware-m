/*
 * Copyright (c) 2025, STMicroelectronic. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include "flash_map/flash_map.h"
#include "target.h"
#include "Driver_Flash.h"

/* When undefined FLASH_DEV_NAME_0 or FLASH_DEVICE_ID_0 , default */
#if !defined(FLASH_DEV_NAME_0) || !defined(FLASH_DEVICE_ID_0)
#define FLASH_DEV_NAME_0  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_0 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_1 or FLASH_DEVICE_ID_1 , default */
#if !defined(FLASH_DEV_NAME_1) || !defined(FLASH_DEVICE_ID_1)
#define FLASH_DEV_NAME_1  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_1 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_2 or FLASH_DEVICE_ID_2 , default */
#if !defined(FLASH_DEV_NAME_2) || !defined(FLASH_DEVICE_ID_2)
#define FLASH_DEV_NAME_2  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_2 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_3 or FLASH_DEVICE_ID_3 , default */
#if !defined(FLASH_DEV_NAME_3) || !defined(FLASH_DEVICE_ID_3)
#define FLASH_DEV_NAME_3  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_3 FLASH_DEVICE_ID
#endif

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Flash device names must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_1;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_3;

#if !defined(FLASH_DRIVER_LIST)
/* Default Drivers list */
const ARM_DRIVER_FLASH *flash_driver[] = {
    &FLASH_DEV_NAME,
#if FLASH_DEV_NAME_0 != FLASH_DEV_NAME
    &FLASH_DEV_NAME_0,
#endif
#if FLASH_DEV_NAME_1 != FLASH_DEV_NAME
    &FLASH_DEV_NAME_1,
#endif
#if FLASH_DEV_NAME_2 != FLASH_DEV_NAME
    &FLASH_DEV_NAME_2,
#endif
#if (MCUBOOT_IMAGE_NUMBER == 2)
#if FLASH_DEV_NAME_1 != FLASH_DEV_NAME
    &FLASH_DEV_NAME_1,
#endif
#if FLASH_DEV_NAME_3 != FLASH_DEV_NAME
    &FLASH_DEV_NAME_3,
#endif
#endif /* (MCUBOOT_IMAGE_NUMBER == 2) */
};
#else
/* Platform driver list */
const ARM_DRIVER_FLASH *flash_driver[] = FLASH_DRIVER_LIST;
#endif /* !defined(FLASH_DRIVER_LIST) */
const int flash_driver_entry_num = ARRAY_SIZE(flash_driver);

const struct flash_area flash_map[] = {
    {
        .fa_id = FLASH_AREA_0_ID,
        .fa_device_id = FLASH_DEVICE_ID_0,
        .fa_driver = &FLASH_DEV_NAME_0,
        .fa_off = FLASH_AREA_0_OFFSET,
        .fa_size = FLASH_AREA_0_SIZE,
    },
    {
        .fa_id = FLASH_AREA_2_ID,
        .fa_device_id = FLASH_DEVICE_ID_2,
        .fa_driver = &FLASH_DEV_NAME_2,
        .fa_off = FLASH_AREA_2_OFFSET,
        .fa_size = FLASH_AREA_2_SIZE,
    },
    {
        .fa_id = FLASH_AREA_1_ID,
        .fa_device_id = FLASH_DEVICE_ID_1,
        .fa_driver = &FLASH_DEV_NAME_1,
        .fa_off = FLASH_AREA_1_OFFSET,
        .fa_size = FLASH_AREA_1_SIZE,
    },
    {
        .fa_id = FLASH_AREA_3_ID,
        .fa_device_id = FLASH_DEVICE_ID_3,
        .fa_driver = &FLASH_DEV_NAME_3,
        .fa_off = FLASH_AREA_3_OFFSET,
        .fa_size = FLASH_AREA_3_SIZE,
    },
};

const int flash_map_entry_num = ARRAY_SIZE(flash_map);

int boot_get_image_exec_ram_info(uint32_t image_id,
                                 uint32_t *exec_ram_start,
                                 uint32_t *exec_ram_size)
{
    int32_t rc =  -1;

    if (image_id == 0 || image_id == 2) {
        (*exec_ram_start) = IMAGE_EXECUTABLE_RAM_START;
        (*exec_ram_size) = IMAGE_EXECUTABLE_RAM_SIZE;
        rc = 0;
    }
    else if (image_id == 1 || image_id == 3) {
        (*exec_ram_start) = DDR_FW_DEST_ADDR;
        (*exec_ram_size) = FLASH_AREA_1_SIZE;
        rc = 0;
    }

    return rc;
}
