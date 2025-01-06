########
STM32MP2
########

The STM32MP2 devices embed: Arm cortex A35, M33 cores.

`Reference manuals <https://www.st.com/en/microcontrollers-microprocessors.html>`__

TF-M is available on cortex M33 and could be view:

- like simple coprocessor launched by cortex A35 (default)
- owner of Trusted Domain (TDCID) and launched by bootrom (``STM32_M33TDCID=ON``)

*****************
Directory content
*****************

- stm/common/stm32mp2xx/bl2:
   Specific code for bl2 platform initialisation.

- stm/common/stm32mp2xx/cmsis_driver:
   Cmsis interface to drivers.

- stm/common/stm32mp2xx/lib
   Specific libraries like qsort, scp...

- stm/common/stm32mp2xx/linker_scripts:
   Specific linker script for bl2, tfm secure and non secure.

- stm/common/stm32mp2xx/native_driver:
   Framework and drivers for system (with device tree support).

- stm/common/stm32mp2xx/stm32mp[21|23|25]
   Soc variant directory to define specific needs like config, cmakelist, startup...

- stm/common/stm32mp2xx/secure:
   Secure adaptation for stm32mp2 SOC device.

- stm/common/stm32mp2xx/secure_fw
   Secure firmware and the exported interfaces for application.

******************************
Specific Software Requirements
******************************

Not yet defined.

************************
Write software on target
************************

Not yet defined.

-------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
