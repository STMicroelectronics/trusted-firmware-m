--------
STM32MP2
--------

The STM32MP2 devices embed: Arm cortex A35, M33 cores.

`Reference manuals <https://www.st.com/en/microcontrollers-microprocessors.html>`__

TF-M is available on cortex M33 and could be view:

- like simple coprocessor launched by cortex A35 (default)
- owner of Trusted Domain (TDCID) and launched by bootrom (``STM32_M33TDCID=ON``)

Directory content
^^^^^^^^^^^^^^^^^

- stm/common/stm32mp2/bl2:
   Specific code for bl2 platform initialisation.

- stm/common/stm32mp2/cmsis_driver:
   Cmsis interface to drivers.

- stm/common/stm32mp2/device:
   Specific startup and linker script.

- stm/common/stm32mp2/native_driver:
   Drivers for tfm system.

- stm/common/stm32mp2/secure:
   Secure adaptation for stm32mp2 SOC device.

Specific Software Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Not yet defined.

Write software on target
^^^^^^^^^^^^^^^^^^^^^^^^

Not yet defined.

-------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
