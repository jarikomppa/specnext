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

useranimationstop:
    jp fail
    
userinput:
    call scaninput
    ld a, (keyfree)
    or 0
    jr z, checkfornoinput
    call anykey
    jp nz, useranimationstop
    ret

checkfornoinput:
    call anykey
    ret nz ; key is still down (from before startup)
    ld a, 1
    ld (keyfree), a
    ret

; Game mode: various user inputs
; In : input scheme, allowed input mask
; Out: input received, frame number
gamemode:
    call scaninput
    ld a, (keyfree)
    or 0
    jr z, checkfornoinput
notdown0:
    ld a, (keydata + 2) ; 2, 0 = Q
    bit 0, a
    jr nz, notdown1
    ld a, 1
    jp gamekeydown
notdown1:    
    ld a, (keydata + 1) ; 1, 0 = A
    bit 0, a
    jr nz, notdown2
    ld a, 2
    jp gamekeydown
notdown2:
    ld a, (keydata + 5) ; 5, 1 = O
    bit 1, a
    jr nz, notdown3
    ld a, 3
    jp gamekeydown
notdown3:
    ld a, (keydata + 5) ; 5, 0 = P
    bit 0, a
    jr nz, notdown4
    ld a, 4
    jp gamekeydown
notdown4:
    ld a, (keydata + 7) ; 7, 0 = space
    bit 0, a
    jr nz, notdown5
    ld a, 5
    jp gamekeydown
notdown5:
    ret
    jp userinput

gamekeydown:
    ld bc, 0x0106 ; array G
    ld hl, 0x0102 ; write index 2
    ld d, 0
    ld e, a
    call intvar
    ld bc, 0x0106 ; array G
    ld hl, 0x0103 ; write index 3
    ld de, (currentframe)
    call intvar

    jp useranimationstop


keyfree:
    db 0

