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

rm -rf error/  hal/  infra/ install/ io/ isoinfra/ kernel/ language/ services/ ecos.ecc makefile
ecosconfig new bp2016_fpga_bp
ecosconfig tree
make -j

