COBJ_FILES = src/*.c

#GITVER=`git log --dense --pretty=oneline -n1 | head -c8`
#TARGET_BASE="cli_release-git-$(GITVER)"
TARGET_BASE="cli_release-git-01"
TARGET_BIN="$(TARGET_BASE).bin"
TARGET_ELF="$(TARGET_BASE).elf"
TARGET_LST="$(TARGET_BASE).lst"
TARGET_MAP="$(TARGET_BASE).map"

DEBUG_TARGET_BASE="cli_debug"
DEBUG_TARGET_BIN="$(DEBUG_TARGET_BASE).bin"
DEBUG_TARGET_ELF="$(DEBUG_TARGET_BASE).elf"
DEBUG_TARGET_LST="$(DEBUG_TARGET_BASE).lst"
DEBUG_TARGET_MAP="$(DEBUG_TARGET_BASE).map"

export COBJ_FILES

debug:
	@echo 'this is for debug cli'
	rm -rf devs/ error/  hal/  infra/ io/ install/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile
	#ecosconfig new bp2016_fpga_bp
	ecosconfig new ${BOARD}
	ecosconfig tree
	make -j
	@echo "------------------$(COBJ_FILES)"
	arm-eabi-gcc -march=armv7-a -Wall -Werror -g -o $(DEBUG_TARGET_ELF) $(COBJ_FILES) -I ./install/include/ -I ./include/ -L ./install/lib/ -T target.ld -nostdlib
	arm-eabi-nm -nsSC $(DEBUG_TARGET_ELF)  > $(DEBUG_TARGET_MAP)
	arm-eabi-objcopy -O binary $(DEBUG_TARGET_ELF) $(DEBUG_TARGET_BIN)
	arm-eabi-objdump -D $(DEBUG_TARGET_ELF) > $(DEBUG_TARGET_LST)
	rm -rf devs/ error/  hal/  infra/ io/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile


release:
	@echo 'this is for release cli'
	rm -rf devs/ error/  hal/  infra/ io/ install/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile
	arm-eabi-gcc -march=armv7-a -g -o $(TARGET_ELF) $(COBJ_FILES) -I ../install/include/ -I ./include/ -L ../install/lib/ -T target.ld -nostdlib
	arm-eabi-nm -n $(TARGET_ELF) | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $(TARGET_MAP)
	arm-eabi-objcopy -O binary $(TARGET_ELF) $(TARGET_BIN)
	arm-eabi-objdump -D $(TARGET_ELF) > $(TARGET_LST)
	rm -rf devs/ error/  hal/  infra/ io/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile


.PHONY: clean
clean:
	rm -rf devs/ error/  hal/  infra/ io/ install/ isoinfra/ kernel/ language/ services/ compat/ fs/ ecos.ecc makefile
	rm -rf *.elf *.bin *.lst *.hex *.map config.mk
