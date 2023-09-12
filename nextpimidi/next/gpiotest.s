    DEVICE ZXSPECTRUMNEXT
; gpiotest
; nextpi gpio test
; by Jari Komppa, http://iki.fi/sol
; 2023
;
    INCLUDE nextdefs.asm
; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     0x2000
    ld a, 1
    mirror a
    nop
    nop
    cp 0x80
    jp nz, notnext

    PUSHALL

    PRINT "GPIO test\r\0"

; specnext regs 0x90-0x93 - 32 bits
; dead 0-1 (internal to pi0)
; I2C 2-3 (-> RTC)
; SPI 7-11 (-> SD)
; UART 14-15
; Pins 28-31 do not exist in pi0 hardware
; 
; leaves..
; 
; 0x90------------------ 0x91------------------- 0x92------------------- 0x93------------------- (output enable)
; 0x98------------------ 0x99------------------- 0x9A------------------- 0x9B------------------- (i/o)
; 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
; -dead-I2C-          -SPI----------       -UART                                    -no hardware

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, 0 ; read all GPIO bits in 0x9A
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 2 ; set pin 1 as write access
    
    nextreg NEXTREG_PI_GPIO_3, 2 ; say we're good for more data

    ld d, 1 ; current flag

    ld bc, 0
loop:
    push bc

    ; Busy wait until data is available
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_3
    out (c), a
    inc b         ; nextreg i/o
wait_for_data:
    in a, (c)
    and 1
    cp d
    jr nz, wait_for_data

    ; read the data
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_2
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)

    and a, 0x07
    out (254), a ; blinky

    ; toggle bit
    ld a, 1
    sub d
    ld d, a

    add a,a

    ld (nextregop+3), a

    ; ready for more data
nextregop:
    nextreg NEXTREG_PI_GPIO_3, 0 ; overwritten above

    pop bc
    djnz loop
    dec c
    jr nz, loop

    PRINT "all done\r\0"
    POPALL
    ret

notnext:
    ld hl, notnextmsg
printmsg:
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printmsg

notnextmsg:
    db "This does not appear to be ZX Spectrum Next.",0
