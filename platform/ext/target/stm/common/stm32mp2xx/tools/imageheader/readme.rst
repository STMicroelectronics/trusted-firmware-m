ImageHeader
-----------

ImageHeader allows to add a stm32 header needed by bootrom

Origin
^^^^^^

Stm32ImageAddHeader script come from:

    .. code:: bash

       $ git clone "ssh://frq09524@gerrit.st.com:29418/mpu/stm32cube/tools/imageheader"

Add stm32_imgtool.py to extract load adress and entry point of elf file then call use previous script to add the header 

How to use
^^^^^^^^^^

example:

    .. code:: bash

       $ ./stm32_imgtool -e <elf file> -b <binary file> -o <output file> -bt <binary type>

