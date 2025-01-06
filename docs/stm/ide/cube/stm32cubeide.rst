############
STM32cubeIDE
############

.. caution::

    This section is a POC, is not the official ST solution. If this cmake solution become mainstream this will be done by stm32cube team.

The goal of this sections is to try to based eclipse IDE (stm32cubeide) on TFM builder without adaptation. In order to:

- Reduce porting effort needed for each release.
- Use TFM tools for user integration of partition, irq... (based on yaml description)
- possibility to refer on TFM documentation

Some solutions has been dismissed to find one

******************
CDT eclipse [Fail]
******************

Eclipse integrate a cmake extension in CDT eclipse, but there is too much instability with "cdt make nature".


Lot of errors: cmake vs make, Project -> properties -> cmake to run cmake-gui.

********************
cmake4eclipse [Fail]
********************

Error while eclipse marketplace installs

"The following solutions are not available: cmake4eclipse2.1.4"

Makefile Project [OK]
*********************

An alternatif is possible.

Create a "Makefile project with existing code" and
replace the cmake-gui integration (above solutions, which used "project nature")
by a build target to call cmake-gui.

Sw requirement
==============

-  :doc:`TFM sw requirements guide </getting_started/tfm_getting_started>`
-  Addon for windows: git-bash >= 2.25.0
-  STM32cubeIDE
-  cmake with cmake-gui:

   if cmake version is older, install a cmake > 3.15 with cmake-gui.

   On Ubuntu, qt is requires:

   .. code-block:: bash

      sudo apt-get install qt5-default

   .. code-block:: bash

      wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2.tar.gz
      tar xvfz cmake-3.20.2.tar.gz
      cd cmake-3.20.2
      ./bootstrap --qt-gui && make && sudo make install


Run STM32CubeIDE
================

This section is required if you must define a proxy, else run STM32CubeIDE normally.

Before to run stm32cubeide in a terminal, you must define:

-  Proxy with set_proxy
-  Set stm32cubeide path in PATH variable.

Create Project
==============

-  file -> new -> Makefile project with existing code

   *  project name: trusted-firmware-m
   *  existing code location: browse on tfm root
   *  toolchain: Cross GCC

.. image:: stm32cubeide_newproject.png
    :align: center

-  Project -> properties

   *  C/C++ Build

      *  Builder Setting

         *  Build command: make -C ./build

.. image:: stm32cubeide_properties.png
    :align: center

-  Project -> properties

   *  C/C++ Build

      *  Behavior

         *  Build: install
         *  Clean: clean

.. image:: stm32cubeide_behavior.png
    :align: center

-  Project -> build target -> create

   *  Target name: config-ui
   *  Build target: -S ./ -B ./build
   *  Build command: cmake-gui

.. image:: stm32cubeide_config-ui.png
    :align: center

-  Project -> build target -> create

   if you wish, you can define a preset config of your target.
   After you can modify this config by config-ui.

   *  Target name: config-stm32mp257_fpga-debug
   *  Build target:

      .. tabs::

         .. group-tab:: Linux

            -DTFM_PLATFORM=stm/stm32mp257_fpga -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake -DTEST_S=ON -DTEST_NS=ON -DTFM_PSA_API=ON -DTFM_ISOLATION_LEVEL=2 -DCMAKE_BUILD_TYPE=debug

         .. group-tab:: Windows

            -DTFM_PLATFORM=stm/stm32mp257_fpga -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake -DTEST_S=ON -DTEST_NS=ON -DTFM_PSA_API=ON -DTFM_ISOLATION_LEVEL=2 -DCMAKE_BUILD_TYPE=debug -G "Unix Makefiles"

   *  Build command: cmake -S ./ -B ./build

.. image:: stm32cubeide_config-stm32mp257.png
    :align: center

-  Project -> build target -> create

   *  Target name: install
   *  Build target: install
   *  Build command: make -C ./build

.. image:: stm32cubeide_install.png
    :align: center

Configure & Build
=================

-  Project -> Build Targets -> Build

   *  Select configurator then click on "Build"

      *  Add entry:

         If you have already preset a configuration with a specific "build target"
         (config-XX), you do not need to "add entry", but you can change the flags.
         Otherwise, set cmake flags with "Add Entry" (depending on the target).

         .. tabs::

            .. group-tab:: STM32MP257_FPGA

               .. csv-table::
                  :header: "Name", "Type", "Value"

                  "TFM_PLATFORM", "STRING", "stm/stm32mp257_fpga"
                  "TFM_TOOLCHAIN_FILE", "STRING", "<ABS_PATH>/toolchain_GNUARM.cmake"
                  "TFM_ISOLATION_LEVEL", "STRING", "2"
                  "TEST_S", "STRING", "ON"
                  "TEST_NS", "STRING", "ON"
                  "TFM_PSA_API", "STRING", "ON"
                  "CMAKE_BUILD_TYPE", "STRING", "debug"

            .. group-tab:: NUCLEO-L552ZE-Q

               .. csv-table::
                  :header: "Name", "Type", "Value"

                  "TFM_PLATFORM", "STRING", "stm/nucleo_l552ze_q"
                  "TFM_TOOLCHAIN_FILE", "STRING", "<ABS_PATH>/toolchain_GNUARM.cmake"

            .. group-tab:: STM32L562E-DK

               .. csv-table::
                  :header: "Name", "Type", "Value"

                  "TFM_PLATFORM", "STRING", "stm/stm32l562e_dk"
                  "TFM_TOOLCHAIN_FILE", "STRING", "<ABS_PATH>/toolchain_GNUARM.cmake"

      *  Configure:

         *  Unix Makefiles
         *  Specify native compilers -> browse on your cross gcc/g++ files for C and C++

      *  Generate:

         if you would set or modify an option (example TFM_ISOLATION_LEVEL) => just click on the properties.
         After, not forget to **configure** and **generate**.

.. image:: stm32cubeide_configurator_variable.png
    :align: center

-  Project -> Build Targets -> Build

   *  Select "install" then click on "Build" (corresponding "hammer" icone) to compile TFM.

.. Note::

    "Build Targets" could be access by lot of way in eclipse

Debug
=====

.. Note::
    A debug configuration using OpenOCD is available for target stm32mp25_fpga

Pre-requisites
--------------

-  You have to install Standalone OpenOCD eclipse plugin named `Eclipse Embedded C/C++ <https://marketplace.eclipse.org/content/eclipse-embedded-cc/promo>`_

   Use Eclipse menu "**Help->Eclipse Marketplace**" and search keyword "openocd"

-  You have to define environment variables $OCD_PATH, $OCD_BIN and $OCD_SCRIPTS :

   .. tabs::

      .. group-tab:: Linux

         .. code-block:: bash

            export OCD_PATH=<your_ocd_root_path>
            export OCD_SCRIPTS=$OCD_PATH/scripts
            export OCD_BIN=bin/openocd-linux-x86_64/bin/openocd

      .. group-tab:: Windows

         .. code-block:: bash

            export OCD_PATH=<your_ocd_root_path>
            export OCD_SCRIPTS=$OCD_PATH/scripts
            export OCD_BIN=bin/openocd-win32/bin/openocd.exe

-  Restart STM32CubeIDE to take into account OpenOCD plugin and environment variables

Launch Debug Configuration
--------------------------

-  Before to launch "**TFM Debug Configuration**", make sure to have built successfully the TFM binaries

-  Reset the stm332mp25_fpga target

-  From your TFM STM32CUbeIDE project, Right click on "**TFM Debug.launch**" file -> Debug AS -> TFM Debug

--------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
