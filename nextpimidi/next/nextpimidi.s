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

    STORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3
    STORENEXTREG NEXTREG_PERIPHERAL3, store_NEXTREG_PERIPHERAL3

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, 32+64 ; set pin 5 & 6 as write access
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, 0 ; read all GPIO bits in 0x9A
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 1+2+4+8 ; set first four bits as write

    ; Read current send counter
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a
    and 32
    ; toggle bit
    xor 32
    ld (flipflop), a
    ld (nextreg_pos_init+3), a
nextreg_pos_init:    
    nextreg NEXTREG_PI_GPIO_0, 0 ; say we're good for more data (value overwritten above)

    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PERIPHERAL3
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    or 2
    out (c), a      ; enable turbosound

    ; Play silent note on all 9 channels
    ld hl, 0
    ld a, 0
    ld b, 9
clearchannels:
    call playay
    inc a
    djnz clearchannels

    ld a, 1
    ld h, 7
    ld l, 0x38 ; enable voices 1-3, disable noise 1-3
    call setchip
    call setay
    inc a     ; same for chip 2..
    call setchip
    call setay
    inc a    ; and chip 3
    call setchip
    call setay

    
    call read_gpio_timeout
    cp 'I'
    jr z, alreadyati
    cp 'M'
    jp nz, server_failed
    call read_gpio_timeout
    cp 'I'
    jp nz, server_failed
alreadyati:    
    call read_gpio_timeout
    cp 'D'
    jp nz, server_failed

    ld hl, initialized
    call printmsg

forever:
    ld bc, 0x7ffe
    in a, (c) ; check for space
    and 1
    jp z, shutdown
    call check_gpio
    jr nz forever

    call read_gpio
    and 0xf0        ; we don't care about midi channel, just react to everything
    ld b, a
    call read_gpio
    ld h, a
    call read_gpio
    ld l, a
    ld a, b
    cp 0x80
    jr nz, notnoteoff
    call stopnote    
    jp forever
notnoteoff:    
    cp 0x90
    jr nz, notnoteon
    srl l
    srl l
    ld a, l
    cp 15
    jr c, under15
    ld a, 15
under15:    
    ld l, a
    call playnote ; h = note, l = (volume/4)<15?(volume/4):15 (so 0..50% volume ramp up and max vol after that)
    jp forever
notnoteon:
    jp forever

shutdown:
    ld hl, quitting
    call printmsg

cleanup:
    call kill_server
    RESTORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3
    RESTORENEXTREG NEXTREG_PERIPHERAL3, store_NEXTREG_PERIPHERAL3

    POPALL
    ret

server_failed:
    ld hl, noserver
    call printmsg
    jp cleanup

; return value in 'a'. If time out occurs, returns 0.
read_gpio_timeout:
    push hl
    ld hl, 0
    xor a
tryloop:
    call check_gpio
    jr z, gotdata
    dec hl
    cp h
    jr nz, tryloop
    cp l
    jr nz, tryloop
    pop hl
    ret
gotdata:
    pop hl
    jp read_gpio    


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

; returns data byte in 'a'; waits forever for data
read_gpio:
    push bc
    push de

    ld a, (flipflop) ; current flag
    ld d, a

    ; Busy wait until data is available
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
wait_for_data:
    in a, (c)
    sla a
    and 32
    cp d
    jr nz, wait_for_data

    ; read the data
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_2
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    ld e, a

    ld a, d
    ; toggle bit
    xor 32

    ld (nextreg_pos_readgpio+3), a ; ready for more data
nextreg_pos_readgpio:
    nextreg NEXTREG_PI_GPIO_0, 0 ; overwritten above

    ld (flipflop), a
    ld a, e

    pop de
    pop bc
    ret

; setay: reg h, value l
setay:
    push bc
    ld bc, 0xfffd
    out (c), h
    ld bc, 0xbffd
    out (c), l
    pop bc
    ret

; setchip: a = active chip
setchip:
    push af
    push bc
    or 0xfc
    ld bc, 0xfffd
    out (c), a
    pop bc
    pop af
    ret

; playay a = channel, h = note, l = volume
playay:
    push af
    push bc
    push de
    push hl
    ld c, 1 ; chip number
chipcalc:    
    cp 3
    jr c, chipdone
    inc c
    sub 3
    jr chipcalc
chipdone:
    ld b, a ; channel in chip
    ld a, c
    call setchip
    ld a, b
    add a, 8
    push hl
    ld h, a ; h is now channel + 8
    ld a, l
    and 15
    ld l, a
    call setay ; volume reg in h (8 + channel), value in l
    pop de
    ld e, d
    ld d, 0
    push de ; both stack top and de are now just note
    ld hl, note_fine
    add hl, de
    ld a, b
    add a
    ld b, a ; b = 2*channel
    ld a, (hl)
    ld h, b ; h is now 2*channel
    ld l, a ; l is now fine value for note
    call setay ; set fine 
    pop de
    ld hl, note_coarse
    add hl, de
    ld a, (hl)
    ld h, b
    inc h   ; h is now 2*channel+1
    ld l, a ; l is note value for coarse
    call setay
    pop hl
    pop de
    pop bc
    pop af
    ret

; playnote: h = note, l = volume
playnote: 
    push af
    push bc
    push de
    push hl
    ld a, (nextch)
    call playay
    ld d, h ; d = note
    ld c, a ; bc = channel
    ld b, 0
    ld hl, chnote
    add hl, bc
    ld (hl), d ; store note to channel array (for noteoff)
    pop hl
    pop de
    pop bc
    ; increment nextch for next playnote
    ld a, (nextch)
    inc a
    cp 9
    jr nz, playnotedone
    xor a ; nextch was 9, roll over to 0
playnotedone:
    ld (nextch), a 
    pop af
    ret

; stopnote: h = note
stopnote:
    push af
    push bc
    push de
    push hl
    ld a, h ; a = note
    ld b, 9
    ld c, 0
    ld hl, chnote
stopnoteloop:    ; find channel for the note
    cp (hl)
    jr z, stopnotefound
    inc c
    inc hl
    djnz stopnoteloop
    pop hl
    pop de
    pop bc
    pop af
    ret         ; didn't find it, must have been clobbered
stopnotefound:
    ; found, c = channel (0-8), (hl) = position in chnote
    ld (hl), 255
    ld a, c
    ld h, 0
    ld l, 0
    call playay ; play silent note on the channel
    pop hl
    pop de
    pop bc
    pop af
    ret

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

initialized:
    db "Running. Press space to quit.\r",0

quitting:
    db "Shutting down..\r",0

nextch:
    db 0

flipflop:
    db 32

note_coarse:
    db 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
note_fine:
    db 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192, 222, 8, 63, 128, 205, 35, 131, 236, 93, 215, 88, 224, 111, 4, 159, 64, 230, 145, 65, 246, 174, 107, 44, 240, 183, 130, 79, 32, 243, 200, 160, 123, 87, 53, 22, 248, 219, 193, 167, 144, 121, 100, 80, 61, 43, 26, 11, 252, 237, 224, 211, 200, 188, 178, 168, 158, 149, 141, 133, 126, 118, 112, 105, 100, 94, 89, 84, 79, 74, 70, 66, 63, 59, 56, 52, 50, 47, 44, 42, 39, 37, 35, 33, 31, 29, 28, 26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8

chnote:
    db 255,255,255, 255,255,255, 255,255,255

store_NEXTREG_CPU_SPEED:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3:
    db 0
store_NEXTREG_PERIPHERAL3:
    db 0
