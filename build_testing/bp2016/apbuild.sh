#!/bin/sh -x

rm -rf error/  hal/  infra/ install/ io/ isoinfra/ kernel/ language/ services/ ecos.ecc makefile

ecosconfig new bp2016_fpga_ap
ecosconfig tree

make -j

GITVER=`git log --dense --pretty=oneline -n1 | head -c8`

TARGET_BASE="ap_two_thread-git-$GITVER"
TARGET_BIN="$TARGET_BASE.bin"
TARGET_ELF="$TARGET_BASE.elf"
TARGET_LST="$TARGET_BASE.lst"

arm-eabi-gcc -o $TARGET_ELF twothreads.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
arm-eabi-nm -n $TARGET_ELF | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > System.map

arm-eabi-objcopy -O binary $TARGET_ELF $TARGET_BIN
arm-eabi-objdump -D $TARGET_ELF > $TARGET_LST

