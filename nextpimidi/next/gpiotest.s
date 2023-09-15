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

; Nextpi GPIO
; ===========
; - specnext regs 0x90-0x93 - 32 bits
; - Pins 0-1 are dead (internal to pi0)
; - Pins 2-3 I2C (-> RTC, probably doesn't work?)
; - Pins 7-11 SPI (-> SD, very likely doesn't work)
; - Pins 14-15 UART (-> term, pisend, etc)
; - Pins 18-21 I2S (-> audio, works and is used)
; - Pins 28-31 do not exist in pi0 hardware
;
; - Pins 24-27 reserved for app control
; 
; leaves..
; 
; 0x90------------------ 0x91------------------- 0x92------------------- 0x93------------------- (output enable)
; 0x98------------------ 0x99------------------- 0x9A------------------- 0x9B------------------- (i/o)
; 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
; XX XX XX XX          XX XX XX XX XX       XX XX       XX XX XX XX                   XX XX XX XX
; -dead-I2C-          -SPI-----------       -UART       -I2S-------       -AppCtl---- -no hardware
;             AA BB CC                            DD DD DD DD DD DD DD DD EE FF GG HH
;            
; A = pi-side 1 bit data counter
; B = next-side 1 bit data counter
; C = transfer direction, 0 = pi->next, 1 = next->pi. Controlled by next.
; D = 8 bits of data (overlaps with I2S, so no sound while this is going)
; E,F,G,H = App control. If a pattern of 0 0 0 0 ->  1 1 1 1 is seen, 
;           service should clean up after itself and die.

    STORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, 32+64 ; set pin 5 & 6 as write access (pin 4 read)
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, 0 ; read all GPIO bits in 0x9A
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 1+2+4+8 ; set first four bits as write
    
    ; Read current send counter
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a         ; server writes to pin 4 = 16, *2 = 32
    and 32        ; mask off any garbage
    xor 32 ; toggle bit
    ld d, a
    ld (nextregopstart+3), a    ; ready for more data    
nextregopstart:
    nextreg NEXTREG_PI_GPIO_0, 0 ; value overwritten above

    ld (flipflop), a

    halt ; a little delay
    halt
    call check_gpio
    jp nz, noserver

    ld bc, 0
    ld hl, 0
loop:
    push bc

    ; Busy wait until data is available
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
wait_for_data:
    in a, (c)
    sla a    ; server writes to pin 4, so 0 or 16. *2 = 0 or 32
    and 32   ; make sure there's no garbage bits
    cp d     ; does this match our current state?
    jr nz, wait_for_data

    ; read the data
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_2
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)

    ;;;; payload
    and a, 0x07
    out (254), a ; blinky
    ;;;; /payload

    ; toggle bit
    ld a, d
    xor 32
    ld d, a

    ld (nextregop+3), a    ; ready for more data
nextregop:
    nextreg NEXTREG_PI_GPIO_0, 0 ; value overwritten above

    pop bc
    djnz loop
    dec c
    jr nz, loop

cleanup:
;    call kill_server

    RESTORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    PRINT "all done\r\0"
    POPALL
    ret

noserver:
    PRINT "GPIO server not responding.\r\0"
    jp cleanup

    ; Transition from 0000 -> 1111 kills the server. Try a couple times with some delay.
kill_server:
    nextreg NEXTREG_PI_GPIO_3, 0
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 15
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 0
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 15
    ret

; returns z if data is ready for reading
check_gpio:
    push bc
    push de
    ld e, a
    ld a, (flipflop) ; current flag
    ld d, a
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a
    and 32
    cp d
    ld a, e
    pop de
    pop bc
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

store_NEXTREG_CPU_SPEED:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3:
    db 0
flipflop:
    db 0
