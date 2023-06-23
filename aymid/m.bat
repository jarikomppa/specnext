@echo off
set opath=%path%
set path=%path%;"\speccy\cc\z88dk20230622\bin\"
set ZCCCFG=/speccy/cc/z88dk20230622/lib/config
zcc +zxn -subtype=nex -vn -startup=1 -clib=sdcc_iy -m --list --c-code-in-asm @zproject.lst -o bin/aymid -create-app 
rem -SO3 --max-allocs-per-node2000000 --opt-code-speed -Cz"--clean"
copy bin\aymid.nex sync\test\
set path=%opath%