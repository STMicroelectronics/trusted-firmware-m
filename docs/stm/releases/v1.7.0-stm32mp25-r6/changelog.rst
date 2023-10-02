v1.7.0-stm32mp25-r6
===================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- Add configuration support from device tree files:

  - Add generic device structure to reference config, driver data,
    init function, initlevel, API of one device.
  - Add python device tree library to parse and generate c header files from
    dts files.
  - Update linker file to integrate initlevel support.
  - Update drivers needed by stm32mp257f ev1 board.
  - Add dts files of stm32mp2 SoC and stm32mp257f ev1 board.

- Align RCC driver and clock framework on TFA/OPTEE.
- Fix toolchain issue >=11.3

.. warning::
   This device is **not secure**:

   - HUK depend of saes driver which is not yet implemented.
   - Profile definition is not defined (study on going).

   Due to toolchain fix you must used ST tf-m-tests:

   - git-web: `tf-m-tests <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=summary>`_
   - tag: `v1.7.0-stm32mp25-r6 <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=shortlog;h=refs/tags/v1.7.0-stm32mp25-r6>`_
   - Add flag to cmake command line: ``-DTFM_TEST_REPO_PATH=<TF-M-tests PATH>``

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
