cls
del /q midisend.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe midisend.s --lst=midisend.lst --sym=midisend.sym --raw=midisend.raw --syntax=abfw
dir midisend.raw
copy /Y midisend.raw ..\sync\dot\midisend
