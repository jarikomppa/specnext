cls
del /q playflx.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe dt.s --lst=dt.lst --sym=dt.sym --raw=dt.raw --syntax=abfw
dir dt.raw
copy /Y dt.raw sync\dot\dt
