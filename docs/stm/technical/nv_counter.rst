##########
Nv counter
##########

Nv counters are stored in backup register (``TAMP``). 128 (32 bit) backup registers are available.

+------------------+-------------------------+
|                  |                         |
|                  +-------------------------+
|                  | PLAT_NV_COUNTER_BL2_[4] |
|                  +-------------------------+
| Backup Registers | PLAT_NV_COUNTER_PS_[3]  |
|                  +-------------------------+
|                  | init value              |
|                  +-------------------------+
|                  |                         |
+------------------+-------------------------+

If ``init value`` contains the right value, the counters are provisioned and
can be used. complete mapping are available `here <https://wiki.st.com/stm32mpu/wiki/STM32MP2_backup_registers>`_

.. note::

   * The backup registers are implemented in the backup domain that remains powered-on by VBAT when
     the VDD power is switched off.
   * A tamper detection can erase the backup registers

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
