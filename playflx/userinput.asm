    MACRO ISKEYDOWN keybyte, keybit
        ld a, (keydata + keybyte)
        bit keybit, a
    ENDM

keydata:
    BLOCK 9, 0

scaninput:
    ld hl, keydata
    ld c, 0xfe
    ld b, c       ; [0]
    ini ; hl = in (c), hl++, b--
    ld b, 0xfd    ; [1]
    ini
    ld b, 0xfb    ; [2]
    ini
    ld b, 0xf7    ; [3]
    ini
    ld b, 0xef    ; [4]
    ini
    ld b, 0xdf    ; [5]
    ini
    ld b, 0xbf    ; [6]
    ini
    ld b, 0x7f    ; [7]
    ini
    ld bc, 31     ; kempston
    in a, (c) 
    xor 0x1f      ; kempston is active-high, let's make it uniform with others
    ld (hl), a    ; [8]
    ret

anykey:
    ld hl, keydata
    ld b, 8 ; don't check for kempston as unconnected joystick may be random
.loop
    ld a, (hl)
    and 0x1f
    cp 0x1f
    ret nz
    inc hl
    djnz .loop
    ret

userinput:
    call scaninput
    ld a, (keyfree)
    or 0
    jp z, .checkfornoinput
    call anykey
    jp nz, fail
    ret
.checkfornoinput:
    call anykey
    ret nz ; key is still down (from before startup)
    ld a, 1
    ld (keyfree), a
    ret

; Game mode: various user inputs
; In : input scheme, allowed input mask
; Out: input received, frame number
gamemode:
    jp userinput

keyfree:
    db 0