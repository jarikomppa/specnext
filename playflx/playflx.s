    DEVICE ZXSPECTRUMNEXT
; PlayFLX
; FLX video player
; by Jari Komppa, http://iki.fi/sol
; 2021
    INCLUDE nextdefs.asm

; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     2000h

    STORENEXTREGMASK NEXTREG_CPU_SPEED, regstore, 3
    STORENEXTREG NEXTREG_MMU3, regstore + 1
    STORENEXTREG NEXTREG_MMU5, regstore + 2
    STORENEXTREG NEXTREG_MMU6, regstore + 3
    STORENEXTREG NEXTREG_MMU7, regstore + 4
    STORENEXTREG NEXTREG_DISPLAY_CONTROL_1, regstore + 5
    STORENEXTREG NEXTREG_LAYER2_CONTROL, regstore + 6
    STORENEXTREG NEXTREG_GENERAL_TRANSPARENCY, regstore + 7
    STORENEXTREG NEXTREG_TRANSPARENCY_COLOR_FALLBACK, regstore + 8
    STORENEXTREG NEXTREG_ENHANCED_ULA_CONTROL, regstore + 9
    STORENEXTREG NEXTREG_ENHANCED_ULA_INK_COLOR_MASK, regstore + 10
    STORENEXTREG NEXTREG_ULA_CONTROL, regstore + 11
    STORENEXTREG NEXTREG_LAYER2_RAMPAGE, regstore + 12

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.

    call allocpage
    jp nc, fail
    ld a, e
    ld (filepage), a
    nextreg NEXTREG_MMU5, a    

    ld  hl, fn
    ld  b,  1       ; open existing
    call    fopen
    jp  c,  fail
    ld (filehandle), a

    call nextfileblock

    call readbyte
    cp 'N'
    jp nz, fail

    call readbyte
    cp 'X'
    jp nz, fail

    call readbyte
    cp 'F'
    jp nz, fail

    call readbyte
    cp 'L'
    jp nz, fail

; header tag read and apparently fine at this point.

    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_DISPLAY_CONTROL_1
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    or 0x80       ; enable layer 2
    out (c), a

    nextreg NEXTREG_LAYER2_CONTROL, 0 ; 256x192 resolution, palette offset 0
    nextreg NEXTREG_GENERAL_TRANSPARENCY, 0 ; transparent color = 0
    nextreg NEXTREG_TRANSPARENCY_COLOR_FALLBACK, 0 ; fallback color = 0
    nextreg NEXTREG_ENHANCED_ULA_CONTROL, 0x11 ; enable ulanext & layer2 palette 1
    nextreg NEXTREG_ENHANCED_ULA_INK_COLOR_MASK, 0xff ; ulanext color mask
    nextreg NEXTREG_ULA_CONTROL, 0x80 ; disable ULA

    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_LAYER2_RAMPAGE
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    add a, a
    ld (framebufferpage), a
    inc a
    ld (framebufferpage+1), a
    inc a
    ld (framebufferpage+2), a
    inc a
    ld (framebufferpage+3), a
    inc a
    ld (framebufferpage+4), a
    inc a
    ld (framebufferpage+5), a
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_LAYER2_RAMSHADOWPAGE
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    add a, a
    ld (framebufferpage+6), a
    inc a
    ld (framebufferpage+7), a
    inc a
    ld (framebufferpage+8), a
    inc a
    ld (framebufferpage+9), a
    inc a
    ld (framebufferpage+10), a
    inc a
    ld (framebufferpage+11), a
    
    ld a, 2
    ld (framebuffers), a

    ; TODO: allocate more backbuffers

; read rest of header

    call readword
    ld (frames), hl
    call readword
    ld (speed), hl

; next up: 512 bytes of palette

    ld hl, palette
    ld bc, 512
    call read

    ld de, 0
    ld bc, 256*192
    ld a, 0
    call screenfill

; set palette

    nextreg NEXTREG_PALETTE_INDEX, 0 ; start from palette index 0
    ld hl, palette
    ld b, 0
pal_loop1:
    ld a, (hl)
    inc hl
    nextreg NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION, a
    djnz pal_loop1
    ; since there's 512 bytes, let's do it again
pal_loop2:
    ld a, (hl)
    inc hl
    nextreg NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION, a
    djnz pal_loop2


/*
    nextreg NEXTREG_PALETTE_INDEX, 0 ; start from palette index 0
    ld b, 0
pal_loop:
    ld a, b
    nextreg NEXTREG_PALETTE_VALUE, a
    djnz pal_loop
*/    
; ready for animation loop

    ld a, (frames)
    ld b, a
animloop:
    push bc

    call readbyte
    push af
    call printbyte
    pop af
    cp 13
    jp z, LZ1B
    cp 10
    jp z, LINEARDELTA8
    cp 16
    jp z, LZ3
    cp 11
    jp z, LINEARDELTA16
    cp 15
    jp z, LZ2B
    cp 14
    jp z, LZ2
    cp 12
    jp z, LZ1
    cp 8
    jp z, LINEARRLE8
    cp 1
    jp z, SAMEFRAME
    cp 2
    jp z, BLACKFRAME
    cp 7
    jp z, ONECOLOR
    cp 9
    jp z, LINEARRLE16
    cp 3
    jp z, RLEFRAME
    cp 4
    jp z, DELTA8FRAME
    cp 5
    jp z, DELTA16FRAME
    cp 6
    jp z, FLI_COPY
    jp UNKNOWN
blockdone:
    pop bc
    djnz animloop

fail:

    ld a, (filehandle)
    call fclose

    ld a, (filepage)
    ld e, a
    call freepage

    RESTORENEXTREG NEXTREG_CPU_SPEED, regstore
    RESTORENEXTREG NEXTREG_MMU3, regstore + 1
    RESTORENEXTREG NEXTREG_MMU5, regstore + 2
    RESTORENEXTREG NEXTREG_MMU6, regstore + 3
    RESTORENEXTREG NEXTREG_MMU7, regstore + 4
    RESTORENEXTREG NEXTREG_DISPLAY_CONTROL_1, regstore + 5
    RESTORENEXTREG NEXTREG_LAYER2_CONTROL, regstore + 6
    RESTORENEXTREG NEXTREG_GENERAL_TRANSPARENCY, regstore + 7
    RESTORENEXTREG NEXTREG_TRANSPARENCY_COLOR_FALLBACK, regstore + 8
    RESTORENEXTREG NEXTREG_ENHANCED_ULA_CONTROL, regstore + 9
    RESTORENEXTREG NEXTREG_ENHANCED_ULA_INK_COLOR_MASK, regstore + 10
    RESTORENEXTREG NEXTREG_ULA_CONTROL, regstore + 11
    RESTORENEXTREG NEXTREG_LAYER2_RAMPAGE, regstore + 12

    or a ; clear carry
    ret ; exit application

; out: a
readbyte:
    push hl
    push bc
    ld hl, 8192 + 0xa000
    ld bc, (fileindex)
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
    sbc hl, bc
    jr nz, doread
    call nextfileblock
    ld hl, 8192
doread:
    ; hl = max bytes to read at once
    pop bc  ; desired copy length
    push bc
    push hl
    sbc hl, bc
    jr nc, oklen
    pop bc   ; max len
    push bc
oklen:
    pop hl ; throw-away max len
    pop hl ; original copy length
    pop de ; destination address
    ; now de = dest, bc = byte count, hl = original byte count
    push hl
    push bc
    ld hl, (fileindex)
    ldir ; [de]=[hl], de++, hl++, bc--
    pop bc
    ld hl, (fileindex)
    add hl, bc
    ld (fileindex), hl
    pop hl
    sbc hl, bc
    ret z      ; If byte count is zero, we're done
    ld bc, hl  ; fake-ok
    ld hl, de  ; fake-ok
    jp read    ; Go again


; bc = bytes
skipbytes:
    push bc
    ld hl, 8192 + 0xa000
    ld bc, (fileindex)
    sbc hl, bc
    jr nz, doskip
    call nextfileblock
    ld hl, 8192
doskip:
    ; hl = max bytes to read at once
    pop bc  ; desired copy length
    push bc
    push hl
    sbc hl, bc
    jr nc, skip_oklen
    pop bc   ; max len
    push bc
skip_oklen:
    pop hl ; throw-away max len
    pop hl ; original copy length
    ; now bc = byte count, hl = original byte count
    push hl
    ld hl, (fileindex)
    add hl, bc
    ld (fileindex), hl
    pop hl
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


LINEARRLE8: ;chunktype = 102; printf("l"); break;
    ; [runbytes][runvalue]
    ; op < 0  [-runbytes][runvalue]
    ; op >= 0 [copybytes][..bytes..]
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
decodeloop:
    push hl

    ; [runbytes][runvalue]
    call readbyte
    ld b, 0
    ld c, a
    call readbyte
    call screenfill
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    push hl

    call readbyte
    or a
    jp p, copy
    ; op < 0  [-runbytes][runvalue]
    neg
    ld b, 0
    ld c, a
    call readbyte
    call screenfill
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp decodeloop

copy:
    ; op >= 0 [copybytes][..bytes..]
    ld b, 0
    ld c, a
    call screencopyfromfile
    ;add de, bc
    ex de, hl
    add hl, bc
    ex de, hl

    pop hl
    dec hl
    sub hl, bc ; fake-ok
    ld a, h
    or a, l
    jp z, blockdone
    jp decodeloop

SAMEFRAME: ;chunktype = 0;  printf("s"); break;
BLACKFRAME: ;chunktype = 13;  printf("b"); break;
RLEFRAME: ;chunktype = 15; printf("r"); break;
DELTA8FRAME: ;chunktype = 12; printf("d"); break;
DELTA16FRAME: ;chunktype = 7;  printf("D"); break;
FLI_COPY: ;chunktype = 16; printf("c"); break;
ONECOLOR: ;chunktype = 101;  printf("o"); break;
LINEARRLE16: ;chunktype = 103; printf("L"); break;
LINEARDELTA8: ;chunktype = 104; printf("e"); break;
LINEARDELTA16: ;chunktype = 105; printf("E"); break;
LZ1: ;chunktype = 106; printf("1"); break;
LZ2: ;chunktype = 107; printf("2"); break;
LZ3: ;chunktype = 108; printf("3"); break;
LZ1B: ;chunktype = 109; printf("4"); break;
LZ2B: ;chunktype = 110; printf("5"); break;
UNKNOWN:
    call readword
    ld bc, hl ; fake-ok
    call skipbytes
    jp blockdone



filehandle:
    db 0
fileindex:
    dw 0xa000
frames:
    db 0,0
speed:
    db 0,0
palette:
    BLOCK 512, 0 ; could be in temp data (like backbuffer)
regstore:
    BLOCK 32, 0 ; currently 13 used
filepage:
    db 0
framebuffers:
    db 0
framebufferpage:
    BLOCK 64, 0    

fn:
    db "/flx/cube1_lrle8.flx", 0

    INCLUDE blitters.asm
    INCLUDE print.asm
    INCLUDE esxdos.asm
