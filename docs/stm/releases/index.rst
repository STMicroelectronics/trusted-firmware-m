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
    concise "stm32mp25" as main
    concise "releases stm32mp25-dev" as dev
    concise "releases stm32mp25-fpga" as fpga
    concise "releases stm32mp25-valid" as valid

    @main
        0 is development #line:002052

    @dev
        0 is {-} #line:002052
        +5 is "vX.Y.Z-rW" #D4007A;line:002052 : no board
        +2 is {-} #line:002052
        dev -> valid
        dev -> fpga
        +6 is "vX.Y.Z-rW" #D4007A : ev1 (copro)
        +2 is {-} #line:002052
        dev -> valid
        dev -> fpga
        +6 is "vY.Y.Z-rW" #D4007A: ev1 (copro|M33TDCID)
        +2 is {-} #line:002052
        dev -> fpga

    @fpga
        0 is {-} #line:002052
        +7 is "-fpga" #D4007A;line:002052 : fpga
        +1 is {-} #line:002052
        +7 is "-fpga" #D4007A;line:002052 : fpga
        +1 is {-} #line:002052
        +7 is "-fpga" #D4007A;line:002052 : fpga
        +1 is {-} #line:002052

   @valid
        0 is {-} #line:002052
        +7 is "-valid" #D4007A;line:002052 : valid3
        +1 is {-} #line:002052
        +7 is "-valid"   #D4007A;line:002052 : valid3
        +1 is {-} #line:002052
        17 is {hidden} : obsolete


    highlight 5 to 8 #FFD300;line:DimGrey : pre-alpha
    highlight 13 to 16 #39A9DC;line:DimGrey : alpha
    highlight 21 to 24 #97C00E;line:DimGrey : beta

    @enduml

The valid3 board is gradually removed. The customer should migrate on ev1 board.
The fpga branch remains available for the next SOC derivation.

--------------

*Copyright (c) 2021 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
