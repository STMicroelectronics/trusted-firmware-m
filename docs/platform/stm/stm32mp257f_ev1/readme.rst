###############
stm32mp257f_ev1
###############

The `stm32mp257f_ev1`_ evaluation board is designed as a complete demonstration and development platform for evaluating
the capabilities of the `STM32MP25`_ microprocessor devices.

*****
Build
*****

These build generate:

- The SPE elf and binaries in ``<BUILD_DIRECTORY>/build_spe/bin``.
- Artifacts for building application (NSPE) in ``<BUILD_DIRECTORY>/build_spe/api_ns``
- Non secure demo and binary concatenated (tfm_s_ns.bin) in ``<BUILD_DIRECTORY>/build_ns/bin``

.. Note::
    Currently, applications can only be built using GCC (GNU ARM Embedded toolchain).

    For **cmake** command line, used **absolute path**.

    Flags to add on cmake config command (cmake command without ``--build``):

    * Profile supported:

      - :doc:`TF-M Profile medium design </configuration/profiles/tfm_profile_medium>`: ``-DTFM_PROFILE=profile_medium``

    * **M33TDCID** boot device (bl2 dt file must be aligned):

      - default, serial nor (ospi): ``-DSTM32_BOOT_DEV=ospi``
      - sdcard (sdmmc1): ``-DSTM32_BOOT_DEV=sdmmc1``
      - emmc (sdmmc2): ``-DSTM32_BOOT_DEV=sdmmc2``

    * To build in **A35TDCID or copro** mode: ``-DSTM32_M33TDCID=OFF``

    * To use external device tree for your components (BL2 | S | NS):

      - ``-DDTS_EXT_DIR=<external_dt_path>``
      - ``-DDTS_BOARD_S=<dts_file_secure>``
      - ``-DDTS_BOARD_NS=<dts_file_non_secure>``
      - ``-DDTS_BOARD_BL2=<dts_file_bl2>``

Building TF-M secure and non secure with|out regression tests
=============================================================

clone the tf-m-tests repository in ``<TF-M-TESTS_DIRECTORY>``.

.. tabs::

   .. group-tab:: Linux

      .. code:: bash

         $ cmake -S <TF-M-TESTS_DIRECTORY>/tests_reg/spe -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DCONFIG_TFM_SOURCE_PATH=<TF-M_DIRECTORY> \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DTEST_S=ON -DTEST_NS=ON \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo
         $ cmake --build <BUILD_DIRECTORY>/build_spe -- install

         $ cmake -S <TF-M-TESTS_DIRECTORY>/tests_reg -B <BUILD_DIRECTORY>/build_ns \
                 -DCONFIG_SPE_PATH=<BUILD_DIRECTORY>/build_spe/api_ns \
                 -DSTM32_M33TDCID=ON
         $ cmake --build <BUILD_DIRECTORY>/build_ns

   .. group-tab:: Windows

      .. code:: bash

         $ cmake -S <TF-M-TESTS_DIRECTORY>/tests_reg/spe -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DCONFIG_TFM_SOURCE_PATH=<TF-M_DIRECTORY> \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DTEST_S=ON -DTEST_NS=ON \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo -G "Unix Makefiles"
         $ cmake --build <BUILD_DIRECTORY>/build_spe -- install

         $ cmake -S <TF-M-TESTS_DIRECTORY>/tests_reg -B <BUILD_DIRECTORY>/build_ns \
                 -DCONFIG_SPE_PATH=<BUILD_DIRECTORY>/build_spe/api_ns \
                 -DSTM32_M33TDCID=ON
         $ cmake --build <BUILD_DIRECTORY>/build_ns

.. Note::

    * To activate or disable S and|or NS regression tests modify ``-DTEST_S=ON|OFF`` ``-DTEST_NS=ON|OFF``.

Building TF-M secure only
=========================

Used this build if you used your own non secure binary.
The secure and non secure binaries must be assembled then signed (see CubeIDE process).

.. tabs::

   .. group-tab:: Linux

      .. code:: bash

         $ cmake -S <TF-M_DIRECTORY> -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo
         $ cmake --build <BUILD_DIRECTORY>/build_spe -- install

   .. group-tab:: Windows

      .. code:: bash

         $ cmake -S <TF-M_DIRECTORY> -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp257f_ev1 \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo -G "Unix Makefiles"
         $ cmake --build <BUILD_DIRECTORY>/build_spe -- install


***************************
Flashing, run and debugging
***************************

.. tabs::

   .. group-tab:: A35-TD flavor

      In A35-TDCID flavor, the Arm® Cortex®-M33 firmware can be loaded by Arm® Cortex®-A35 with these commands

      .. code:: bash

         $ cd /sys/class/remoteproc/remoteproc0
         $ echo "firmware name" > firmware
         $ echo start > state

      .. Note::
         - The firmware must be **signed**, refer to `How_to_protect_the_coprocessor_firmware`_ wiki page.
         - The firmware file must be in /lib/firmware

      * In developpment, gdb/openocd can load and debug Arm® Cortex®-M33 firmware firmware but the
        debug port must be open.

      * The Secure and Non Secure log are mixed on uart5 of stm32mp257f_ev1 board.
        You could setup a terminal with options 115200,8N1, no HW flow control.

      .. code::

         [INF] welcome to stm32mp257f eval1
         [INF] Beginning provisioning
         [INF] DUMMY_PROVISIONING is not suitable for production!
         [INF] This device is NOT SECURE

   .. group-tab:: M33-TD flavor

      * To start in m33tdcid, The :ref:`m33tdcid_flash_layout` must be loaded in external nor and the boot pin must be set to nor.

      * To debug, add this flag ``-DDEBUG_AUTHENTICATION=FULL`` at build command line. With this flag, BL2 opens debug port and waits a debugger connection.

      * The Secure and Non Secure log are mixed on uart5 of stm32mp257f_ev1 board.
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

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*

.. _stm32mp257f_ev1: https://www.st.com/en/evaluation-tools/stm32mp257f-ev1.html
.. _STM32MP25: https://www.st.com/en/microcontrollers-microprocessors/stm32mp2-series.html
.. _How_to_protect_the_coprocessor_firmware: https://wiki.st.com/stm32mpu/wiki/How_to_protect_the_coprocessor_firmware
