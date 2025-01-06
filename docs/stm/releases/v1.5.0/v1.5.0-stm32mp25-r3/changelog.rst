v1.5.0-stm32mp25-r3
===================

based on :doc:`tfm 1.5.0 </releases/1.5.0>`

New major features
------------------

- for rpmsg non secure application (example):

    - add ipc share memory with non secure access right (sau region)
    - fix interrupt configuration

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
