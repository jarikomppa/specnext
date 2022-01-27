
keydata:
    BLOCK 9, 0

scaninput:
    ld bc, 0xfefe
    in a, (c)
    ld (keydata + 0), a
    ld bc, 0xfdfe
    in a, (c)
    ld (keydata + 1), a
    ld bc, 0xfbfe
    in a, (c)
    ld (keydata + 2), a
    ld bc, 0xf7fe
    in a, (c)
    ld (keydata + 3), a
    ld bc, 0xeffe
    in a, (c)
    ld (keydata + 4), a
    ld bc, 0xdffe
    in a, (c)
    ld (keydata + 5), a
    ld bc, 0xbffe
    in a, (c)
    ld (keydata + 6), a
    ld bc, 0x7ffe
    in a, (c)
    ld (keydata + 7), a
    ld bc, 31 ; kempston
    in a, (c) 
    xor 0x1f
    ld (keydata + 8), a
    ret

    MACRO ISKEYDOWN keybyte, keybit
        ld a, (keydata + keybyte)
        bit keybit, a
    ENDM

userinput:
    call scaninput
    ISKEYDOWN 7, 0 ; go to space. space. spaaaaceeeeee. to quit.
    jp z, fail
    ret

gamemode:
    jp userinput    