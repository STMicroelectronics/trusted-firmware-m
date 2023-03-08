v1.7.0-stm32mp25-r2
===================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- Add eval board with:

    - Copro Mode, M33TDCID not supported
    - In Copro mode: secure configuration (RIF) must be aligned with TDCID component
    - Profile supported: small (isolation level 1), medium (isolation level 2)
    - Protected Storage on spi nor (octospi2)
    - Internal Trusted Storage and Non-Volatile counter on RAM emulation
    - Dummy provisioning enabled

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
