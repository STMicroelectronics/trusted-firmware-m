stm32mp257f_ev
##############

The stm32mp257f_ev board is dedicated to evaluate and experimentation
on :doc:`STM32MP2 SoC of stmicroelectronics </platform/stm/common/stm32mp2/readme>`

.. Note::

   Only copro mode is enabled, M33 TDCID is not yet available


Build
*****

.. code:: bash

   $ cmake -S <SRC_DIRECTORY> -B <BUILD_DIRECTORY> \
           -DTFM_PLATFORM=stm/stm32mp257f_ev \
           -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake \
           -DTFM_PROFILE=profile_small \
           -DCMAKE_BUILD_TYPE=debug
   $ make  -C <BUILD_DIRECTORY> install

.. Note::
    Currently, applications can only be built using GCC (GNU ARM Embedded toolchain).

    Profile supported:

    * :doc:`TF-M Profile small design </configuration/profiles/tfm_profile_small>`
    * :doc:`TF-M Profile medium design </configuration/profiles/tfm_profile_medium>`

    Flags to add on cmake command:

    * To run S and NS regression tests add -DTEST_S=ON -DTEST_NS=ON.
    * To run S and NS regression tests of internal tf-m-tests add -DTFM_TEST_REPO_PATH=<TF-M-tests PATH>

Flashing, run and debugging
***************************

The M33 copro firmware can be loaded by cortex A35 with these commands

.. code:: bash

   $ cd /sys/class/remoteproc/remoteproc0
   $ echo "firmware name" > firmware
   $ echo start > state

.. Note::
   The firmware file must be in /lib/firmware

In developpment (debug open), the gdb/openocd can load and debug M33 firmware

* Run openocd with this script::

     set GDB_PORT 50000
     set AP_NUM 8
     #compatible dk/eval
     source [find board/stm32mp25x_dk.cfg]
     gdb_breakpoint_override hard

* Run gdb of your arm toolchain, then execute theses commands::

     target remote :50000
     set pagination off
     set print pretty

     print "remove hold pen m33"
     # set RCC_S RCC_CPUBOOTCR BOOT_CPU2 1
     monitor stm32mp25x.axi mww 0x54200434 0x1
     # set RCC_S RCC_C2RSTCSETR C2RST 0x1
     monitor stm32mp25x.axi mww 0x5420040c 0x1

     # set SCB VTOR 0x80000000
     monitor stm32mp25x.m33 mww 0xe000ed08 0x80000000
     monitor stm32mp25x.m33 invoke-event examine-end

     load <BUILD_DIRECTORY>/bin/tfm_s.elf
     add-symbol-file <BUILD_DIRECTORY>/bin/tfm_s.elf
     set $sp=0x80900800

     thbreak Reset_Handler
     commands
            print "Secure Reset_Handler"
            thbreak main
            commands
                   thb ns_agent_tz_init_c
	           commands
		          #sau is configured
                          #so access in Secure on ns memory => generate an ns request on rif
                          restore <BUILD_DIRECTORY>/bin/tfm_ns.bin binary 0x80100000
                          add-symbol-file <BUILD_DIRECTORY>/bin/tfm_ns.elf
	           end

                   thbreak Reset_Handler
	           commands
		          print "Non Secure Reset_Handler"
                          #add your ns breakpoints
                   end
            end
     end

Console
*******

The Secure and Non Secure log are mixed on uart5 of stm32mp257 soc.
You could setup a terminal with options 115200,8N1, no HW flow control.

.. code::

     [INF] Beginning TF-M provisioning
     [WRN] TFM_DUMMY_PROVISIONING is not suitable for production! This device is NOT SECURE
     [Sec Thread] Secure image initializing!
     TF-M isolation level is: 0x00000002
     Booting TF-M v1.7.0-stm32mp25-r2
     Creating an empty ITS flash layout.
     [INF][Crypto] Provisioning entropy seed... complete.
     Non-Secure system starting...


-------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
