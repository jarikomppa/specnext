printmsg:
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printmsg

hex:
    db "0123456789ABCDEF"
newline:
    db "\r", 0

;; ab 00 ab -> ba b0 0a
printbyte:
    ld b, a
    and 0xf
    ld c, a
    ld a, b
    .4 rrca
    and 0xf
    ld b, a
    ; b and c contain nibbles now
    ld hl, hex
    ld d, 0
    ld e, b
    add hl, de
    ld a, (hl)
    ld hl, SCRATCH
    ld (hl), a
    ld hl, hex
    ld e, c
    add hl, de
    ld a, (hl)
    ld hl, SCRATCH+1
    ld (hl), a
    ld a, 0
    inc hl
    ld (hl), a
    ld hl, SCRATCH
    jp printmsg

printword:
    push hl
    ld a, h
    call printbyte
    pop hl
    ld a, l
    jp printbyte

printnewline:
    ld hl, newline
    jp printmsg
    