# Generate boot.scr.uimg:
# ./tools/mkimage  -C none -A arm -T script -d flash_nor.scr.cmd boot.scr
#
setenv bl2_file bl2.stm32
setenv tfm_file tfm_s_ns_signed.bin
setenv bl2_offset   0x0
setenv slot0_offset 0x20000
setenv slot2_offset 0x120000
setenv ddr_tmp1 0x90000000
setenv ddr_tmp2 0x91000000

setenv cmp_nor 'if cmp.b ${ddr_tmp1} ${ddr_tmp2} ${filesize}; then echo =>OK; else echo =>ERROR; exit ;fi'

echo ===== erase bl2, primary secondary slot =====
mtd erase nor1 ${bl2_offset} 0x220000

echo ===== write ${bl2_file} =====
ext4load mmc 0:8 ${ddr_tmp1}  ${bl2_file}
mtd write nor1 ${ddr_tmp1} ${bl2_offset} ${filesize}

echo ===== write ${tfm_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${tfm_file}
mtd write nor1 ${ddr_tmp1} ${slot0_offset} ${filesize}
mtd write nor1 ${ddr_tmp1} ${slot2_offset} ${filesize}

echo ===== Check ${tfm_file} =====
mtd read nor1 ${ddr_tmp2} ${slot0_offset} ${filesize}
run cmp_nor
mtd read nor1 ${ddr_tmp2} ${slot2_offset} ${filesize}
run cmp_nor

echo ===== Check bl2.stm32 =====
ext4load mmc 0:8 ${ddr_tmp1} ${bl2_file}
mtd read nor1 ${ddr_tmp2} ${bl2_offset} ${filesize}
run cmp_nor

echo ===== success =====
