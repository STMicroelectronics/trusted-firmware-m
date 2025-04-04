
#####################
v2.1.0-stm32mp-r1-rc3
#####################

based on :doc:`tfm 2.1.0 </releases/2.1.0>`

******************
New major features
******************

- Add stm32mp21 support for m33 and a35 td flavor mode.
- Add support of stm32mp215f dk board.
- Update mailbox sharing, needed to PSA support.
- Activate CACHE support
- Improve sdmmc and ospi throughput

****************
Tested platforms
****************

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
