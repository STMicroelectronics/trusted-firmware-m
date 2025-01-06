v1.5.0-stm32mp25-r1
===================

based on :doc:`tfm 1.5.0 </releases/1.5.0>`

New major features
------------------

- Rebased on tfm 1.5.0 => adaptations
- Add IAC driver
- Add serf driver
- Fix vector table alignment (Exceptions supported x 4)^2
- Add i/d cache management on TFM (ddr)
- Fix clock isb issue
- Add M33TDCID config flag to isolate TDCID code
- Fix qspi memory access right for PS
- Add reset framework
- Add stm32 reset driver
- Add platform service to hold and start the co-processor
- Modify TF-M tests to add a specific test suites
- Add STM32 extra test suite

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
