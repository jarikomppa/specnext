    DEVICE ZXSPECTRUMNEXT
; killserver
; nextpi gpio server killer dot
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

    PRINT "killing nextpi server..\r\0"

    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 1+2+4+8 ; set first four bits as write

    call kill_server

    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    PRINT "all done\r\0"
    POPALL
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

store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3:
    db 0

