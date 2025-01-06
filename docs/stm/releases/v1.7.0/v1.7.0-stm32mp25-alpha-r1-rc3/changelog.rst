v1.7.0-stm32mp25-alpha-r1-rc3
=============================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- m33tdcid:

  - fix re-start sequence of cortexA
  - fix Illegal access with SDMMC1/CIDx[1,2,etc.], Secure and Privilege writing to MCUSRAM2

.. warning::
   This device is **not secure**:

   - HUK depend of saes driver which is not yet implemented.
   - Profile definition is not defined (study on going).

.. warning::
   If you would use your own NS firmware, add this flag ``-DNS=FALSE`` on cmake command

.. warning::
   In m33tdcid, CortexA must be started by Non secure firmware with specific CPU service.
   An example is available on internal tf-m-test (file: main_ns.c function: tfm_ns_start_copro).

.. warning::
   If you build with tfm ns firmware (default configuration), you must clone stm32 tf-m-tests repository:

   - git-web: `tf-m-tests <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=summary>`_
   - tag: `v1.7.0-stm32mp25-alpha-r1-rc3 <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=shortlog;h=refs/tags/v1.7.0-stm32mp25-alpha-r1-rc3>`_
   - Add flag to cmake command line: ``-DTFM_TEST_REPO_PATH=<TF-M-tests PATH>``

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
