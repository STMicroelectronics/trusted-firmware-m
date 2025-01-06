###############
Getting started
###############

*************
Prerequisites
*************

- Follow the :doc:`SW requirements guide </getting_started/tfm_getting_started>` to set up your environment.

********
Software
********

.. tabs::

   .. group-tab:: Trusted Firmware M

      Trusted Firmware M for internal stm32 platforms is available on:

      -  `TF-M gitweb <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/trusted-firmware-m.git;a=summary>`_
      -  `TF-M gerrit <https://gerrit.st.com/q/project:mpu%252Foe%252Fst%252FTF-M%252Ftrusted-firmware-m>`_

      .. code:: bash

         $ git clone "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/trusted-firmware-m"

      .. code:: bash

         $ git fetch "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/trusted-firmware-m"

   .. group-tab:: TF M tests

      Trusted Firmware M tests for internal stm32 platforms is available on:

      -  `TF-M-tests gitweb <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-tests.git;a=summary>`_
      -  `TF-M-tests gerrit <https://gerrit.st.com/q/project:mpu%252Foe%252Fst%252FTF-M%252Ftf-m-tests>`_

      .. code:: bash

         $ git clone "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/tf-m-tests"

      .. code:: bash

         $ git fetch "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/tf-m-tests"

   .. group-tab:: TF M extra

      Trusted Firmware M extra contains stm32 platform tests:

      -  `TF-M-extras gitweb <https://gerrit.st.com/gitweb?p=mpu/oe/st/TF-M/tf-m-extras.git;a=summary>`_
      -  `TF-M-extras gerrit <https://gerrit.st.com/q/project:mpu%252Foe%252Fst%252FTF-M%252Ftf-m-extras>`_

      .. code:: bash

         $ git clone "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/tf-m-extras"

      .. code:: bash

         $ git fetch "ssh://<frqXXX>@gerrit.st.com:29418/mpu/oe/st/TF-M/tf-m-extras"

*****
Build
*****

Instructions for your board:

.. toctree::
    :maxdepth: 1
    :glob:

    ../../platform/stm/stm32*/**
    ../../platform/stm/nucleo*/**
    ../../platform/stm/b_*/**

--------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
