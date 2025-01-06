===================
v1.3.0-stm32mp25-r1
===================

based on :doc:`tfm 1.3.0 </releases/1.3.0>`

New major features
------------------

first team delivery with features:

    - mcuboot load S & NS images from slot 1 or 2 to ddr (ramloading)
    - risaf defined in 2 step (bl2 and tfm)
    - support of drivers systick, pwr, rcc, risaf, risup/rimu, gpio
    - support of flash driver simulate in ddr
    - support of stm32mp257_fpga board

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
