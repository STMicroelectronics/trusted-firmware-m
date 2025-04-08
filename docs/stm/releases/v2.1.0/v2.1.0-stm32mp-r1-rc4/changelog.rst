
#####################
v2.1.0-stm32mp-r1-rc4
#####################

based on :doc:`tfm 2.1.0 </releases/2.1.0>`

******************
New major features
******************

- Add entopy framework and stm32 RNG driver.
- Update DDR encryption.
- Manage DDR authentication.
- Fix otp mapping management.
- Update mp23xx support.
- Update start/stop sequence of cortex A.
- Add scmi channel for Cortex A35 secure.
- Provide API for SCMI services to M33 NS.
- Add support of stm32mp21 cut 1.1.
- Add mce encryption for stm32mp21.

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
