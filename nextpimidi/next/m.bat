cls
del /q gpiotest.raw
\speccy\ext\sjasmplus-1.18.3.win\sjasmplus.exe gpiotest.s --lst=gpiotest.lst --sym=gpiotest.sym --raw=gpiotest.raw --syntax=abfw
dir gpiotest.raw
copy /Y gpiotest.raw ..\sync\dot\gpiotest
