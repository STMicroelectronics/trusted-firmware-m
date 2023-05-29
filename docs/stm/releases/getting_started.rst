===============
Getting started
===============

Prerequisites
-------------

- Follow the :doc:`SW requirements guide </getting_started/tfm_getting_started>` to set up your environment.

Software
--------

.. tabs::

   .. group-tab:: Trusted Firmware M

      Trusted Firmware M for stm32 platforms is available on:

      -  `TF-M gitweb <NOT YET DEFINED>`_
      -  `TF-M gerrit <NOT YET DEFINED>`_

      .. code:: bash

         $ git clone "ssh://NOT YET DEFINED"

      .. code:: bash

         $ git fetch "ssh://NOT YET DEFINED"

   .. group-tab:: TF M tests

      Trusted Firmware M tests for stm32 platforms is available on:

      -  `TF-M-tests gitweb <NOT YET DEFINED>`_
      -  `TF-M-tests gerrit <NOT YET DEFINED>`_

      .. code:: bash

         $ git clone "ssh://NOT YET DEFINED"

      .. code:: bash

         $ git fetch "ssh://NOT YET DEFINED"

   .. group-tab:: TF M extra

      Trusted Firmware M extra contains stm32 platform tests:

      -  `TF-M-extras gitweb <NOT YET DEFINED>`_
      -  `TF-M-extras gerrit <NOT YET DEFINED>`_

      .. code:: bash

         $ git clone "ssh://NOT YET DEFINED"

      .. code:: bash

         $ git fetch "ssh://NOT YET DEFINED"


Build
-----

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
