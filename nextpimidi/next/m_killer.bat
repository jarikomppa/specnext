cls
del /q killserver.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe killserver.s --lst=killserver.lst --sym=killserver.sym --raw=killserver.raw --syntax=abfw
dir killserver.raw
copy /Y killserver.raw ..\sync\dot\killserver
