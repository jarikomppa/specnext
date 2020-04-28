@echo off
set PATHTEMP=%PATH%
set PATH=/speccy/cc/z88dk200/bin;%PATH%
set ZCCCFG=/speccy/cc/z88dk200/lib/config
rem zcc +zxn -v -startup=30 -clib=sdcc_iy -O3 -SO3 --opt-code-size --max-allocs-per-node200000 test.c -o test -subtype=dotn -create-app -pragma-define:CLIB_OPT_SCANF=0 -pragma-define:CLIB_OPT_PRINTF=0x5605
zcc +zxn -v -startup=4 -clib=sdcc_iy hexdump.c -o hexdump -subtype=dotn -create-app 
set PATH=%PATHTEMP%
