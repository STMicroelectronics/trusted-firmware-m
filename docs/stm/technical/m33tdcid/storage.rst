#######
Storage
#######

************************
Internal Trusted Storage
************************

ITS is stored on backup sram. This memory can be shared between cortex A and M, the ``RISAF1`` allows to define the access right.

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

PS is stored on external serial nor and need security setup:

- memory mapped region ``RISAF2``
- port muxing ``OCTOSPI IO manager`` and pin muxing
- peripheral security access ``RISUP``


+------+-----------------------+
|      | FREE                  |
|      +-----------------------+
|      | TFM_HAL_PS_FLASH_AREA |
|      +-----------------------+
| SNOR | tfm secondary slot    |
|      +-----------------------+
|      | ...                   |
|      +-----------------------+
|      | primary bl2 slot      |
+------+-----------------------+

--------------

*Copyright (c) 2023 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
