v1.7.0-stm32mp25-r4
===================

based on :doc:`tfm 1.7.0 </releases/1.7.0>`

New major features
------------------

- Move ITS on backup sram on stm32mp257f_ev board.
- Map Initial Attestation services on platform devices:

  - iak, lcs, entropy seed, implementation id on otp driver.
  - iak type & len are fixed according to the selected profile
  - non volatile counter defined on backup reg.

- At start (in development), the platform can be provisioning by dummy value if it is not secure and ``TFM_DUMMY_PROVISIONING``, ``STM32_PROV_FAKE`` are enabled.


.. warning::
   This device is **not secure**:

   - HUK depend of saes driver which is not yet implemented.
   - Profile definition is not defined (study on going).

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

*Copyright (c) 2023 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
