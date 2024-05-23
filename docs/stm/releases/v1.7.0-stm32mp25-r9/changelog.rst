v1.7.0-stm32mp25-r9
===================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- Use external repository to fetch ddr binary `stm32-ddr-phy-binary <https://github.com/STMicroelectronics/stm32-ddr-phy-binary>`_
- Fix issue on external devicetree support
- Regenerate the device tree if any of the dts, dtsi or .h dependencies are affected.
- m33tdcid:

    - Update ddr: timming, remove deprecated code
    - Add rif aware support for: rcc, omm, pinctrl

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
   Only stm32mp257 soc revB is supported.

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

*Copyright (c) 2024 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
