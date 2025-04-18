######################################################
About TF-M Authentication Keys and Secret Provisioning
######################################################

************
Introduction
************

This document provides an overview of the process for generating and utilizing
TF-M authentication keys, specifically focusing on elliptic curve P-256 keys.
These keys are essential for ensuring the integrity and authenticity of firmware
and other secure components via root of trust. The guide includes detailed
instructions and examples for generating these keys using OpenSSL and ST tools,
creating hash binaries, and provisioning secrets on STM32 platforms using U-Boot
and the ``stm32key`` command.

*************
Generate Keys
*************

The elliptic curve P-256 keys can be generated using standard tools.
The document provides examples using OpenSSL and ST tools:

Example with OpenSSL (Method used here):

.. code-block:: bash

    openssl ecparam -genkey -name prime256v1 -noout -out privateKey00.pem
    openssl ecparam -genkey -name prime256v1 -noout -out privateKey01.pem
    openssl ecparam -genkey -name prime256v1 -noout -out privateKey02.pem

Example with ST Tools:

.. code-block:: bash

    STM32_KeyGen_CLI -abs . -pwd <pswd_k1> <pswd_k2> <pswd_k3> <pswd_k4> <pswd_k5> <pswd_k6> <pswd_k7> <pswd_k8> -n 8

Only the files ``privateKey00``, ``privateKey01``, and ``privateKey02`` are
necessary for further steps. Note that hashes produced by``STM32_KeyGen_CLI``
cannot be used for TF-M.

.. note::

   Remove the passphrase from the keys is strongly recomended. Otherwise, the
   password will be requested multiple times (4-5 times) during the TF-M build
   process. Unfortunately, there is no straightforward way to avoid this, as the
   tools used by TF-M do not support passing the password as an argument.

To remove the password with OpenSSL (example with ``privateKey00``):

.. code-block:: bash

    openssl ec -in privateKey00.pem -out tfm-privateKey00.pem

********************
Create Hash Binaries
********************

To generate the TF-M public key hash, it is recommended to use OpenSSL. TF-M
requires the hash of the DER format of the public key, which includes the header
and not just the parameters. This document provides a detailed explanation of
how to generate the hash data. An example process for ``privateKey00`` is
provided:

.. code-block:: bash

    # Extract public key in DER format
    openssl ec -in privateKey00.pem -pubout -outform DER -out public_key00.der

    # Hash the key and store the result in a file
    openssl dgst -sha256 -binary public_key00.der > hash00.bin

************************************
Flash OTP Using stm32key with U-Boot
************************************

There are multiple ways to provision OTP on the STM32 platform. ST recommends
using the `Secure Secret Provisioning <https://wiki.st.com/stm32mpu/wiki/Secure_Secret_Provisioning_(SSP)_overview>`_
method. However, there is also a dedicated command in U-Boot named ``stm32key``.
The usage of this command is described in this `wiki page <https://wiki.st.com/stm32mpu/wiki/How_to_use_U-Boot_stm32key_command>`_.

The following secrets can be provisioned:

- **TFM-M33-FW-PKH**: Hash of the Public Key for M33TDCID TF-M Firmware
- **TFM-DDR-FW-PKH**: Hash of the Public Key for M33TDCID DDR Firmware
- **TFM-A35-FW-PKH**: Hash of the Public Key for M33TDCID A35 bare metal Firmware
- **TFM-IAK**: Trusted Firmware M initial attestation symmetric key

The rest of this section will present how to use ``stm32key`` to fuse
`TFM-M33-FW-PKH``. The other secrets can be provisioned in a similar way.


Prerequisite
============

The prerequiste are available on ST wiki `here <https://wiki.st.com/stm32mpu/wiki/How_to_use_U-Boot_stm32key_command>`_.


Step One: Select Secret
=======================

The key is selected with the command ``stm32key select <secret>``, where
``<secret>`` is the name of the OTP section related to your secret. Use the
command ``stm32key list`` to see the list of secrets that ``stm32key`` can
provision.

.. code-block::

    stm32key select TFM-M33-FW-PKH


Step Two: Load Secret File in DDR
=================================

The file containing the secret must be available in DDR before proceeding with
the ``stm32key``command. In the next step, this file is assumed to be loaded at
address ``0x84000000``. The file is copied to the bootfs section of the SD card.

.. code-block::

    load mmc 0#bootfs 0x84000000 hash00.bin

The data loaded can be checked with the following command:

.. code-block::

    stm32key read 0x84000000


Step Three: Fuse Secret
=======================

To write and lock the keys in OTP, use the command:

.. code-block::

    stm32key fuse 0x84000000

.. warning::

   Verify the keys before confirming the operation as this operation is
   **irreversible**.

After the previous command, the device contains the keys to authenticate images,
and it can be verified with the command:

.. code-block::

    stm32key read

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
