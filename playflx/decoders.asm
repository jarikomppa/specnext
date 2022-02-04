; ------------------------------------------------------------------------
LZ4:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from current frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
; LZ copy from *current frame*
    ld a, (previousframe)
    push af
    ld a, (rendertarget)
    ld (previousframe), a
.tick:
    push hl
    call readbyte
    or a
    jp m, .rle
    jp z, .rle
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from current frame
    cp 127
    jr z, .longcopyprev_a
    ld b, 0
    ld c, a
    jr .docopyprev_a
.longcopyprev_a:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_a:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jr z, .done

.tock:
    push hl
    call readbyte
    or a
    jp m, .copyprev_b
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    ld b, 0
    ld c, a
    push bc
    push de
    call screencopyfromfile
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a
    sbc hl, bc
    ld a, h
    or a, l
    jr z, .done
    jp .tick

.done:
    pop af
    ld (previousframe), a
    jp blockdone

.rle:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
    cp -128
    jr z, .longrle
    neg
    ld b, 0
    ld c, a
    jr .dorle
.longrle:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.dorle:
    call readbyte
    push bc
    push de
    call screenfill
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jr z, .done
    jr .tock

.copyprev_b:
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
    cp -128
    jr z, .longcopyprev
    neg
    ld c, a
    ld b, 0
    jr .docopyprev
.longcopyprev:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jr z, .done
    jp .tick

; ------------------------------------------------------------------------
LZ5:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from previous frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
.tick:
    push hl
    call readbyte
    or a
    jp m, .rle
    jp z, .rle
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
    cp 127
    jr z, .longcopyprev_a
    ld b, 0
    ld c, a
    jr .docopyprev_a
.longcopyprev_a:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_a:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone

.tock:
    push hl
    call readbyte
    or a
    jp m, .copyprev_b
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from previous frame
    cp 127
    jr z, .longcopyfromfile
    ld b, 0
    ld c, a
    jr .docopyfromfile
.longcopyfromfile:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyfromfile:
    push bc
    push de
    call screencopyfromfile
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

.rle:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
    cp -128
    jr z, .longrle
    neg
    ld b, 0
    ld c, a
    jr .dorle
.longrle:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.dorle:
    call readbyte
    push bc
    push de
    call screenfill
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jr .tock

.copyprev_b:
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from previous frame
    cp -128
    jr z, .longcopyprev
    neg
    ld c, a
    ld b, 0
    jr .docopyprev
.longcopyprev:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

; ------------------------------------------------------------------------
LZ6:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
.tick:
    push hl
    call readbyte
    or a
    jp m, .rle
    jp z, .rle
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
    cp 127
    jr z, .longcopyprev_a
    ld b, 0
    ld c, a
    jr .docopyprev_a
.longcopyprev_a:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_a:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone

.tock:
    push hl
    call readbyte
    or a
    jp m, .copyprev_b
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    cp 127
    jr z, .longcopyfromfile
    ld b, 0
    ld c, a
    jr .docopyfromfile
.longcopyfromfile:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyfromfile:
    push bc
    push de
    call screencopyfromfile
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

.rle:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
    cp -128
    jr z, .longrle
    neg
    ld b, 0
    ld c, a
    jr .dorle
.longrle:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.dorle:
    call readbyte
    push bc
    push de
    call screenfill
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp .tock

.copyprev_b:
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
    cp -128
    jr z, .longcopyprev_b
    neg
    ld c, a
    ld b, 0
    jr .docopyprev_b
.longcopyprev_b:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_b:
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok

; LZ copy from *current frame*
    ld a, (previousframe)
    push af
    ld a, (rendertarget)
    ld (previousframe), a

    call screencopyfromprevframe
    
    pop af
    ld (previousframe), a

    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

; ------------------------------------------------------------------------
LZ1B:
; op  > 0 [op][16 bit ofs] copy from previous
; op <= 0 [-op][run byte] 
; op >= 0 [op][.. op bytes ..]
; op <  0 [-op][16 bit ofs] copy from previous
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
.tick:
    push hl
    call readbyte
    or a
    jp m, .rle
    jp z, .rle
; op  > 0 [op][16 bit ofs] copy from previous
    ld b, 0
    ld c, a
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone

.tock:
    push hl
    call readbyte
    or a
    jp m, .copyprev
; op >= 0 [op][.. op bytes ..]
    ld b, 0
    ld c, a
    push bc
    push de
    call screencopyfromfile
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

.rle:
; op <= 0 [-op][run byte] 
    neg
    ld b, 0
    ld c, a
    call readbyte
    push bc
    push de
    call screenfill
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jr .tock

.copyprev
; op <  0 [-op][16 bit ofs] copy from previous
    neg
    ld c, a
    ld b, 0
    call readword
    push bc
    push de
    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

; ------------------------------------------------------------------------
LZ3C:
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
; op <= 0 [-128][2 byte len] or [-op][run byte]
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
; op <= 0 [-128][2 byte len] or [-op][.. bytes ..]
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
.tick:
    push hl
    call readbyte
    or a
    jp m, .rle
    jp z, .rle
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
    cp 127
    jr z, .longcopyprev_a
    ld b, 0
    ld c, a
    jr .docopyprev_a
.longcopyprev_a:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_a:
    call readbyte ; +/- offset
    push bc
    push de

    ex hl, de ; hl = offset
    ld e, a  ; sign extend a -> de
    add a, a ;
    sbc a, a ;
    ld d, a  ;
    
    add hl, de

    pop de   ; restore de = offset
    push de

    ld ix, hl ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone

.tock:
    push hl
    call readbyte
    or a
    jp m, .copy_b
    jp z, .copy_b
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
    cp 127
    jr z, .longcopyprev_b
    ld c, a
    ld b, 0
    jr .docopyprev_b
.longcopyprev_b
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopyprev_b:

    call readbyte ; +/- offset
    push bc
    push de

    ex hl, de
    ld e, a  ; sign extend a -> de
    add a, a ;
    sbc a, a ;
    ld d, a  ;

    add hl, de    
    ld ix, hl ; fake-ok

    pop de
    push de

    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone

    jp .tick

.rle:
; op <= 0 [-128][2 byte len] or [-op][run byte]
    cp -128
    jr z, .longrle
    neg
    ld b, 0
    ld c, a
    jr .dorle
.longrle:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.dorle:
    call readbyte
    push bc
    push de
    call screenfill
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jr .tock

.copy_b:
; op <= 0 [-128][2 byte len] or [-op][.. bytes ..]
    cp -128
    jr z, .longcopy
    neg
    ld c, a
    ld b, 0
    jr .docopy
.longcopy:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
.docopy:
    push bc
    push de
    call screencopyfromfile
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jp .tick

; ------------------------------------------------------------------------
SAMEFRAME: ;chunktype = 0;  printf("s"); break;
    call readword ; hl = bytes in block; ignored, as it's 0
    ex de, hl ; screen offset 0
    ld ix, 0 ; source offset
    ld bc, 256*192
    call screencopyfromprevframe
    jp blockdone

; ------------------------------------------------------------------------
BLACKFRAME: ;chunktype = 13;  printf("b"); break;
    call readword ; hl = bytes in block; ignored, as it's 0
    xor a
    jp ONECOLOR.withA

; ------------------------------------------------------------------------
ONECOLOR: ;chunktype = 101;  printf("o"); break;
    call readword ; hl = bytes in block; ignored, as it's 1
    call readbyte ; color
.withA:
    ld bc, 256*192 ; 0xc000
    ld d, c
    ld e, c ; screen offset 0
    call screenfill
    jp blockdone

; ------------------------------------------------------------------------
UNKNOWN: ; Just skip it.
    call readword
    ld bc, hl ; fake-ok
    call skipbytes
    jp blockdone

