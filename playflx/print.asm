doprintmsg:
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printmsg

printerrmsg:
    im 1
    ei
    call printmsg
    jp fail

printpagetemp:
    db 0
printmsg:
    push bc
    push af
    STORENEXTREG NEXTREG_MMU2, printpagetemp
    RESTORENEXTREG NEXTREG_MMU2, regstore + 1
    call doprintmsg
    RESTORENEXTREG NEXTREG_MMU2, printpagetemp
    pop af
    pop bc
    ret

hex:
    db "0123456789ABCDEF"
newline:
    db "\r", 0

printbyte:
    push af
    push bc
    push de
    push hl

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
    call printmsg

    pop hl
    pop de
    pop bc
    pop af
    ret

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
    