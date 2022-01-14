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

    ld  hl, fn
    ld  b,  1       ; open existing
    call    fopen
    jp  c,  fail
    ld (filehandle), a

    ld a, (filehandle)
    call freadbyte
    cp 'N'
    jp nz, fail

    ld a, (filehandle)
    call freadbyte
    cp 'X'
    jp nz, fail

    ld a, (filehandle)
    call freadbyte
    cp 'F'
    jp nz, fail

    ld a, (filehandle)
    call freadbyte
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
    ld (framebufferpages), a

    ; TODO: allocate more backbuffers

; read rest of header

    ld a, (filehandle)
    call freadword
    ld (frames), hl
    ld a, (filehandle)
    call freadword
    ld (speed), hl

; next up: 512 bytes of palette

    ld a, (filehandle)
    ld hl, (palette)
    ld bc, 512
    call fread

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
  
; ready for animation loop

    ld a, (frames)
    ld b, a
animloop:
    push bc

    ld a, (filehandle)
    call freadbyte
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

    ret ; exit application

LINEARRLE8: ;chunktype = 102; printf("l"); break;
    ; [runbytes][runvalue]
    ; op < 0  [-runbytes][runvalue]
    ; op >= 0 [copybytes][..bytes..]
    ld a, (filehandle)
    call freadword
    ; hl = bytes in block
    ld a, (framebufferpage)
    nextreg NEXTREG_MMU3, a
    ld de, 0x6000 
decodeloop:
    push hl

    push de
    ld a, (filehandle)
    call freadbyte
    pop de
    ld (de), a
    inc de

    pop hl
    dec hl

    ld a, h
    or a, l
    jr nz, decodeloop
    halt
    jp blockdone

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
    ld a, (filehandle)
    call freadword
    call fskipbytes
    jp blockdone


filehandle:
    db 0
frames:
    db 0,0
speed:
    db 0,0
palette:
    BLOCK 512, 0 ; could be in temp data (like backbuffer)
scratch:
    BLOCK 1024, 0 ; todo: move elsewhere (or get rid of)
regstore:
    BLOCK 32, 0 ; currently 13 used
framebufferpages:
    db 0
framebufferpage:
    BLOCK 64, 0    

fn:
    db "/flx/cube1_lrle8.flx", 0

    INCLUDE print.asm
    INCLUDE esxdos.asm
