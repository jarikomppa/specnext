cls
del /q nextpimidi.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe nextpimidi.s --lst=nextpimidi.lst --sym=nextpimidi.sym --raw=nextpimidi.raw --syntax=abfw
dir nextpimidi.raw
copy /Y nextpimidi.raw ..\sync\dot\nextpimidi
