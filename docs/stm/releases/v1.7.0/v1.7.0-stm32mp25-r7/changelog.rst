v1.7.0-stm32mp25-r7
===================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- Fix aci build: parallel and tag format
- Add i2c framework and stm32 driver
- Add stm32mp257 soc revB support

- Add stm32mp257f ev1 m33tdcid support:

  - Move bl2 stack on sram1
  - Add regulator framework and stm32 driver
  - Add ddr driver for bl2
  - Add risab driver
  - Update rifsc, risaf, iac, serc driver
  - Add rif aware feature to gpio driver
  - Update clock driver to support m33tdcid
  - Generate bsec_mirror in bl2

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
   Only stm32mp257 soc revB supports m33tdcid.
   Due to issue `174897 <https://intbugzilla.st.com/show_bug.cgi?id=174897>`_ the cortex A is not started.


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
