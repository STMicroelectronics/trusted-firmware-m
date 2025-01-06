===================
v1.4.0-stm32mp25-r1
===================

based on :doc:`tfm 1.4.0 </releases/1.4.0>`

New major features
------------------

  - Rebased on tfm 1.4.0
  - Add cubeide project & debug files
  - Fix non secure init for binary != TEST_NS
  - Fix to execute psa arch tests
  - Rework stm32 log level (cmake flag)
  - Fix uart initialization
  - Move bl2 stack to sysram
  - Rework fake ddr flash to memory mapped flash driver
    (common for ddr, bkp sram, bkpreg)
  - Add spi flash driver (quad/octal) for bl2 and tfm (read/write)
  - Move ITS on backup sram
  - Move NVcounter on backup register
  - Move PS on ospi (default) or backup sram (cmake flag)
  - Add cmake flag for boot device ddr (default) or ospi

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
