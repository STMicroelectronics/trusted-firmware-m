v1.5.0-stm32mp25-r2
===================

based on :doc:`tfm 1.5.0 </releases/1.5.0>`

New major features
------------------

- print STM32_BOARD_MODEL cmake variable at runtine
- update stm32 clock driver for !M33TDCID
- fix its and nv counter bug in tfm framework (for small profile)
- add usart5 instance
- fix bug to set isolation level 1
- add copro service get/set state (running, reset, off)
- add uboot script to flash bl2, tfm binaries on nor
- add stm32 image tool
- bl2: open debug on DEBUG_AUTHENTICATION=FULL
- fixup ospi boot
- move backup domain init in bl2

Tested platforms
----------------

Tests result TEST_S & TEST_NS for:

.. toctree::
    :maxdepth: 1
    :glob:

    *_test

.. include:: issues.rst
.. include:: fixed.rst

--------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
