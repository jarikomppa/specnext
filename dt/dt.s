    DEVICE ZXSPECTRUMNEXT
; dt
; Delta time calculator
; by Jari Komppa, http://iki.fi/sol
; 2022
;

; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     0x2000
    ld a, 1
    mirror a
    nop
    nop
    cp 0x80
    jp nz, notnext

    ld a, h
    or l
    jp z, printhelp
    
    ld a, (hl)

    or "a"-"A" ; if it happens to be upper case, now it's lower

    sub "a" 
    jp c, printhelp
    cp "z"-"a"+1
    jp nc, printhelp
    ld (intvar), a        

    ld c, a ; intvar number
    ld b, 1 ; array
    ld hl, 0x0000 ; read array index 0
    exx                            ; place parameters in alternates
    ld      de, 0x01c9             ; IDE_INTEGER_VAR
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call    
    ld (starttime), de

    ld a, (intvar)
    ld c, a ; intvar number
    ld b, 1 ; array
    ld hl, 0x0001 ; read array index 1
    exx                            ; place parameters in alternates
    ld      de, 0x01c9             ; IDE_INTEGER_VAR
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call    
    ld (starttime+2), de

    ; Read end time from RTC
    ; de = time in ms-dos format
    ; h = seconds (ms-dos format has 2 sec accuracy)
    ; l = 100ths, not actually supported by hardware
    rst     0x8
    .db     0x8e ; M_GETDATE
    jp c, nortc
    ld (endtime), de
    ld l, h
    ld h, 0
    ld (endtime+2), hl

    ; convert ms-dos time to linear minutes
    ; (24*60 < 64k, so it'll fit in 16 bits)
    ld hl, (endtime)
    call timestamptomins
    ld (endtime), hl


    ld hl, (starttime + 2)
    ld e, l ; seconds
    ld hl, (endtime + 2)
    ld a, l
    
    ld bc, (starttime) ; minutes
    ld hl, (endtime)

    or a
    sbc e
    jr nc, .minutes
    add a, 60
.minutes:    
    sbc hl, bc
    jr nc, .daycycle
    ld bc, 24*60
    add hl, bc
.daycycle:    
    ld c, a

    ; (if you try to measure over 24 hours, it's your problem)

    ; now hl:c has minutes:seconds

    ld a, '0'-1
    ld de, 10000
    or a
.l10k:
    inc a
    sbc hl, de
    jr nc, .l10k
    add hl, de
    cp a, "0"
    jr z, .skipzero10k
    rst 16
.skipzero10k:

    ld a, '0'-1
    ld de, 1000
    or a
.l1k:
    inc a
    sbc hl, de
    jr nc, .l1k
    add hl, de
    cp a, "0"
    jr z, .skipzero1k
    rst 16
.skipzero1k:

    ld a, '0'-1
    ld de, 100
    or a
.l100:
    inc a
    sbc hl, de
    jr nc, .l100
    add hl, de
    cp a, "0"
    jr z, .skipzero100
    rst 16
.skipzero100:

    ld a, '0'-1
    ld de, 10
    or a
.l10:
    inc a
    sbc hl, de
    jr nc, .l10
    add hl, de
    cp a, "0"
    jr z, .skipzero10
    rst 16
.skipzero10:

    ld a, '0'-1
    ld de, 1
    or a
.l1:
    inc a
    sbc hl, de
    jr nc, .l1
    add hl, de
    rst 16

    ld a, ':'
    rst 16

    ld h, 0
    ld l, c

    ld a, '0'-1
    ld de, 10
    or a
.s10:
    inc a
    sbc hl, de
    jr nc, .s10
    add hl, de
    rst 16

    ld a, '0'-1
    ld de, 1
    or a
.s1:
    inc a
    sbc hl, de
    jr nc, .s1
    add hl, de
    rst 16

    ld a, "\r"
    rst 16
    ld a, "\r"
    rst 16
    
    ld de, (endtime)
    ld c, a ; intvar number
    ld b, 1 ; array
    ld hl, 0x0100 ; read array index 0
    exx                            ; place parameters in alternates
    ld      de, 0x01c9             ; IDE_INTEGER_VAR
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call    

    ld de, (endtime+2)
    ld a, (intvar)
    ld c, a ; intvar number
    ld b, 1 ; array
    ld hl, 0x0101 ; read array index 1
    exx                            ; place parameters in alternates
    ld      de, 0x01c9             ; IDE_INTEGER_VAR
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call    

    or a
    ret

; ms-dos timestamp format. Let's ignore day. We're only a dot.
; 15-11 Hours (0-23)
; 10-5 	Minutes (0-59)
; 4-0 	Seconds/2 (0-29) 
;
; I'm pretty sure large parts of this could be done with z80n mul..
;
; in/out hl
timestamptomins:
    ; hhhhhmmm mmmsssss
    
    ld a, h
    rrca ; mhhhhhmm
    rrca ; mmhhhhhm
    rrca ; mmmhhhhh
    and 0x1f
    ld b, a ; hours
    ld a, h
    rlca ; hhhhmmmh
    rlca ; hhhmmmhh
    rlca ; hhmmmhhh
    and 0x38 ; 0b0011 1000
    ld c, a
    ld a, l
    rlca ; mmsssssm
    rlca ; msssssmm
    rlca ; sssssmmm
    and 0x07
    add c
    ld h, 0
    ld l, a ; minutes
    ld a, b ; hours
    or a
    jr z, .nohours
    ld de, 60
.loop:   
    add hl, de
    djnz .loop
.nohours:
    ret

nortc:
    ld hl, rtcfailmsg
    jp printloop

printhelp:
    ld hl, helptext
printloop:
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printloop

notnext:
    ld hl, notnextmsg
    jp printloop

helptext:
       ;12345678901234567890123456789012
    db "DT v0.1 by Jari Komppa\r"
    db "http://iki.fi/sol\r"
    db "Time delta calculator\r\r"
    db "SYNOPSIS:\r"
    db ".DT INTVAR\r\r"
    db "where INTVAR is one letter\r"
    db "eg, DT X will use %X(n)\r"
    db "x(0)=minutes, x(1)=seconds\r\r"
    db 0

rtcfailmsg:
    db "RTC read failed.\r",0

notnextmsg:
    db "This does not appear to be ZX Spectrum Next.",0

starttime:
    dw 0, 0

endtime:
    dw 0, 0

intvar:
    db 0
