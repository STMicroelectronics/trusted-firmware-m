#######
Storage
#######

************************
Internal Trusted Storage
************************

ITS is stored on backup SRAM. This memory can be shared and the ``RISAF1`` allows to define the access right (depend on Arm® Cortex®-A: Optee).

+-----------------+------------------------+
|                 | TFM_HAL_ITS_FLASH_AREA |
| BKPSRAM         +------------------------+
|                 | FREE                   |
+-----------------+------------------------+

.. note::

   - TFM_HAL_ITS_FLASH_AREA used the last 4KB of ``BKPSRAM`` (size: 8KB).
   - To retain the content of the backup domain when VDD is turned off, the VBAT pin can be connected
     to an optional standby voltage supplied from a battery or an another source.

*****************
Protected Storage
*****************

PS is stored on external SNOR and needs security setup (depend on Arm® Cortex®-A: Optee):

- memory mapped region ``RISAF2``
- port muxing ``OCTOSPI IO manager`` and pin muxing
- peripheral security access ``RISUP``

+-----------------+------------------------+
|                 | FREE                   |
| SNOR            +------------------------+
|                 | TFM_HAL_PS_FLASH_AREA  |
+-----------------+------------------------+

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
