v1.7.0-stm32mp25-alpha-r1-rc3
=============================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

-  M33-TD flavor:

  - Correction of Arm® Cortex®-A re-start sequence 
  - fix illegal access with SDMMC1/CIDx[1,2,etc.], secure and privilege writing to MCUSRAM2

.. warning::
   This device is **not secure**:

   - HUK depends on SAES driver which is not yet implemented.
   - Profile definition is not defined (study on going).

.. warning::
   If you would use your own nonsecure firmware, add this flag ``-DNS=FALSE`` on cmake command

.. warning::
   With M33-TD flavor, Arm® Cortex®-A  must be started by nonsecure firmware with specific CPU service.
   An example is available on internal tf-m-test (file: main_ns.c function: tfm_ns_start_copro).

.. warning::
   If you build with TF-M nonsecure firmware (default configuration), you must clone stm32 tf-m-tests repository:

   - git: tf-m-tests
   - tag: v1.7.0-stm32mp25-alpha-r1-rc3
   - Add flag to cmake command line: ``-DTFM_TEST_REPO_PATH=<TF-M-tests PATH>``

.. warning::
   Only stm32mp25x lines-RevY are supported.

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

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
