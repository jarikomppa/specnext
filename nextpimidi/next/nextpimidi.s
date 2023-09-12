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

    ; Read current send counter
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_3
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    and 1
    ; toggle bit
    ld d, a
    ld a, 1
    sub d    
    ld (flipflop), a
    add a
    ld (nextreg_pos_init+3), a
nextreg_pos_init:    
    nextreg NEXTREG_PI_GPIO_3, 0 ; say we're good for more data

    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PERIPHERAL3
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    or 2
    out (c), a      ; enable turbosound

;; ok

    ld hl, 0
    ld a,0
    ld b, 9
clearchannels:
    call playay
    inc a
    djnz clearchannels


;;; kosh

    ld a, 1
    ld h, 7
    ld l, 0x38 // enable voices 1-3, disable noise 1-3
    call setchip
    call setay
    inc a
    call setchip
    call setay
    inc a
    call setchip
    call setay

forever:
    PRINT "."
    call read_gpio
    and 0xf0
    ld b, a
    call read_gpio
    ld h, a
    call read_gpio
    ld l, a
    ld a, b
    cp 0x80
    jr nz, notnoteoff
    PRINT "noteoff"
    call stopnote    
    jp forever
notnoteoff:    
    cp 0x90
    jr nz, notnoteon
    PRINT "noteon"
;    srl l
;    srl l
;    ld a, l
;    cp 15
;    jr nc, under15
    ld a, 15
;under15:    
    ld l, a
    call playnote
    jp forever
notnoteon:
    jp forever


    POPALL
    ret


; returns data byte in 'a'; waits forever for data
read_gpio:
    push bc
    push de


    ld a, (flipflop) ; current flag
    ld d, a

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

    ld e, a

    ; toggle bit
    ld a, 1
    sub d
    ld d, a

    add a,a

    ld (nextreg_pos_readgpio+3), a
    ; ready for more data
nextreg_pos_readgpio:
    nextreg NEXTREG_PI_GPIO_3, 0 ; overwritten above

    ld a, d
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
    ld h, a
    ld a, l
    and 15
    ld l, a
    call setay ; volume in h, l
    pop de
    ld e, 0
    push de
    ld hl, note_fine
    add hl, de
    ld a, b
    add a
    ld b, a
    ld a, (hl)
    ld h, b
    ld l, a
    call setay ; set fine 
    pop de
    ld hl, note_coarse
    add hl, de
    ld a, (hl)
    ld h, b
    inc h
    ld l, a
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
    ld d, h
    ld b, a
    ld c, 0
    ld hl, chnote
    add hl, bc
    ld (hl), d
    pop hl
    pop de
    pop bc
    ld a, (nextch)
    inc a
    ld (nextch), a
    cp 9
    pop af
    ret nz
    push af
    xor a
    ld (nextch), a
    pop af
    ret

; stopnote: h = note
stopnote:
    push af
    push bc
    push de
    push hl
    ld a, h
    ld b, 9
    ld c, 0
    ld hl, chnote
stopnoteloop:    
    cp (hl)
    jr z, stopnotefound
    inc c
    inc hl
    djnz stopnoteloop
    pop hl
    pop de
    pop bc
    pop af
    ret
stopnotefound:
    ld (hl), 255
    ld a, c
    ld h, 0
    ld l, 0
    call playay
    pop hl
    pop de
    pop bc
    pop af
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

nextch:
    db 0

flipflop:
    db 1

note_coarse:
    db 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
note_fine:
    db 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192, 222, 8, 63, 128, 205, 35, 131, 236, 93, 215, 88, 224, 111, 4, 159, 64, 230, 145, 65, 246, 174, 107, 44, 240, 183, 130, 79, 32, 243, 200, 160, 123, 87, 53, 22, 248, 219, 193, 167, 144, 121, 100, 80, 61, 43, 26, 11, 252, 237, 224, 211, 200, 188, 178, 168, 158, 149, 141, 133, 126, 118, 112, 105, 100, 94, 89, 84, 79, 74, 70, 66, 63, 59, 56, 52, 50, 47, 44, 42, 39, 37, 35, 33, 31, 29, 28, 26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8

chnote:
    db 255,255,255, 255,255,255, 255,255,255