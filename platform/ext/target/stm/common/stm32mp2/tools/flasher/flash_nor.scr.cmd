# Generate boot.scr.uimg:
# ./tools/mkimage  -C none -A arm -T script -d flash_nor.scr.cmd boot.scr
#
setenv bl2_file bl2.stm32
setenv tfm_file tfm_s_ns_signed.bin
setenv fwddr_file ddr4_pmu_train.bin

setenv bl2_p_offset	0x0000000
setenv bl2_s_offset	0x0040000
setenv fwddr_p_offset	0x0080000
setenv fwddr_s_offset	0x00C0000
setenv tfm_p_offset	0x0100000
setenv tfm_s_offset	0x0A00000
setenv tfm_ps_offset	0x1300000

setenv ddr_tmp1 0x90000000
setenv ddr_tmp2 0x92000000

setenv cmp_nor 'if cmp.b ${ddr_tmp1} ${ddr_tmp2} ${filesize}; then echo =>OK; else echo =>ERROR; exit ;fi'

echo ===== erase bl2, fw_ddr, primary secondary slot =====
mtd erase nor1 ${bl2_p_offset} 0x1300000

echo ===== write ${bl2_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${bl2_file}
mtd write nor1 ${ddr_tmp1} ${bl2_p_offset} ${filesize}
mtd write nor1 ${ddr_tmp1} ${bl2_s_offset} ${filesize}

echo ===== write ${fwddr_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${fwddr_file}
mtd write nor1 ${ddr_tmp1} ${fwddr_p_offset} ${filesize}
mtd write nor1 ${ddr_tmp1} ${fwddr_s_offset} ${filesize}

echo ===== write ${tfm_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${tfm_file}
mtd write nor1 ${ddr_tmp1} ${tfm_p_offset} ${filesize}
mtd write nor1 ${ddr_tmp1} ${tfm_s_offset} ${filesize}

echo ===== Check ${tfm_file} =====
mtd read nor1 ${ddr_tmp2} ${tfm_p_offset} ${filesize}
run cmp_nor
mtd read nor1 ${ddr_tmp2} ${tfm_s_offset} ${filesize}
run cmp_nor

echo ===== Check ${bl2_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${bl2_file}
mtd read nor1 ${ddr_tmp2} ${bl2_p_offset} ${filesize}
run cmp_nor
mtd read nor1 ${ddr_tmp2} ${bl2_s_offset} ${filesize}
run cmp_nor

echo ===== Check ${fwddr_file} =====
ext4load mmc 0:8 ${ddr_tmp1} ${fwddr_file}
mtd read nor1 ${ddr_tmp2} ${fwddr_p_offset} ${filesize}
run cmp_nor
mtd read nor1 ${ddr_tmp2} ${fwddr_s_offset} ${filesize}
run cmp_nor

echo ===== success =====
