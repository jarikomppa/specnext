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

; de dest (returns advanced by +bc)
; hl src (returns advanced by +bc)
; bc bytes (is preserved)
; LDIR cost is 52 + 21*bc @3.5MHz ; 62 + 24*bc @28MHz (+read_wait)
; DMA cost is 233 + 4*bc @3.5MHz ; 279 + 5*bc @28MHz (+read_wait)
memcpy:
    ; check BC against break-even point (LDIR vs DMA)
    ld a, 12 ; break-even is 12 @28MHz, 11 @3.5MHz
    cp c
    sbc a, a                    ; 00 for C <0..12>, FF for C <13..FF>
    or b                        ; non-zero for BC > 12
    jr nz, .dma_memcpy
    push bc
    ldir
    pop bc
    ret
.dma_memcpy:
    ; this code style using `out (n),a` is actually reasonably fast and preserves HL,DE,BC for free
    ld a, 0b01111101; // R0-Transfer mode, A -> B, write adress + block length
    out (PORT_DATAGEAR_DMA), a
    ld a, l ; source
    out (PORT_DATAGEAR_DMA), a
    ld a, h ; source
    out (PORT_DATAGEAR_DMA), a
    ld a, c ; count
    out (PORT_DATAGEAR_DMA), a
    ld a, b ; count
    out (PORT_DATAGEAR_DMA), a
    ld a, 0b10101101; // R4-Continuous mode (use this for block transfer), write dest adress
    out (PORT_DATAGEAR_DMA), a
    ld a, e ; dest
    out (PORT_DATAGEAR_DMA), a
    ld a, d ; dest
    out (PORT_DATAGEAR_DMA), a
    ld a, 0b11001111; // R6-Load
    out (PORT_DATAGEAR_DMA), a
    ld a, 0x87;       // R6-Enable DMA
    IFNDEF PERF_GRIND
    out (PORT_DATAGEAR_DMA), a
    ENDIF
    ; advance HL,DE the same way how LDIR would, but BC is preserved
    add hl, bc
    ex de, hl
    add hl, bc
    ex de, hl
    ret

; ------------------------------------------------------------------------

; de = screen offset (returns advanced by +bc)
; bc = bytes to fill
; a = byte to fill
screenfill:
    ld (.a+1), a ; set the self-modify value to seed memcpy
    ; check if we're filling zero bytes
    ld a, b
    or c
    ret z

.nonzerobc:
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
    nextreg DSTMMU, a
    inc a
    nextreg DSTMMU+1, a
    inc a
    nextreg DSTMMU+2, a
    
    ; calculate output address masked to MMU5..7 (0xa000..0xffff) region
    ld a, d
    and 0x1f
    or high DESTADDR ; carry = 0
    ld d, a ; de = output address
    ld h, a
    ld l, e ; hl = output address too
    inc de ; de++ for memcpy
    dec bc ; bc = count-1 for memcpy and max-span test

    ; clamp bc = count-1 to fit into dest window
    ld a, l
    add a, c
    ld a, h
    adc a, b ; carry = 0 if span fits
    jr nc, .okspan
    ld a, l ; clamped "(count-1)": bc = 0xffff-hl = -1 + -hl = -1 + (~hl + 1) = ~hl
    cpl
    ld c, a
    ld a, h
    cpl
    ld b, a
.okspan:
    ; now hl,de,bc are set for memcpy (but needs bc==0 check), stack: screen ofs, original count
.a: ld (hl), 0 ; seed memcpy (self-modify value)
    ld a, b
    or c
    call nz, memcpy ; if BC!=0 then fill the rest

    inc bc ; restore bc to fill-count
    pop hl
    pop de
    add hl, bc
    ex de, hl ; de = advance screen offset, hl = original count, carry = 0
    sbc hl, bc
    ret z ; all bytes filled

    ld bc, hl  ; fake-ok - remaining bytes

    jp .nonzerobc ; let's go again

; ------------------------------------------------------------------------

; de = screen offset (returns advanced by +bc)
; bc = bytes to copy
screencopyfromfile:
    ; check if we're copying zero bytes
    ld a, b
    or c
    ret z
.nonzerobc:
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
    nextreg DSTMMU, a
    inc a
    nextreg DSTMMU+1, a

    ; calculate max span and output address masked to MMU3 0x6000 region
    ld hl, 0x4000 + DESTADDR
    ld a, d
    and 0x1f
    or high DESTADDR ; carry = 0
    ld d, a
    sbc hl, de ; hl = max span, carry = 0
    sbc hl, bc
    jr nc, .okspan
    add hl, bc
    ld bc, hl ; fake-ok - clamp bc to maxspan
.okspan:
    ; now bc = count, de = output address, stack: screen ofs, original count

    push bc
    call read
    pop bc
    pop hl
    add hl, bc ; advance screen offset, carry = 0
    ex de,hl
    pop hl
    sbc hl, bc
    ret z ; all bytes copied

    ld bc, hl  ; fake-ok - remaining bytes
    
    jp .nonzerobc ; let's go again


; ------------------------------------------------------------------------

; de = screen offset (returns advanced by +bc)
; bc = bytes to copy
; ix = prev frame offset (returns advanced by +bc)
screencopyfromprevframe:
    ; check if we're copying zero bytes
    ld a, b
    or c
    ret z
    ld a, (rendertarget)
    ld (.rt+1), a ; self-modify the render target calculation

.nonzerobc:
    push bc ; preserve origcount
    push de ; and screen offset

    ; map framebuffer bank
    ld a, ixh
    rlca
    rlca
    rlca
    and 7
.pf:add a, 123 ; previousframe variable (self-modify storage)
    nextreg SRCMMU, a

    ; calculate max span and source address masked to MM2 0x4000 region
    ld hl, 0x2000 + SRCADDR
    ld e, ixl
    ld a, ixh
    and 0x1f
    or high SRCADDR ; carry = 0
    ld d, a ; de = source offset masked to 8ki at 0xa000

    sbc hl, de ; hl = max span, carry = 0
    sbc hl, bc
    jr nc, .okspan
    add hl, bc
    ld bc, hl ; fake-ok - clamp bc to maxspan
.okspan:
    ex de, hl
    pop de
    ; hl = source address, de = screen offset, bc = clamped count, stack: original count

    ; map framebuffer bank (16ki window, so any source-span always fits)
    push de
    ld a, d
    rlca
    rlca
    rlca
    and 7
.rt:add a, 0 ; self-modified to value of (rendertarget)
    nextreg DSTMMU, a
    inc a
    nextreg DSTMMU+1, a

    ; calculate target address
    ld a, d
    and 0x1f
    or high DESTADDR ; carry = 0
    ld d, a
    ; hl = source address, de = output address, bc = clamped count, stack: screen ofs, original count

    call memcpy

    add ix, bc ; advance source offset
    pop hl
    add hl, bc ; advance screen offset, carry = 0
    ex de,hl
    pop hl
    sbc hl, bc
    ld a, (filepage)
    nextreg SRCMMU, a
    ret z ; all bytes copied

    ld bc, hl  ; fake-ok - remaining bytes

    jp .nonzerobc ; let's go again
