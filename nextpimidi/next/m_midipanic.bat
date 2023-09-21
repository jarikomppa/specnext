cls
del /q midipanic.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe midipanic.s --lst=midipanic.lst --sym=midipanic.sym --raw=midipanic.raw --syntax=abfw
dir midipanic.raw
copy /Y midipanic.raw ..\sync\dot\midipanic
