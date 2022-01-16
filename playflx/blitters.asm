
; de = screen offset
; bc = bytes to fill
; a = byte to fill
screenfill:
    push af
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, screenfill_nonzero
    pop af
    ret
    
screenfill_nonzero:
    push bc
    push de
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7

    ld hl, framebufferpage
    add a, (hl)
    nextreg NEXTREG_MMU3, a
    
    ; calculate max span
    pop de
    push de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    sbc hl, de    
    ; hl = max span
    pop de
    pop bc
    push bc
    push de
    push hl
    sbc hl, bc   
    jr nc, okspan
    pop bc  ; bc = max span
    push bc
okspan:    
    pop hl ; throw-away unused maxspan
    pop de
    pop hl ; original byte count    
    pop af

    ; now a = color, bc = count, hl = original count, de = screen ofs

    push de
    push bc
    push hl
    push af

    ; calculate output address
    ld a, d
    and 0x1f
    ld d, a
    ld hl, 0x6000 ; mmu3
    add hl, de
    ld de, hl ; fake-ok
    pop af
    push af
    ld (hl), a ; seed memfill
    inc de
    dec bc
    ; if we're filling one byte, make sure we're not filling 64k..
    ld a, b
    or c
    jr z, skipcopy
    ldir ; (de)=(hl), de++, hl++, bc--
skipcopy:
    pop af
    pop hl
    pop bc
    pop de
    sbc hl, bc
    ret z ; all bytes filled
    ex de, hl  ;
    add hl, bc ; add de, bc - increment screen offset
    ex de, hl  ;
    ld bc, hl  ; fake-ok - remaining bytes
    jp screenfill ; let's go again


; de = screen offset
; bc = bytes to fill
screencopyfromfile:
    push bc
    push de
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, screenfill_nonzerofromfile
    pop de
    pop bc
    ret
    
screenfill_nonzerofromfile:
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7
    cp 6
    ret z
    cp 7
    ret z
    ld hl, framebufferpage
    add a, (hl)
    nextreg NEXTREG_MMU3, a
    
    ; calculate max span
    pop de
    push de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    sbc hl, de    
    ; hl = max span
    pop de
    pop bc
    push bc
    push de
    push hl
    sbc hl, bc   
    jr nc, okspanfromfile
    pop bc  ; bc = max span
    push bc
okspanfromfile:    
    pop hl ; throw-away unused maxspan
    pop de
    pop hl ; original byte count    

    ; now bc = count, hl = original count, de = screen ofs

    push de
    push bc
    push hl

    ; calculate output address
    ld a, d
    and 0x1f
    ld d, a
    ld hl, 0x6000 ; mmu3
    add hl, de
    call read
    pop hl
    pop bc
    pop de
    sbc hl, bc
    ret z ; all bytes filled
    ex de, hl  ;
    add hl, bc ; add de, bc - increment screen offset
    ex de, hl  ;
    ld bc, hl  ; fake-ok - remaining bytes
    jp screencopyfromfile ; let's go again
