#!/bin/sh -x

make -f Makefile clean
make debug BOARD="bp2016_asic_bp"
