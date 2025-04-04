#############
Memory layout
#############

************
Image layout
************

On STM32MP2 serie, the mcuboot upgrade strategy is defined to ``RAM_LOAD``: the latest image is copied to RAM and runs from there.

This involves to create one image for secure and nonsecure firmware.

The assemble.py tool is used to concatenate secure and nonsecure binary to a single binary blob (tfm_s_ns.bin).

Then wrapper.py tool signed this blob and add header and trailer
(tfm_s_ns_signed.bin).

+---------------------+------------------------+---------------+----------------+
|                     |                        | BL2_TRAILER   | BL2_TRAILER    |
|                     |                        +---------------+----------------+
|                     |                        |               | NS_CODE        |
|                     | NS_PARTITION           | IMAGE_NS_CODE | (vector, code) |
|                     +------------------------+---------------+----------------+
|                     |                        |               | S_CODE         |
|                     |                        |               | (vector,       |
|                     |                        | IMAGE_S_CODE  | veneer, code)  |
|                     |                        +---------------+----------------+
| tfm_s_ns_signed.bin | S_PARTITION            | BL2_HEADER    | BL2_HEADER     |
+---------------------+------------------------+---------------+----------------+


.. Note::
        **The header size must be defined to align vector table in RAM.**
        On Arm® Cortex®-M33 the vector table must be naturally aligned to
        a power of two whose alignment value is greater than or equal to
        Number of exceptions supported x 4.

        example for (16 internal irq + 320 peripheral IRQ):
                - vtor size = ((16 + 320) * 4) = 1344
                - vtor alignment = 2048 => 0x800

.. _m33tdcid_flash_layout:

************
Flash layout
************

The table below shows the flash layout loaded in persistent storage (external nor).

+------------------------+---------------------+--------------+
|                        |                     | NS_PARTITION |
| tfm secondary slot     | tfm_s_ns_signed.bin +--------------+
|                        |                     | S_PARTITION  |
+------------------------+---------------------+--------------+
|                        |                     | NS_PARTITION |
| tfm primary slot       | tfm_s_ns_signed.bin +--------------+
|                        |                     | S_PARTITION  |
+------------------------+---------------------+--------------+
| secondary ddr firmware | ddr4_pmu_train.bin  | binary       |
+------------------------+---------------------+--------------+
| primary ddr firmware   | ddr4_pmu_train.bin  | binary       |
+------------------------+---------------------+--------------+
| secondary bl2          | bl2.stm32           | BL2_CODE     |
+------------------------+---------------------+--------------+
| primary bl2            | bl2.stm32           | BL2_CODE     |
+------------------------+---------------------+--------------+

- The bootrom copies the bl2 image from boot device to RETRAM then identifies and executes this firmware.
- The bl2 copies a valid DDR firmware to SRAM2 and initialize ddr hardware block.
- The bl2 selects the slot actif and copies secure and nonsecure partition to ``S_IMAGE_LOAD_ADDRESS`` (DDR), then identifies and executes the secure partition.
- the protected storage is defined in external NOR device.

**********
Ram layout
**********

The ``S_IMAGE_LOAD_ADDRESS`` is defined to ``DDR_RAM_OFFSET``.

Bl2
===

+----------+----------------------+
|          |                      |
| DDR      | S_IMAGE_LOAD_ADDRESS |
|          |                      |
+----------+----------------------+
|          |                      |
+----------+----------------------+
|          |                      |
| S_RETRAM | BL2_CODE             |
|          |                      |
+----------+----------------------+
|          |                      |
+----------+----------------------+
|          | BOOT_TFM_SHARED_DATA |
|          +----------------------+
|          |                      |
| SRAM1    | BL2_DATA             |
|          |                      |
|          +----------------------+
|          | bsec_mirror          |
+----------+----------------------+

TFM
===

The vector table of ``S_CODE_START`` and ``NS_CODE_START`` meets alignment conditions by:

- ``S_IMAGE_LOAD_ADDRESS`` + ``BL2_HEADER_SIZE``.
- ``S_IMAGE_LOAD_ADDRESS`` + ``FLASH_S_PARTITION_SIZE``

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
|                 +-----------------------+
|                 | BL2_TRAILER           |
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
|                 +---------+-------------+
|                 | BL2_HEADER            |
|                 +-----------------------+
|                 |                       |
+-----------------+-----------------------+

************
Access Right
************

TFM follows FF-M 1.0 definition and should implement several isolation levels.
STM32MP2 platforms used Arm® Cortex®-M33 MPU and RIF device to enforce and manage the isolation of memory resources.

.. important::
        TF-M is loaded and executed in DDR, this device is protected by RISAF4 instance.
        RISAF4 requires a granularity aligned on 4KBytes (Region, Sub-region).
        => To do that, the common tfm linker script (l1-l2,l3) must be specific for
        stm32mp2 platform.

.. [#risaf] RISAF attributes: RO: Read Only; RW: Read Write; S: secure; NS: non-secure; P: privileged; NP: unprivileged.

.. Note::
	RISAF sub-region feature is not yet implemented

Secure
======

Code
----

+--------+-------------+----------+-------------+---------+
| vector | veneer      | TFM_Code | TFM_APP_ROT | tfm_lib |
+--------+-------------+----------+-------------+---------+
|                                 | Sub reg 2B  |         |
|                                 | (RO, S, NP) |         |
|                                 | [#risaf]_   |         |
+--------+-------------+----------+-------------+---------+
|        | Sub reg 2A  |                                  |
|        | (RO, S, NP) |                                  |
|        | [#risaf]_   |                                  |
+--------+-------------+----------------------------------+
| Base region 2 (RO, S, P) [#risaf]_                      |
+---------------------------------------------------------+

Data
----

+-----------------+------------------------+--------------------+----------+
| TFM_secure_data | TFM_APP_RW_STACK       | TFM_PSA_RW_STACK   | tfm_data |
+-----------------+------------------------+--------------------+----------+
| Sub reg 4B: Not yet used                                                 |
+-----------------+------------------------+-------------------------------+
|                 | Sub reg 4A             |                               |
|                 | (RW, S, NP) [#risaf]_  |                               |
+-----------------+------------------------+-------------------------------+
| Base region 4 (RW, S, P) [#risaf]_                                       |
+--------------------------------------------------------------------------+

.. Note::
        In Level3:
            - The TF-M APP ROT access rights will be dynamically configured on the active partition. The others will inherit from the base region..
            - All APP ROT must be aligned on RISAF4 constraints => linker script modification.

Non-secure
==========

Today, nonsecure example has no isolation. However a basic kernel uses commonly 2 isolation levels (kernel, user).

Code
----

+--------+-------------+--------------+
| vector | Kernel side | User side    |
+--------+-------------+--------------+
| Sub reg 3B: Not yet used            |
+----------------------+--------------+
|                      | Sub reg 3A   |
|                      | (RO, NS, NP) |
|                      | [#risaf]_    |
+----------------------+--------------+
| Base region 3 (RO, NS, P) [#risaf]_ |
+-------------------------------------+

Data
----

+-------------------------------------+--------------+
| Kernel side                         | User side    |
+-------------------------------------+--------------+
| Sub reg 5B: Not yet used            |              |
+-------------------------------------+--------------+
|                                     | Sub reg 5A   |
|                                     | (RW, NS, NP) |
|                                     | [#risaf]_    |
+-------------------------------------+--------------+
| Base region 5 (RW, NS, P) [#risaf]_ |              |
+-------------------------------------+--------------+

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
