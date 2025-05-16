
#####################
v2.1.0-stm32mp-r1-rc5
#####################

based on :doc:`tfm 2.1.0 </releases/2.1.0>`

******************
New major features
******************

- Add nvmem framework
- Read otp map in devicetree and use bsec nvmem
- Read soc revision
- Fix debug issue on stm32mp21
- Display software version and board info

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
