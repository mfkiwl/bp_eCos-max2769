#!/bin/sh -x

rm -rf error/ devs/  hal/  infra/ install/ io/ isoinfra/ kernel/ language/ services/ ecos.ecc makefile *.map *.bin *.elf *.lst

ecosconfig new bp2016_fpga_bp
ecosconfig tree

make -j

TARGET_BASE="timer_api_test"
TARGET_BIN="$TARGET_BASE.bin"
TARGET_ELF="$TARGET_BASE.elf"
TARGET_LST="$TARGET_BASE.lst"
TARGET_MAP="$TARGET_BASE.map"

arm-eabi-gcc -o $TARGET_ELF timer_api_test.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
arm-eabi-nm -n $TARGET_ELF | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $TARGET_MAP

arm-eabi-objcopy -O binary $TARGET_ELF $TARGET_BIN
arm-eabi-objdump -D $TARGET_ELF > $TARGET_LST
