ECOS_GLOBAL_CFLAGS = -Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Woverloaded-virtual -Wno-write-strings -mno-thumb-interwork -Wstrict-prototypes -Wno-comment -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon -g -Os -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fvtable-gc -ffreestanding
ECOS_GLOBAL_LDFLAGS = -mno-thumb-interwork -mcpu=cortex-a7 -static -n -g -nostdlib
ECOS_COMMAND_PREFIX = arm-eabi-
