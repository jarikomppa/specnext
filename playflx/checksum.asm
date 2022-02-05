; output: de = checksums
calcchecksum:
    push bc
    push hl
    push af
    ld de, 0
    ld bc, 0x0600 ; b: 6 banks, c:bank offset = 0
.banks:
    push bc
    ld hl, rendertarget
    ld a, c
    add a, (hl)
    nextreg DSTMMU, a
    ld hl, DSTADDR ; mmu3 base address
    ld b, 32
.outer:
    push bc
    ld b, 0 ; 256 cycles
.inner:
    ; outer*inner=8192
    ld a, e
    xor (hl)
    ld e, a
    add a, d
    ld d, a

    inc hl
    djnz .inner

    pop bc
    djnz .outer

    pop bc
    inc c ; next bank
    djnz .banks
    pop af
    pop hl
    pop bc
    ret

dumpname:
    db "framedump.dat",0

writeout:
    PUSHALL
    ld hl, dumpname
    ld b, 0x2 + 0xc ; write, overwrite
    call fopen
    jp c, .fail    
    ld de, 0
    ld bc, 0x0600 ; b: 6 banks, c:bank offset = 0
.banks:
    push bc
    ld hl, rendertarget
    push af
    ld a, c
    add a, (hl)
    nextreg DSTMMU, a
    pop af
    ld hl, DSTADDR ; mmu3 base address
    ld bc, 8192
    push af
    call fwrite
    pop af
    pop bc
    inc c ; next bank
    djnz .banks
    call fclose
.fail:
    POPALL
    ret
