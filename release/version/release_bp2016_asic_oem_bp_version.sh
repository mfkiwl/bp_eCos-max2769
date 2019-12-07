#!/bin/sh -x

ECOS_REPOSITORY=~/workplace/bp_eCos/ecos-3.0/packages ; export ECOS_REPOSITORY
PATH=~/workplace/bp_eCos/ecos-3.0/tools/bin:$PATH ; export PATH
PATH=/opt/gcc/gnutools-4.7.3/arm-eabi/bin:$PATH ; export PATH

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

