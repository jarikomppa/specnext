; out: a
readbyte:
    push hl
    push bc
    ld hl, 8192 + 0xa000
    ld bc, (fileindex)
    or a    
    sbc hl, bc
    call z, nextfileblock
    ld hl, (fileindex)
    ld a, (hl)  
    inc hl
    ld (fileindex), hl
    pop bc
    pop hl
    ret

; out: hl
readword:
    call readbyte
    ld l, a
    call readbyte
    ld h, a
    ret

; hl = buf
; bc = bytes
read:
    push hl
    push bc
    ld hl, 8192 + 0xa000
    ld bc, (fileindex)
    or a
    sbc hl, bc
    jr nz, .doread
    call nextfileblock
    ld hl, 8192
.doread:
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
    
    ld hl, de  ; fake-ok - destination address
    
    jp read    ; Go again


; bc = bytes
skipbytes:
    push bc
    ld hl, 8192 + 0xa000
    ld bc, (fileindex)
    or a
    sbc hl, bc
    jr nz, .doskip
    call nextfileblock
    ld hl, 8192
.doskip:
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


nextfileblock:
    push af
    push hl
    push bc
    push de
    ld a, (filehandle)
    ld hl, 0xa000 ; mmu5
    ld bc, 8192
    call fread
    ld hl, 0xa000
    ld (fileindex), hl
    pop de
    pop bc
    pop hl
    pop af
    ret
