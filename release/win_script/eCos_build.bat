echo off
set pa_old=%cd%
cd ..

gcc\gcc-arm-none-eabi-5_2-2015q4-20151219-win32\bin\arm-none-eabi-gcc.exe -o win_script\eCos_testing.elf  src\main.c -I "%cd%\install\\include" -L "%cd%\install\lib" -T "%cd%\install\lib\target.ld" -nostdlib

gcc\gcc-arm-none-eabi-5_2-2015q4-20151219-win32\bin\arm-none-eabi-objdump -D win_script\eCos_testing.elf > win_script\eCos_testing.lst

gcc\gcc-arm-none-eabi-5_2-2015q4-20151219-win32\bin\arm-none-eabi-objcopy.exe -O binary win_script\eCos_testing.elf win_script\eCos_testing.bin


cd %pa_old%





