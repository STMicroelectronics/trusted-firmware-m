v1.7.0-stm32mp25-r11
====================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- m33tdcid:

    - Add firewall support needed by omm
    - Add rifaware support for pwr, rtc, tamp, hpdma, exti, fmc
    - Add sdmmc framework and sdmmc2 driver
    - Fill flash map with gpt at runtime

.. warning::
   This device is **not secure**:

   - HUK depend of saes driver which is not yet implemented.
   - Profile definition is not defined (study on going).

.. warning::
   Due to toolchain fix you must used specific tf-m-tests:

   - git-web: `tf-m-tests <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=summary>`_
   - tag: `v1.7.0-stm32mp25-r6 <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=shortlog;h=refs/tags/v1.7.0-stm32mp25-r6>`_
   - Add flag to cmake command line: ``-DTFM_TEST_REPO_PATH=<TF-M-tests PATH>``

.. warning::
   in m33tdcid, scmi server is not fully functional.
   the cortexA generates illegal access.
   the tfm tests are passed without starting cortexA.

.. warning::
   Only stm32mp257 soc revC is supported.

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
