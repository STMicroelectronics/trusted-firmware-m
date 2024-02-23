#!/usr/bin/env python3
#
# Copyright (c) 2022 STMicroelectronics.
#
# SPDX-License-Identifier: Apache-2.0
import os
import re
import argparse
from elftools.elf.elffile import ELFFile
import Stm32ImageAddHeader

def main():
    parser = argparse.ArgumentParser(description="Extract info from elf")
    # Add the arguments
    parser.add_argument('-e', '--elf_file', help='elf file', required=True)
    parser.add_argument('-b', '--bin_file', help='binary file', required=True)
    parser.add_argument('-o', '--out_file', help='output file', required=True)
    parser.add_argument('-bt', '--binary_type', help='binary file', required=True)
    parser.add_argument('-tr', '--truncate_val', help='truncate file in byte', required=False, default=0, type=int)
    parser.add_argument('-v_maj', '--major_version', help='major version', required=False, default=1, type=int)
    parser.add_argument('-v_min', '--minor_version', help='minor version', required=False, default=0, type=int)

    args = parser.parse_args()

    if os.path.isfile(args.elf_file) is False:
        raise Exception("No such file:{}".format(args.elf_file))

    if os.path.isfile(args.bin_file) is False:
        raise Exception("No such file:{}".format(args.bin_file))

    with open(args.elf_file, "rb") as file:
        elf_file = ELFFile(file)
        text_section = elf_file.get_section_by_name(".text")

        load_address = text_section.header["sh_addr"]
        entry_point = elf_file.header.e_entry

    binary_type = int(args.binary_type, 16)

    print("elf file     :{}".format(args.elf_file))
    print("bin file     :{}".format(args.bin_file))
    print("load address :0x{0:X}".format(load_address))
    print("entry point  :0x{0:X}".format(entry_point))
    print("binary type  :0x{0:X}".format(binary_type))
    print("header ver   :{}.{}".format(args.major_version, args.minor_version))

    stm32im = Stm32ImageAddHeader.Stm32Image(args.major_version, args.minor_version, entry_point, load_address, binary_type)
    ret = stm32im.generate(args.bin_file, args.out_file)

    if ret != 0:
      print("Aborted")
      return ret

    stm32im.print_header()
    print("%s generated" % args.out_file)

    #truncate the file
    if args.truncate_val!=0 :
        f = open(args.out_file, "a")
        f.truncate(args.truncate_val)
        f.close()

if __name__ == "__main__":
    main()
