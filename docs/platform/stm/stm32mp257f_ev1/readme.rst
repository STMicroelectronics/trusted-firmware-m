stm32mp257f_ev1
###############

The stm32mp257f_ev1 board is dedicated to evaluate and experimentation
on :doc:`STM32MP2 SoC of stmicroelectronics </platform/stm/common/stm32mp2/readme>`

Build
*****
.. tabs::

   .. group-tab:: Linux

      .. code:: bash

         $ cmake -S <SRC_DIRECTORY> -B <BUILD_DIRECTORY> \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_small \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo
         $ make  -C <BUILD_DIRECTORY> install

   .. group-tab:: Windows

      .. code:: bash

         $ cmake -S <SRC_DIRECTORY> -B <BUILD_DIRECTORY> \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_small \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo -G "Unix Makefiles"
         $ make  -C <BUILD_DIRECTORY> install


.. Note::
    Currently, applications can only be built using GCC (GNU ARM Embedded toolchain).

    Profile supported:

    * :doc:`TF-M Profile small design </configuration/profiles/tfm_profile_small>`
    * :doc:`TF-M Profile medium design </configuration/profiles/tfm_profile_medium>`

    Flags to add on cmake command:

    * To build in **M33TDCID** mode add ``-DSTM32_M33TDCID=ON``
    * To run S and NS regression tests add ``-DTEST_S=ON`` ``-DTEST_NS=ON``.
    * To run S and NS regression tests of internal tf-m-tests add ``-DTFM_TEST_REPO_PATH=<TF-M-tests PATH>``

Flashing, run and debugging
***************************
.. tabs::

   .. group-tab:: copro

      The M33 copro firmware can be loaded by cortex A35 with these commands

      .. code:: bash

         $ cd /sys/class/remoteproc/remoteproc0
         $ echo "firmware name" > firmware
         $ echo start > state

      .. Note::
         - The firmware must be **signed**, refer to OPTEE documentation.
         - The firmware file must be in /lib/firmware

      * In developpment, gdb/openocd can load and debug M33 firmware but the
        debug port must be open.

      * The Secure and Non Secure log are mixed on uart5 of stm32mp257 soc.
        You could setup a terminal with options 115200,8N1, no HW flow control.

      .. code::

         [INF] welcome to stm32mp257f eval1
         [INF] Beginning provisioning
         [INF] DUMMY_PROVISIONING is not suitable for production!
         [INF] This device is NOT SECURE

   .. group-tab:: m33tdcid

      * To start in m33tdcid, The :ref:`m33tdcid_flash_layout` must be loaded in external nor and the boot pin must be set to nor.

      * To debug, add this flag ``-DDEBUG_AUTHENTICATION=FULL`` at build command line. With this flag, BL2 opens debug port and waits a debugger connection.

      * The Secure and Non Secure log are mixed on uart5 of stm32mp257 soc.
        You could setup a terminal with options 115200,8N1, no HW flow control.

      .. code::

         [INF] welcome
         [INF] mcu sysclk: 400000000
         [INF] Starting bootloader
         [INF] Beginning provisioning
         [INF] DUMMY_PROVISIONING is not suitable for production!
         [INF] This device is NOT SECURE
         [INF] Primary   slot: version=1.7.0+0
         [INF] Secondary slot: version=1.7.0+0
         [INF] RAM loading to 0x80000000 is succeeded.
         [INF] Image 0 loaded from the primary slot
         [INF] Bootloader chainload address offset: 0x100000
         [INF] Jumping to the first image slot
         [INF] Enable Macronix quad support

         [INF] welcome to stm32mp257f eval1
         [INF] Beginning provisioning
         [INF] DUMMY_PROVISIONING is not suitable for production!
         [INF] This device is NOT SECURE
         Non-Secure system starting...

-------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
