    DEVICE ZXSPECTRUMNEXT
; PlayFLX
; FLX video player
; by Jari Komppa, http://iki.fi/sol
; 2022
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
    ld (cmdline-DOTDIFF), hl
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
    ld a, e

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
    STORENEXTREG NEXTREG_MMU7, regstore + 4
    STORENEXTREG NEXTREG_DISPLAY_CONTROL_1, regstore + 5
    STORENEXTREG NEXTREG_LAYER2_CONTROL, regstore + 6
    STORENEXTREG NEXTREG_GENERAL_TRANSPARENCY, regstore + 7
    STORENEXTREG NEXTREG_TRANSPARENCY_COLOR_FALLBACK, regstore + 8
    STORENEXTREG NEXTREG_ENHANCED_ULA_CONTROL, regstore + 9
    STORENEXTREG NEXTREG_ENHANCED_ULA_INK_COLOR_MASK, regstore + 10
    STORENEXTREG NEXTREG_ULA_CONTROL, regstore + 11
    STORENEXTREG NEXTREG_LAYER2_RAMPAGE, regstore + 12

    call parsecmdline

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

    ld  hl, SCRATCH
    ld  b,  1       ; open existing
    call    fopen
    jp  c,  fail_open
    ld (filehandle), a

    call nextfileblock

    call readbyte
    cp 'N'
    jp nz, fail_type

    call readbyte
    cp 'X'
    jp nz, fail_type

    call readbyte
    cp 'F'
    jp nz, fail_type

    call readbyte
    cp 'L'
    jp nz, fail_type


; header tag read and apparently fine at this point.

    ; allocate framebuffers
allocframebuffers:
    ld b, 37-5 ; total framebuffers to try
    ; -5 because specnext 3.01.05 core has a bug and doesn't display the last few. TODO: upgrade to .10
    ; (curiously, cspect has a very similar issue)
    ld e, 0 ; page; 6 pages per framebuffer (222 pages to reserve)
    ld hl, allocpages
    ld ix, framebufferpage
.nextframebuffer:
    push bc
    ld a, e
    ld (SCRATCH+100), a ; put framebuffer's first page to scratch
    ld c, 0 ; successful reserves
    ld b, 6 ; pages per framebuffer
.nextframebufferpage:
    push bc
    push de
    push hl
    call reservepage
    pop hl
    pop de
    pop bc
    jr nc, .reservefail
    inc c         ; on successful alloc, increase c,
    ld (hl), e    ; store page to be freed on teardown
    inc hl        ; allocpages++
.reservefail:
    inc e         ; next page
    djnz .nextframebufferpage
    ld a, c
    cp 6
    jr nz, .noframebuffer ; failed to allocate at least 1 of the 6 pages, no twinkie
    ld a, (SCRATCH+100) ; put the first page of the framebuffer to table
    ld (ix), a
    ;call printbyte
    inc ix
    ld a, (framebuffers) ; increase number of framebuffers
    inc a
    ld (framebuffers), a
    ;call printbyte
.noframebuffer:    
    pop bc
    djnz .nextframebuffer

    ; make sure we have at least two framebuffers
    ld a, (framebuffers)
    cp 2
    jp c, fail_mem

;    ld a, 29
;    ld (framebuffers), a

    ; set rendertarget and previous to some legal value
    ld a, (renderpageidx)
    ld c, a
    ld b, 0
    ld hl, framebufferpage
    add hl, bc
    ld a, (hl)
    ld (rendertarget), a
    ld (previousframe), a


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

setpalette:
; set palette
    nextreg NEXTREG_PALETTE_INDEX, 0 ; start from palette index 0
    ld hl, SCRATCH
    ld b, 0
.loop1:
    ld a, (hl)
    inc hl
    nextreg NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION, a
    djnz .loop1

    ; since there's 512 bytes, let's do it again
.loop2:
    ld a, (hl)
    inc hl
    nextreg NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION, a
    djnz .loop2

; ready for animation loop

; ------------------------------------------------------------------------

startanim:
    ld bc, (frames)
    ei
animloop:
    push bc


    ld a, (framebuffers)
    ld e, a
    ld a, (renderpageidx)
    inc a              ; renderpageidx++
    cp e
    jr nz, .notrollover ; if renderpageidx == framebuffers
    ld a, 0
.notrollover:
    ld (renderpageidx), a
    ld e, a
    ld hl, framebufferpage
    ld c, a
    ld b, 0
    add hl, bc
    ld a, (hl)
    ld (rendertarget), a

    ;call printbyte
.wait:
    ;call isrc
    ld a, (showpageidx)
    cp e       ; if showpageidx == renderpageidx
    jr z, .wait ; wait for the isr to progress

;    ; TODO: remember to remove this clear
;    PUSHALL
;    ld de, 0
;    ld bc, 256*192
;    ld a, 100
;    call screenfill 
;    POPALL

    call readbyte
    ;call printbyte
    cp 18
    jp z, LZ5
    cp 17
    jp z, LZ4
    cp 13
    jp z, LZ1B
    cp 19
    jp z, LZ6
    cp 21
    jp z, LZ3C
    cp 1
    jp z, SAMEFRAME
    cp 2
    jp z, BLACKFRAME
    cp 7
    jp z, ONECOLOR

    jp UNKNOWN
blockdone:
    call readword     ; checksums -> hl
  /*  
    call calcchecksum ; checksums -> de
    or a
    sbc hl, de
    jr z, .checksum_ok
    add hl, de
    call printword
    ex de, hl
    call printword
    ld hl, (frames)
    pop bc ; number of frames    
    or a
    sbc hl, bc
    call printword
    call writeout
    jp fail
.checksum_ok:    
*/
    ; advance the readypage so it can be shown
    ld a, (renderpageidx)
    ld (readypageidx), a ; mark current renderpage as ready
    ld a, (rendertarget)
    ld (previousframe), a ; current render target is now previous
    
input_call:
    call userinput
    
    pop bc ; number of frames
    dec bc
    ld a, b
    or c
    jp nz, animloop

; ------------------------------------------------------------------------

loopjumppoint: ; option writes jump here
    nop
    nop
    nop

; ------------------------------------------------------------------------

    ; Done with decoding, wait for frames to show

    ld a, (readypageidx)
    ld e, a
.waitforfinish:
    ;call isrc
    ld a, (showpageidx)
    cp e
    jr nz, .waitforfinish ; wait for the isr to progress

fail:

    di

    call closeisr7

    ld a, (filehandle)
    call fclose

    ld a, (filepage)
    ld e, a
    call freepage

    ; free allocated framebuffer pages
    ld hl, allocpages
    ld b, 0
freeframebuffers:
    ld a, (hl)
    cp 0
    jr z, .notallocated
    ld e, a
    push bc
    push hl
    call freepage
    pop hl
    pop bc
.notallocated:
    inc hl 
    djnz freeframebuffers


    RESTORENEXTREG NEXTREG_MMU3, regstore + 1
    RESTORENEXTREG NEXTREG_MMU5, regstore + 2
    RESTORENEXTREG NEXTREG_MMU7, regstore + 4
    RESTORENEXTREG NEXTREG_DISPLAY_CONTROL_1, regstore + 5
    RESTORENEXTREG NEXTREG_LAYER2_CONTROL, regstore + 6
    RESTORENEXTREG NEXTREG_GENERAL_TRANSPARENCY, regstore + 7
    RESTORENEXTREG NEXTREG_TRANSPARENCY_COLOR_FALLBACK, regstore + 8
    RESTORENEXTREG NEXTREG_ENHANCED_ULA_CONTROL, regstore + 9
    RESTORENEXTREG NEXTREG_ENHANCED_ULA_INK_COLOR_MASK, regstore + 10
    RESTORENEXTREG NEXTREG_ULA_CONTROL, regstore + 11
restore_layer2_rampage: ; for the option not to restore this reg
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
    ld e, a
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

loopanim:
; perform loop
; note: do this *before* waitforfinish for better results
; (audio, if any, may require special handling for loops though)
    ld bc, 0
    ld de, 0
    ld hl, 0
    ld a, (filehandle)
    call fseek ; seek to beginning
    call nextfileblock
    ld bc, 4+2+2+512
    call skipbytes
    jp startanim




fail_open_msg:
    db "File open failed\r", 0
fail_open:
    ld hl, fail_open_msg
    call printmsg
    jp fail

fail_type_msg:
    db "Wrong file type\r",0
fail_type:
    ld hl, fail_type_msg
    call printmsg
    jp fail

fail_mem_msg:
    db "Can't allocate framebuffers\r",0
fail_mem:
    ld hl, fail_mem_msg
    call printmsg
    jp fail


isr:
;    reti
;isrc:
    PUSHALL
    
    ; Wait for N frames
    ld bc, (speed)
    ld a, (framewaits)
    inc a
    cp c
    jr z, .nodelay
    ld (framewaits), a
    jp .notready
.nodelay:
    ; If we can, advance to the next frame.
    ld a, (readypageidx)
    ld e, a
    ld a, (framebuffers)
    ld d, a
    ld a, (showpageidx)
    cp a, e
    jr z, .notready    ; if showpageidx == readypageidx 
    inc a              ; showpageidx++
    cp a, d
    jr nz, .notrollover ; if showpageidx != framebuffers
    ld a, 0
.notrollover:
    ld (showpageidx), a
    ld d, 0
    ld e, a
    ld hl, framebufferpage
    add hl, de
    ld a, (hl)
    ;call printbyte
    srl a ; 16k pages
    ;call printbyte
    nextreg NEXTREG_LAYER2_RAMPAGE, a

    ; Clear frame waits here (so if frame wasn't realy we'll show it ASAP)
    ld a, 0
    ld (framewaits), a

.notready:

    POPALL
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
framewaits:
    db 0
regstore:
    BLOCK 16, 0 ; currently 14 used
filepage:
    db 0
isrpage:
    db 0    
stackpage:
    db 0
framebuffers:
    db 0
framebufferpage:
    BLOCK 40, 0
allocpages:
    BLOCK 256, 0
previousframe:
    db 0
rendertarget:
    db 0
renderpageidx:
    db 0
readypageidx:
    db 0
showpageidx:
    db 0
spstore:
    db 0,0
cmdline
    dw 0


;fn:
    ;db "/flx/output.flx", 0
;    db "/flx/hw.flx", 0


/*
    INCLUDE checksum.asm
*/    
    INCLUDE isr.asm
    INCLUDE cachedio.asm
    INCLUDE decoders.asm
    INCLUDE blitters.asm
    INCLUDE print.asm
    INCLUDE esxdos.asm
    INCLUDE cmdline.asm
    INCLUDE userinput.asm
