; out: a
readbyte:
    push hl
    ld hl, (fileindex) ; if 0xa200, the buffer is exhausted and needs to read next block
    bit 1, h
    res 1, h ; resets 0xa200 -> 0xa000
    call nz, nextfileblock
    ld a, (hl)
    inc hl
    ld (fileindex), hl
    pop hl
    ret

; out: hl
readword:
    call readbyte
    ld l, a
    call readbyte
    ld h, a
    ret

; de = buf
; bc = bytes
read:
    push de
    push bc
    ld hl, 512 + FILEBUF
    ld bc, (fileindex)
    or a
    sbc hl, bc
    call z, nextfileblock_hl512
    ; hl = max bytes to read at once
    pop bc  ; desired copy length
    push bc
    push hl
    or a
    sbc hl, bc
    jr nc, .oklen
    pop bc   ; max len
    push bc
.oklen:
    pop hl ; throw-away max len
    pop hl ; original copy length
    pop de ; destination address
    ; now de = dest, bc = byte count, hl = original byte count
    push hl
    push bc
    push de
    ld hl, (fileindex)
    ;ldir ; [de]=[hl], de++, hl++, bc--
    call memcpy
    pop hl
    pop bc
    add hl, bc
    ex de, hl 
    ld hl, (fileindex)
    add hl, bc
    ld (fileindex), hl
    pop hl
    or a
    sbc hl, bc
    ret z      ; If byte count is zero, we're done

    ld bc, hl  ; fake-ok - remaining bytes
    
    jp read    ; Go again


; bc = bytes
skipbytes:
    push bc
    ld hl, 512 + FILEBUF
    ld bc, (fileindex)
    or a
    sbc hl, bc
    call z, nextfileblock_hl512
    ; hl = max bytes to read at once
    pop bc  ; desired copy length
    push bc
    push hl
    or a
    sbc hl, bc
    jr nc, .oklen
    pop bc   ; max len
    push bc
.oklen:
    pop hl ; throw-away max len
    pop hl ; original copy length
    ; now bc = byte count, hl = original byte count
    push hl
    ld hl, (fileindex)
    add hl, bc
    ld (fileindex), hl
    pop hl
    or a
    sbc hl, bc
    ret z      ; If byte count is zero, we're done
    ld bc, hl  ; fake-ok
    jp skipbytes ; Go again


nextfileblock_hl512:
    ld hl, 512
nextfileblock:
    push af
    push hl
    push bc
    push de
    ld a, (filehandle)
    ld hl, FILEBUF ; mmu5
    ld (fileindex), hl
    ld bc, 512
    call fread
    pop de
    pop bc
    pop hl
    pop af
    ret

startstream:
restartstream:
endstream:
    ret
