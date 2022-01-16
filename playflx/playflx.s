    DEVICE ZXSPECTRUMNEXT
; PlayFLX
; FLX video player
; by Jari Komppa, http://iki.fi/sol
; 2021
    INCLUDE nextdefs.asm


; 0x0000 mmu0 = rom
; 0x2000 mmu1 = dot program
; 0x4000 mmu2 = unmapped for std prints to work
; 0x6000 mmu3 = 8k framebuffer
; 0x8000 mmu4 = stack/scratch
; 0xa000 mmu5 = file i/o buffer
; 0xc000 mmu6 = dot program copy (for isr to work - isr + rom calls = boom)
; 0xe000 mmu7 = isr trampoline + empty space up to 0xfe00

DOTADDR EQU 0xc000
DOTDIFF EQU 0xc000-0x2000
SCRATCH EQU 0x8000 

; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     DOTADDR
    di
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    ex af, af'
    push af
    exx
    push bc
    push de
    push hl

    ; grab mmu6 nextreg
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_MMU6
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    push af

    ; alloc page for our dot command copy
    ld      hl, 0x0001 ; alloc zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
    jp nc, allocfail-DOTDIFF        ; in case of failure, do a clean exit

    push af
    nextreg NEXTREG_MMU6, a        ; use the newly allocated page
    
    ld (spstore-DOTDIFF), sp

    ; set up dot command error handler - if we get an error,
    ; do the cleanup. No idea if this will actually work.
    ld      hl, teardown-DOTDIFF
	rst     0x8
	.db     0x95

    ; Copy the dot command over to the newly allocated page
    ld de, DOTADDR
    ld hl, 0x2000
    ld bc, 0x2000
    ldir
    
    ; And jump to it.
    jp realstart
realstart:

    ;call printbyte
    ;jp teardown-DOTDIFF

    STORENEXTREGMASK NEXTREG_CPU_SPEED, regstore, 3
    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.

    STORENEXTREG NEXTREG_MMU4, regstore + 13
    call allocpage
    jp nc, fail
    ld a, e
    ld (stackpage), a
    nextreg NEXTREG_MMU4, a    
    ld sp, 0xa000

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

    call setupdma

    call allocpage
    jp nc, fail
    ld a, e
    ld (isrpage), a
    nextreg NEXTREG_MMU7, a    

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
    ld a, NEXTREG_LAYER2_RAMPAGE
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    add a, a
    ld (framebufferpage), a
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_LAYER2_RAMSHADOWPAGE
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    add a, a
    ld (framebufferpage+1), a
    
    ld a, 2
    ld (framebuffers), a

    ; TODO: allocate more backbuffers

    ld de, 0
    ld bc, 256*192
    ld a, 0
    call screenfill 

    ld de, isr
    call setupisr7

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


; read rest of header

    call readword
    ld (frames), hl
    call readword
    ld (speed), hl

; next up: 512 bytes of palette

    ld hl, SCRATCH
    ld bc, 512
    call read

; set palette
    nextreg NEXTREG_PALETTE_INDEX, 0 ; start from palette index 0
    ld hl, SCRATCH
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
    ei
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
    ;halt
    pop bc
    djnz animloop

fail:

    di

    call closeisr7

    ld a, (filehandle)
    call fclose

    ld a, (filepage)
    ld e, a
    call freepage


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
    RESTORENEXTREG NEXTREG_CPU_SPEED, regstore


    ld a, (isrpage)
    ld e, a
    call freepage

    ld a, (stackpage)
    ld e, a
    call freepage
    RESTORENEXTREG NEXTREG_MMU4, regstore + 13

    jp teardown-DOTDIFF
teardown:
    ld sp, (spstore-DOTDIFF)
    pop af
    ld      hl, 0x0003 ; free zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
allocfail:
    pop af
    nextreg NEXTREG_MMU6, a

    pop hl
    pop de
    pop bc
    pop af
    exx
    ex af,af'
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af

    or a ; clear carry
    ei
    ret ; exit application

isr:
    ei
    reti

filehandle:
    db 0
fileindex:
    dw 0xa000
frames:
    db 0,0
speed:
    db 0,0
regstore:
    BLOCK 32, 0 ; currently 14 used
filepage:
    db 0
isrpage:
    db 0    
stackpage:
    db 0
framebuffers:
    db 0
framebufferpage:
    BLOCK 64, 0
spstore:
    db 0,0

fn:
    db "/flx/cube1_lrle8.flx", 0

    INCLUDE isr.asm
    INCLUDE cachedio.asm
    INCLUDE decoders.asm
    INCLUDE blitters.asm
    INCLUDE print.asm
    INCLUDE esxdos.asm
