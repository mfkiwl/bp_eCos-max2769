#!/bin/sh -x

rm -rf error/  hal/  infra/ install/ io/ isoinfra/ kernel/ language/ services/ ecos.ecc makefile *.map

ecosconfig new bp2016_fpga
ecosconfig tree

make -j

TARGET_BASE="80M_test"
TARGET_BIN="$TARGET_BASE.bin"
TARGET_ELF="$TARGET_BASE.elf"
TARGET_LST="$TARGET_BASE.lst"

arm-eabi-gcc -o $TARGET_ELF 80M_test.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
arm-eabi-nm -n $TARGET_ELF | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > 80M_test_system.map

arm-eabi-objcopy -O binary $TARGET_ELF $TARGET_BIN
arm-eabi-objdump -D $TARGET_ELF > $TARGET_LST

