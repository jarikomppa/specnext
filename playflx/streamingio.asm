; replacement for cachedio using streaming i/o

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
    ld hl, 512 + 0xa000
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
    ld hl, 512 + 0xa000
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

filemap:
    BLOCK 126,0 ; 640 megs ought to be enough?
filemapptr:
    dw 0
blocksleft:
    dw 0
cardflags:
    db 0


endstream:
    ld a,(cardflags)
    rst 0x8
    db 0x87 ; DISK_STRMEND
    ret

startstream:
    ld a, (filehandle)
    ld hl, filemap
    ld de, 21 
    rst 0x8
    db 0x85 ; DISK_FILEMAP
    ld (cardflags), a
    jp c, streaming_failed1 ; call failed
    ld a, d
    or e
    jp z, streaming_failed2 ; no entries
    ex de, hl
    ld de, 20
    or a
    sbc hl, de
    jp c, streaming_failed3 ; too many entries

    ld hl, filemap
    ld (filemapptr), hl
    call startfileblock
    ret

startfileblock:
    ld hl, (filemapptr)
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld c, (hl)
    inc hl
    ld b, (hl) ; BCDE=card address
    inc hl
    push bc    ; (stack)DE=card address            
    ld c, (hl)
    inc hl
    ld b, (hl) ; BC=number of 512-byte blocks
    inc hl
    ld (filemapptr), hl
    ld (blocksleft), bc
    pop hl     ; HLDE=card address from dot
    ld a, (cardflags)
    or a, 0x80 ; we'll wait for the start token
    rst 0x8
    db 0x86 ; DISK_STRMSTART
    jp c, streaming_failed4
    ret

nextfileblock_hl512:
    ld hl, 512
nextfileblock:
    push af
    push hl
    push bc
    push de
    ld c, 0xeb
waittoken: ; wait for new data block to be ready
    in a,(c)
    inc a
    jr z, waittoken    
    ; a should be 0xfe+1 now, we probably should check for that..
    ld hl, 0xa000
    ld (fileindex), hl
;   INI = (hl)=(c), hl++, b--
;   move this 1KB of INI elsewhere (generate them)
    .512 ini
    in a, (c)       ; skip crc 1/2 (needs (n)op between)
    ld hl, (blocksleft)
    dec hl
    ld (blocksleft), hl
    in a, (c)       ; skip crc 2/2
    ld a, h
    or l
    call z, nextfilemapblock
    pop de
    pop bc
    pop hl
    pop af
    ret

nextfilemapblock:
    call endstream
    call startfileblock
    ret

restartstream:
    ld a, (filehandle)
    ld hl, SCRATCH
    ld bc, 1
    call fread ; streaming api wants us to read a byte
    call startstream
    ret


streamfailmsg1:
    db "Can't map file.\r",0
streamfailmsg2:
    db "No file map entries.\r",0
streamfailmsg3:
    db "File too fragmented. Try .defrag?\r",0
streamfailmsg4:
    db "Streaming failed.\r",0
streaming_failed1:
    ld hl, streamfailmsg1
    call printmsg
    jp fail
streaming_failed2:
    ld hl, streamfailmsg2
    call printmsg
    jp fail
streaming_failed3:
    ld hl, streamfailmsg3
    call printmsg
    jp fail    
streaming_failed4:
    ld hl, streamfailmsg4
    call printmsg
    jp fail        