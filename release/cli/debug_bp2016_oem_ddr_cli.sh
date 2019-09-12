#!/bin/sh -x

#ECOS_REPOSITORY=/home/huyue/ecos/ecos-3.0/packages ; export ECOS_REPOSITORY
ECOS_REPOSITORY=~/workplace/ecos/ecos-3.0/packages ; export ECOS_REPOSITORY
# eCos paths - do not modify this line, it is used by the installer
PATH=~/workplace/ecos/ecos-3.0/tools/bin:$PATH ; export PATH
# End eCos paths - do not modify this line, it is used by the installer
PATH=/opt/gcc/gnutools-4.7.3/arm-eabi/bin:$PATH ; export PATH

make -f Makefile clean
make debug BOARD="bp2016_asic_bp_ddr"
