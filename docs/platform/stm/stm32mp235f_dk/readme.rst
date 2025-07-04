###############
stm32mp235f_dk
###############

The `stm32mp235f_dk`_ evaluation board is designed as a complete demonstration and development platform for evaluating
the capabilities of the `STM32MP23`_ microprocessor devices.

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

      - sdcard (sdmmc1): ``-DSTM32_BOOT_DEV=sdmmc1``
      - emmc (sdmmc2): ``-DSTM32_BOOT_DEV=sdmmc2``

    * To build in **A35TDCID or copro** mode: ``-DSTM32_M33TDCID=OFF``

    * To use external device tree for your components (BL2 | S | NS):

      - ``-DDTS_EXT_DIR=<external_dt_path>``
      - ``-DDTS_BOARD_S=<dts_file_secure>``
      - ``-DDTS_BOARD_NS=<dts_file_non_secure>``
      - ``-DDTS_BOARD_BL2=<dts_file_bl2>``

    * If your board isn't provisionned with real secret you should enable dummy provisionning.
      - ``-DTFM_DUMMY_PROVISIONING=ON``
    * When dummy provisioning enabled you can also override OTP values with dummy provisionning.
      - ``-DSTM32_OVERRIDE_OTP=ON``

Building TF-M secure and non secure with|out regression tests
=============================================================

clone the tf-m-tests repository in ``<TF-M-TESTS_DIRECTORY>``.

.. tabs::

   .. group-tab:: Linux

      .. code:: bash

         $ cmake -S <TF-M-TESTS_DIRECTORY>/tests_reg/spe -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp235f_dk \
                 -DCONFIG_TFM_SOURCE_PATH=<TF-M_DIRECTORY> \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DSTM32_BOOT_DEV=sdmmc1 \
                 -DTFM_PARTITION_PROTECTED_STORAGE=OFF \
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
                 -DTFM_PLATFORM=stm/stm32mp235f_dk \
                 -DCONFIG_TFM_SOURCE_PATH=<TF-M_DIRECTORY> \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DSTM32_BOOT_DEV=sdmmc1 \
                 -DTFM_PARTITION_PROTECTED_STORAGE=OFF \
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
                 -DTFM_PLATFORM=stm/stm32mp235f_dk \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DSTM32_BOOT_DEV=sdmmc1 \
                 -DTFM_PARTITION_PROTECTED_STORAGE=OFF \
                 -DCMAKE_BUILD_TYPE=Relwithdebinfo
         $ cmake --build <BUILD_DIRECTORY>/build_spe -- install

   .. group-tab:: Windows

      .. code:: bash

         $ cmake -S <TF-M_DIRECTORY> -B <BUILD_DIRECTORY>/build_spe \
                 -DTFM_PLATFORM=stm/stm32mp235f_dk \
                 -DTFM_TOOLCHAIN_FILE=<TF-M_DIRECTORY>/toolchain_GNUARM.cmake \
                 -DTFM_PROFILE=profile_medium \
                 -DSTM32_M33TDCID=ON \
                 -DSTM32_BOOT_DEV=sdmmc1 \
                 -DTFM_PARTITION_PROTECTED_STORAGE=OFF \
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

      * The Secure and Non Secure log are mixed on uart5 of stm32mp235f_dk board.
        You could setup a terminal with options 115200,8N1, no HW flow control.

      .. code::

	 [INF] welcome to MCUboot: v2.1.0-stm32mp-[version]
	 [INF] board: stm32mp235f disco
	 [WRN] This device was provisioned with dummy keys.

	 [WRN] This device is NOT SECURE

	 [WRN] NV_MM_COUNTER_INIT is not suitable for production! This device is NOT SECURE

   .. group-tab:: M33-TD flavor

      * To debug, add this flag ``-DDEBUG_AUTHENTICATION=FULL`` at build command line. With this flag, BL2 opens debug port and waits a debugger connection.

      * The Secure and Non Secure log are mixed on uart5 of stm32mp235f_dk board.
        You could setup a terminal with options 115200,8N1, no HW flow control.

      .. code::

	 [INF] welcome to MCUboot: v2.1.0-stm32mp-[version]
	 [INF] board: stm32mp235f disco
	 [INF] dts: stm32mp235f-dk-cm33tdcid-ostl-sdcard-bl2.dts
	 [INF] boot device: sdmmc1
	 [INF] mcu sysclk: 400000000
	 [INF] Starting bootloader
	 [WRN] This device was provisioned with dummy keys.

	 [WRN] This device is NOT SECURE

	 [WRN] NV_MM_COUNTER_INIT is not suitable for production! This device is NOT SECURE

	 [INF] PSA Crypto init done, sig_type: EC-P256
	 [INF] Primary   slot: version=0.1.0+0
	 [INF] Secondary slot: version=0.1.0+0
	 [INF] Image 1 RAM loading to 0xe060000 is succeeded.
	 [INF] Image 1 loaded from the primary slot
	 [INF] BL2: image 1, enable DDR-FW
	 [INF] Primary   slot: version=2.1.2+0
	 [INF] Secondary slot: version=2.1.2+0
	 [INF] Image 0 RAM loading to 0x80000000 is succeeded.
	 [INF] Image 0 loaded from the primary slot
	 [INF] Bootloader chainload address offset: 0x104400
	 [INF] Jumping to the first image slot
	 [INF] welcome to TF-M: v2.1.0-stm32mp-[version]
	 [INF] board: stm32mp235f disco
	 [INF] dts: stm32mp235f-dk-cm33tdcid-ostl-sdcard-s.dts
	 [WAR] This device was provisioned with dummy keys.
	 [WAR] This device is NOT SECURE
	 [    0.000000] SCP-firmware v2.13.0-stm32mp-[version]
	 [    0.000000]
	 [    0.000000] [FWK] Module initialization complete!
	 Non-Secure system starting..

-------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*

.. _stm32mp235f_dk: https://www.st.com/en/evaluation-tools/stm32mp235f-dk.html
.. _STM32MP23: https://www.st.com/en/microcontrollers-microprocessors/stm32mp2-series.html
.. _How_to_protect_the_coprocessor_firmware: https://wiki.st.com/stm32mpu/wiki/How_to_protect_the_coprocessor_firmware
