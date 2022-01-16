cls
del /q playflx.raw
\speccy\ext\sjasmplus-1.18.2.win\sjasmplus.exe playflx.s --lst=playflx.lst --sym=playflx.sym --raw=playflx.raw --syntax=abfw
dir playflx.raw
copy /Y playflx.raw sync\dot\playflx
