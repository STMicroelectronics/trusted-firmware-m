#############
Memory layout
#############

***************
Firmware layout
***************

The firmware is formated with:

* stm32 header
* tfm secure signed
* non secure signed

This firmware can be generated with sign_rproc_fw.py provided by **Optee**

    .. code:: bash

       $ <OPTEE_PATH>/scripts/sign_rproc_fw.py sign \
       --in <BUILD_DIRECTORY>/bin/tfm_s.elf.strip \
       --in <BUILD_DIRECTORY>/bin/tfm_ns.elf.strip \
       --out <BUILD_DIRECTORY>/bin/rproc_tfm_s_ns_sign.stm32 \
       --key <OPTEE_PATH>/keys/default_rproc.pem \
       --plat-tlv SBOOTADDR 0x80000000

    .. note::

       * Before being signed, the elf files must be striped (.elf.strip).
       * SBOOTADDR defines vector table of first stage (tfm_s).

**********
Ram layout
**********

.. note::
       
   On Arm® Cortex®-M33 the vector table must be naturally aligned to
   a power of two whose alignment value is greater than or equal to
   Number of exceptions supported x 4.

   example for (16 internal irq + 320 peripheral IRQ):

   - vtor size = ((16 + 320) * 4) = 1344
   - vtor alignment = 2048 => 0x800

   So ``S_CODE_START`` and ``NS_CODE_START`` must be aligned on 0x800

+-----------------+-----------------------+
|                 |                       |
|                 +-----------------------+
|                 |                       |
|                 | NS_DATA               |
|                 |                       |
|                 +-----------------------+
|                 |                       |
|                 | S_DATA                |
|                 |                       |
| DDR             +---------+-------------+
|                 |         | code        |
|                 |         +-------------+
|                 | NS_CODE | vector      |
|                 +---------+-------------+
|                 |         | code        |
|                 |         +-------------+
|                 |         | veneer      |
|                 |         +-------------+
|                 | S_CODE  | vector      |
+-----------------+---------+-------------+

--------------

*Copyright (c) 2023 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
