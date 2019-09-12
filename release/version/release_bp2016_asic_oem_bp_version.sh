#!/bin/sh -x

if [ $# -gt 0 ]
then
RELEASE_MAJOR_VERSION=$1
else
RELEASE_MAJOR_VERSION=0
fi
# echo "{$RELEASE_MAJOR_VERSION}"
export RELEASE_MAJOR_VERSION

if [ $# -gt 1 ]
then
RELEASE_MINOR_VERSION=$2
else
RELEASE_MINOR_VERSION=5
fi
# echo "{$RELEASE_MINOR_VERSION}"
export RELEASE_MINOR_VERSION

RELEASE_SUPPORT_BOARD=\"bp2016_oem_board-SRAM\"
export RELEASE_SUPPORT_BOARD

rm -rf devs/ error/  hal/  infra/ io/ install/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile
rm -rf *.elf *.bin *.lst *.hex *.map config.mk

ecosconfig new bp2016_asic_bp
ecosconfig tree
make -j

