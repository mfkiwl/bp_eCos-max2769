#!/bin/sh -x

if [ $# -gt 0 ]
then
	TH_NAME=$1
	if [ "$1" = "help" ]
	then
		echo "$0 1 //compile threads_sched.c" 
		echo "$0 2 //compile threads_slice.c" 
		echo "$0 3 //compile threads_slice2.c" 
		echo "$0 4 //compile threads_semaphore.c" 
		echo "$0 5 //compile kmutex1.c for test condition relative functions " 
		echo "$0 6 //compile kflag1.c for test flag relative functions " 
		echo "$0 7 //compile kmbox1.c for test message box relative functions " 
		exit 0
	fi
else
	TH_NAME=0
fi

rm -rf error/  hal/  infra/ install/ io/ isoinfra/ kernel/ language/ services/ ecos.ecc makefile

ecosconfig new bp2016_fpga_bp
ecosconfig tree

make -j

GITVER=`git log --dense --pretty=oneline -n1 | head -c8`

if [ $TH_NAME -eq 1 ]
then
TARGET_BASE="bp_threads_sched-git-$GITVER"
elif [ $TH_NAME -eq 2 ]
then
TARGET_BASE="bp_threads_slice-git-$GITVER"
elif [ $TH_NAME -eq 3 ]
then
TARGET_BASE="bp_threads_slice2-git-$GITVER"
elif [ $TH_NAME -eq 4 ]
then
TARGET_BASE="bp_threads_semaphore-git-$GITVER"
elif [ $TH_NAME -eq 5 ]
then
TARGET_BASE="bp_threads_cond-git-$GITVER"
elif [ $TH_NAME -eq 6 ]
then
TARGET_BASE="bp_threads_flag-git-$GITVER"
elif [ $TH_NAME -eq 7 ]
then
TARGET_BASE="bp_threads_mbox-git-$GITVER"
else
TARGET_BASE="bp_two_thread-git-$GITVER"
fi

TARGET_BIN="$TARGET_BASE.bin"
TARGET_ELF="$TARGET_BASE.elf"
TARGET_LST="$TARGET_BASE.lst"

if [ $TH_NAME -eq 1 ]
then
arm-eabi-gcc -o $TARGET_ELF threads_sched.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 2 ]
then
arm-eabi-gcc -o $TARGET_ELF threads_slice.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 3 ]
then
arm-eabi-gcc -o $TARGET_ELF threads_slice2.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 4 ]
then
arm-eabi-gcc -o $TARGET_ELF threads_semaphore.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 5 ]
then
arm-eabi-gcc -o $TARGET_ELF kmutex1.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 6 ]
then
arm-eabi-gcc -o $TARGET_ELF kflag1.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
elif [ $TH_NAME -eq 7 ]
then
arm-eabi-gcc -o $TARGET_ELF kmbox1.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
else
arm-eabi-gcc -o $TARGET_ELF twothreads.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
fi

arm-eabi-nm -nsSC $TARGET_ELF  > System.map

arm-eabi-objcopy -O binary $TARGET_ELF $TARGET_BIN
arm-eabi-objdump -D $TARGET_ELF > $TARGET_LST
