# eCos

## test eCos
1. config environment path
```
. batch_ecos_gcc4.7.3.sh
```

2. compile demo file
```
mkdir smdk2410
cd smdk2410/
ecosconfig new smdk2410
ecosconfig tree
make -j
cp ../ecos-3.0/examples/twothreads.c ./
arm-eabi-gcc -o a.out twothreads.c -I ./install/include/ -L ./install/lib/ -T target.ld -nostdlib
```
