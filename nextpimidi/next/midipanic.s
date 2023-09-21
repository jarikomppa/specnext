    DEVICE ZXSPECTRUMNEXT
; nextpimidi
; nextpi midi test
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
;
; AppCtl:
;  0 -> 15: quit
; 10 -> 12: move to receive mode
; 10 ->  3: move to send mode
; 10 ->  6: move to idle mode

    STORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, 32 ; set pin 5 as write access
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, 0xff ; write all GPIO bits in 0x9A
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 1+2+4+8 ; set first four bits as write

    ; switch to "send midi" mode
    nextreg NEXTREG_PI_GPIO_3, 10
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 3


    ; Read current send counter
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a
    and 32
    ld (flipflop), a
    nextreg NEXTREG_PI_GPIO_0, a

    push hl ; enough stuff on stack for server failure rewind
    ld a, 'M'
    call write_gpio
    ld a, 'I'
    call write_gpio
    ld a, 'D'
    call write_gpio
    pop hl
; note off = 0x8x    
    ld l, 0
    ld c, 16
    ld b, 128
    ld a, 0x80
clearloop:
    push af    
    call write_gpio
    ld a, b
    dec a
    call write_gpio
    ld a, 0
    call write_gpio
    pop af
    djnz clearloop
    ld b, 128
    inc a
    dec c
    jr nz, clearloop

shutdown:
    ld hl, quitting
    call printmsg

cleanup:
    ; switch to "idle" mode
    nextreg NEXTREG_PI_GPIO_3, 10
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 6

    RESTORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    POPALL
    ret

server_failed:
    ld hl, noserver
    call printmsg
    jp cleanup

write_gpio:
    push af
    push bc    
    ld bc, 0
.checkloop:
    call check_gpio
    jr z, .ready
    djnz .checkloop
    dec c
    jr nz, .checkloop
    pop bc
    pop af
    pop hl ; rewind stack
    pop hl
    jp server_failed
.ready:
    pop bc
    pop af
    nextreg NEXTREG_PI_GPIO_2, a
    ld a, (flipflop)
    xor 32 ; flip bit
    ld (flipflop), a
    nextreg NEXTREG_PI_GPIO_0, a
    ret

; returns z if data is ready
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

noserver:
    db "The nextpi-usbmidi server does not seem to be running.\r",0
quitting:
    db "All done.\r",0

flipflop:
    db 32

store_NEXTREG_CPU_SPEED:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3:
    db 0
