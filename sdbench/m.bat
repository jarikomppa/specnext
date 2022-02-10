cls
del /q sdbench.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe sdbench.s --lst=sdbench.lst --sym=sdbench.sym --raw=sdbench.raw --syntax=abfw
dir sdbench.raw
copy /Y sdbench.raw sync\dot\sdbench
