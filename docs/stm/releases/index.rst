########
Releases
########

.. toctree::
    :maxdepth: 1
    :glob:

    getting_started
    platforms.rst
    changelog.rst

############
Branch plans
############

.. uml::

    @startuml
    concise "TrustedFirmware.org" as main
    concise "releases v1.7.0-stm32mp25-dev" as stm_v1.7.0
    concise "releases v2.1.0-stm32mp-dev" as stm_v2.1.0

    @main
        0 is development #line:002052

    @stm_v1.7.0
        0 is {hidden}
        +6 is {-} #line:002052
        main -> stm_v1.7.0
        note top of stm_v1.7.0: based on TF-Mv1.7.0
        +1 is "v1.7.0-stm32mp25-r1" #D4007A;line:002052
        +4 is {-} #line:002052
        +1 is "v1.7.0-stm32mp25-rX" #D4007A;line:002052
        +4 is {-} #line:002052
        +1 is "v1.7.0-stm32mp25-alpha-r1-rc3" #D4007A;line:002052
        +5 is "END" #D4007A;line:002052
        +1 is {hidden}

    @stm_v2.1.0
        0 is {hidden}
        +17 is {-} #line:002052
        main -> stm_v2.1.0
        note top of stm_v2.1.0: based on TF-Mv2.1.0
        +1 is "v2.1.0-stm32mp-r1-rc1" #39A9DC;line:002052
        +4 is {-} #line:002052
        +1 is "v2.1.0-stm32mp-rX" #39A9DC;line:002052
        +4 is {-} #line:002052

    @enduml

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
