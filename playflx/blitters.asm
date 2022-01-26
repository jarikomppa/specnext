setupdma:
    ld hl, .initdata
    ld bc, PORT_DATAGEAR_DMA | (.datasize<<8)
    otir
    ret
.initdata:
    db  $C3,$C3,$C3,$C3,$C3,$C3 ; reset DMA from any possible state (6x reset command)
    db  %10'0'0'0010            ; WR5 = stop on end of block, /CE only
    db  %1'0'000000             ; WR3 = all extra features [of real chip] disabled
    db  %0'1'01'0'100           ; WR1 = A address ++, memory
    db  2                       ; + custom 2T timing
    db  %0'1'01'0'000           ; WR2 = B address ++, memory
    db  2                       ; + custom 2T timing
.datasize: EQU $ - .initdata

; ------------------------------------------------------------------------

; de dest
; hl src
; bc bytes
memcpy:
    ; check BC against break-even point (LDIR vs DMA)
    ld a, 20 ; todo: figure out actual break-even point
    cp c
    sbc a, a                    ; 00 for C <0..20>, FF for C <21..FF>
    or b                        ; non-zero for BC > 20
    jr nz, .dma_memcpy
    ldir
    ret
.dma_memcpy:
    push de
    push bc
    push hl
    ld bc, PORT_DATAGEAR_DMA
    ld a, 0x83; // DMA_DISABLE
    out (PORT_DATAGEAR_DMA), a
    ld a, 0b01111101; // R0-Transfer mode, A -> B, write adress + block length
    out (PORT_DATAGEAR_DMA), a
    pop hl
    out (c), l ; source
    out (c), h ; source
    pop hl
    out (c), l ; count
    out (c), h ; count
    ld a, 0b10101101; // R4-Continuous mode (use this for block transfer), write dest adress
    out (PORT_DATAGEAR_DMA), a
    pop hl
    out (c), l ; dest
    out (c), h ; dest
    ld a, 0b10000010; // R5-Restart on end of block, RDY active LOW
    out (PORT_DATAGEAR_DMA), a
    ld a, 0b11001111; // R6-Load
    out (PORT_DATAGEAR_DMA), a
    ld a, 0x87;       // R6-Enable DMA
    out (PORT_DATAGEAR_DMA), a
    ret

; ------------------------------------------------------------------------

; de = screen offset
; bc = bytes to fill
; a = byte to fill
screenfill:
    push af
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, .nonzero
    pop af
    ret
    
.nonzero:
    push bc
    push de
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7

    ld hl, rendertarget
    add a, (hl)
    nextreg NEXTREG_MMU3, a
    
    ; calculate max span
    pop de
    push de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    or a
    sbc hl, de    
    ; hl = max span
    pop de
    push de
    push hl
    or a
    sbc hl, bc   
    jr nc, .okspan
    pop bc  ; bc = max span
    push bc
.okspan:
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
    jr z, .skipcopy
    ;ldir ; (de)=(hl), de++, hl++, bc--
    call memcpy
.skipcopy:
    pop af
    pop hl
    pop bc
    pop de
    or a
    sbc hl, bc
    ; de = screen offset, hl = remaining bytes, bc = bytes just processed, a = color
    ret z ; all bytes filled

    ex de, hl  ;
    add hl, bc ; add de, bc - increment screen offset
    ex de, hl  ;

    ld bc, hl  ; fake-ok - remaining bytes

    jp screenfill ; let's go again

; ------------------------------------------------------------------------

; de = screen offset
; bc = bytes to fill
screencopyfromfile:
    push bc
    push de
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, .nonzero
    pop de
    pop bc
    ret
    
.nonzero:
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7
    ld hl, rendertarget
    add a, (hl)
    nextreg NEXTREG_MMU3, a
    
    ; calculate max span
    pop de
    push de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    or a
    sbc hl, de    
    ; hl = max span
    pop de
    pop bc
    push bc
    push de
    push hl
    or a
    sbc hl, bc   
    jr nc, .okspan
    pop bc  ; bc = max span
    push bc
.okspan:
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
    or a
    sbc hl, bc
    ret z ; all bytes filled

    ex de, hl  ;
    add hl, bc ; add de, bc - increment screen offset
    ex de, hl  ;

    ld bc, hl  ; fake-ok - remaining bytes
    
    jp screencopyfromfile ; let's go again


; ------------------------------------------------------------------------

; de = screen offset
; bc = bytes to fill
; ix = prev frame offset
screencopyfromprevframe:
    push bc
    push de
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, .nonzero
    pop de
    pop bc
    ret
    
.nonzero:
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7
    ld hl, rendertarget
    add a, (hl)
    nextreg NEXTREG_MMU3, a
    
    ; calculate max span
    pop de
    push de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    or a
    sbc hl, de    
    ; hl = max span
    pop de
    pop bc
    push bc
    push de
    push hl
    or a
    sbc hl, bc   
    jr nc, .okspan
    pop bc  ; bc = max span
    push bc
.okspan:    
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
    push ix
    call readprevframe
    pop ix
    pop hl
    pop bc
    pop de
    or a
    sbc hl, bc
    ret z ; all bytes filled

    ex de, hl  ;
    add hl, bc ; add de, bc - increment screen offset
    ex de, hl  ;

    add ix, bc

    ld bc, hl  ; fake-ok - remaining bytes
    
    jp screencopyfromprevframe ; let's go again

; hl = target address
; bc = bytes
; ix = source offset
readprevframe:
    ; check if we're filling zero bytes
    ld a, b
    or c
    jr nz, .nonzero
    ret
    
.nonzero:
    push hl ; stack: target addr
    push bc ; stack: bytecount, target address
    push ix 
    pop de  
    ; map framebuffer bank
    ld a, d
    rlca
    rlca
    rlca
    and 7
    ld hl, previousframe
    add a, (hl)
    nextreg NEXTREG_MMU5, a
    
    ; calculate max span
    push ix
    pop de
    ld a, d
    and 0x1f ; mask to 8k
    ld d, a
    ld hl, 0x2000
    or a
    sbc hl, de    
    ; hl = max span
    pop bc  ; stack: target address
    push bc
    push hl ; stack: maxspan, bytecount, targetaddr
    or a
    sbc hl, bc   
    jr nc, .okspan
    pop bc  ; stack: bytecount, targetaddr
    push bc ; stack: maxspan, bytecount, targetaddr
.okspan:    
    pop hl ; stack: bytecount, targetaddr
    pop hl ; stack: targetaddr
    pop af ; stack: -

    ; now bc = count, hl = original count, de = masked prevframe addr 

    push bc
    push hl
    push af ; stack = count, origcount, targetaddr

    ; calculate output address
    ld hl, 0xa000 ; mmu5
    add hl, de
    pop de
    push de
    ; hl = source address
    ; de = dest address
    ; bc = count

    call memcpy
    ld a, (filepage)
    nextreg NEXTREG_MMU5, a
    pop de ; dest addr
    pop hl ; origcount
    pop bc ; count
    or a
    sbc hl, bc
    ret z ; all bytes filled
;    ret

    add ix, bc ; increment source offset

    ex de, hl  
    add hl, bc ; increment destination offset

    ld bc, de  ; fake-ok - remaining bytes
    
; hl = target address
; bc = bytes
; ix = source offset
    jp readprevframe ; let's go again
