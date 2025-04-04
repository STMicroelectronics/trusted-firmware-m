###########################
Initial Attestation Service
###########################

************
Introduction
************

Initial Attestation Services are based on OTP ``BSEC`` registers.

The TDCID owner is in charge to:

- Shadow OTP registers in SRAM memory (first 4K).
- Protect this area (secure & Read Only).

Shadow layout
=============

+-------+------------+
|       |            |
|       | OTP status |
|       |   [368]    |
|       |            |
|       +------------+
|       |            |
| SRAM1 | OTP values |
|       |   [368]    |
|       |            |
|       +------------+
|       | State      |
|       +------------+
|       | Magic      |
+-------+------------+

The OTP mapping is defined in reference Manual.

********
Services
********

* Security lifecycle (LCS): return secured (``PLAT_OTP_LCS_SECURED``), if the device is closed_locked and
  provisioning_done and disable_scan (depend of otp 18 and 124).

* Implementation ID: otp[5..7].

* Entropy seed: otp[332..347].

* IAK: key => otp[348..355]. len, type and id are fixed.

.. note::

   The optional services are not implemented.

*********
Exception
*********

* HUK: The ``SAES`` hardware block provides an DHUK.

* Profile definition: study on going.

.. warning::

   Like HUK & profile definition is not yet available a workaround has been implemented
   but is not suitable for production! This device is **NOT SECURE**.

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
