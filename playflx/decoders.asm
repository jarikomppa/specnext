; ------------------------------------------------------------------------
LZ4:
; op < 0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op >=0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from current frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
; LZ copy from *current frame*
    ld a, (previousframe)
    push af
    ld a, (rendertarget)
    ld (previousframe), a
tickLZ4:
    push hl
    call readbyte
    or a
    jp m, rleLZ4
    jp z, rleLZ4
; len >  0 [len][16bit offset] or [127][16 bit len][16 bit offset]
    cp 127
    jr z, longcopyprevLZ4_a    
    ld b, 0
    ld c, a
    jr docopyprevLZ4_a
longcopyprevLZ4_a:    
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ4_a:
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
    jr z, LZ4done

tockLZ4:
    push hl
    call readbyte
    or a
    jp m, copyprevLZ4_b
; len >= 0 [len][len bytes] to copy
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
    jr z, LZ4done
    jp tickLZ4

LZ4done:
    pop af
    ld (previousframe), a
    jp blockdone

rleLZ4:
; len <= 0 [-len][run byte] or [-128][16 bit len][run byte]
    cp -128
    jr z, longrleLZ4
    neg
    ld b, 0
    ld c, a
    jr dorleLZ4
longrleLZ4:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
dorleLZ4:
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
    jr z, LZ4done
    jr tockLZ4

copyprevLZ4_b:    
; len <  0 [-len][16bit offset] or [-128][16 bit len][16 bit offset]
    cp -128
    jr z, longcopyprevLZ4
    neg
    ld c, a
    ld b, 0
    jr docopyprevLZ4
longcopyprevLZ4:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ4:
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
    jr z, LZ4done
    jp tickLZ4

; ------------------------------------------------------------------------
LZ5:
; op < 0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op >=0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from current frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLZ5:
    push hl
    call readbyte
    or a
    jp m, rleLZ5
    jp z, rleLZ5
; len >  0 [len][16bit offset] or [127][16 bit len][16 bit offset]
    cp 127
    jr z, longcopyprevLZ5_a    
    ld b, 0
    ld c, a
    jr docopyprevLZ5_a
longcopyprevLZ5_a:    
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ5_a:
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

tockLZ5:
    push hl
    call readbyte
    or a
    jp m, copyprevLZ5_b
; len >= 0 [len][len bytes] to copy
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
    jp tickLZ5

rleLZ5:
; len <= 0 [-len][run byte] or [-128][16 bit len][run byte]
    cp -128
    jr z, longrleLZ5
    neg
    ld b, 0
    ld c, a
    jr dorleLZ5
longrleLZ5:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
dorleLZ5:
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
    jr tockLZ5

copyprevLZ5_b:    
; len <  0 [-len][16bit offset] or [-128][16 bit len][16 bit offset]
    cp -128
    jr z, longcopyprevLZ5
    neg
    ld c, a
    ld b, 0
    jr docopyprevLZ5
longcopyprevLZ5:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ5:
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
    jp tickLZ5

; ------------------------------------------------------------------------
LZ6:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
; op >=0 [op][literal bytes] or [127][2 byte size][literal bytes] - Copy literal values
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLZ6:
    push hl
    call readbyte
    or a
    jp m, rleLZ6
    jp z, rleLZ6
; op > 0 [op][2 byte offset] or [127][2 byte size][2 byte offset] - Copy from previous frame
    cp 127
    jr z, longcopyprevLZ6_a    
    ld b, 0
    ld c, a
    jr docopyprevLZ6_a
longcopyprevLZ6_a:    
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ6_a:
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

tockLZ6:
    push hl
    call readbyte
    or a
    jp m, copyprevLZ6_b
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
    jp z, blockdone
    jp tickLZ6

rleLZ6:
; op <=0 [-op][runvalue] or [-128][2 byte size][runvalue] - RLE
    cp -128
    jr z, longrleLZ6
    neg
    ld b, 0
    ld c, a
    jr dorleLZ6
longrleLZ6:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
dorleLZ6:
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
    jp tockLZ5

copyprevLZ6_b:    
; op < 0 [-op][2 byte offset] or [-128][2 byte size][2 byte offset] - Copy from current frame
    cp -128
    jr z, longcopyprevLZ6_b
    neg
    ld c, a
    ld b, 0
    jr docopyprevLZ6_b
longcopyprevLZ6_b:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ6_b:
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
    jp tickLZ6

; ------------------------------------------------------------------------
LZ1B:
; op  > 0 [op][16 bit ofs] copy from previous
; op <= 0 [-op][run byte] 
; op >= 0 [op][.. op bytes ..]
; op <  0 [-op][16 bit ofs] copy from previous
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLZ1b:
    push hl
    call readbyte
    or a
    jp m, rleLZ1b
    jp z, rleLZ1b
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

tockLZ1b:
    push hl
    call readbyte
    or a
    jp m, copyprevLZ1b_b
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
    jp tickLZ1b

rleLZ1b:
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
    jr tockLZ1b

copyprevLZ1b_b:    
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
    jp tickLZ1b

; ------------------------------------------------------------------------
LZ3C:
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
; op <= 0 [-128][2 byte len] or [-op][run byte]
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
; op <= 0 [-128][2 byte len] or [-op][.. bytes ..]
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLZ3c:
    push hl
    call readbyte
    or a
    jp m, rleLZ3c
    jp z, rleLZ3c
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
    cp 127
    jr z, longcopyprevLZ3c_a    
    ld b, 0
    ld c, a
    jr docopyprevLZ3c_a
longcopyprevLZ3c_a:    
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ3c_a:
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

tockLZ3c:
    push hl
    call readbyte
    or a
    jp m, copyLZ3c_b
    jp z, copyLZ3c_b
; op >  0 [127][2 byte len] or [op][current ofs +/- signed byte] copy from previous
    cp 127
    jr z, longcopyprevLZ3c_b
    ld c, a
    ld b, 0
    jr docopyprevLZ3c_b
longcopyprevLZ3c_b
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyprevLZ3c_b:

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

    jp tickLZ3c

rleLZ3c:
; op <= 0 [-128][2 byte len] or [-op][run byte]
    cp -128
    jr z, longrleLZ3c
    neg
    ld b, 0
    ld c, a
    jr dorleLZ3c
longrleLZ3c:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
dorleLZ3c:
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
    jr tockLZ3c

copyLZ3c_b:
; op <= 0 [-128][2 byte len] or [-op][.. bytes ..]
    cp -128
    jr z, longcopyLZ3c
    neg
    ld c, a
    ld b, 0
    jr docopyLZ3c
longcopyLZ3c:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
docopyLZ3c:
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
    jp tickLZ3c

; ------------------------------------------------------------------------
SAMEFRAME: ;chunktype = 0;  printf("s"); break;
    call readword ; hl = bytes in block; ignored, as it's 0
    ld de, 0 ; screen offset
    ld ix, 0 ; source offset
    ld bc, 256*192
    call screencopyfromprevframe
    jp blockdone

; ------------------------------------------------------------------------
BLACKFRAME: ;chunktype = 13;  printf("b"); break;
    call readword ; hl = bytes in block; ignored, as it's 0
    ld de, 0 ; screen offset
    ld bc, 256*192
    ld a, 0
    call screenfill
    jp blockdone

; ------------------------------------------------------------------------
ONECOLOR: ;chunktype = 101;  printf("o"); break;
    call readword ; hl = bytes in block; ignored, as it's 1
    call readbyte ; color
    ld de, 0 ; screen offset
    ld bc, 256*192    
    call screenfill
    jp blockdone

; ------------------------------------------------------------------------
UNKNOWN: ; Just skip it.
    call readword
    ld bc, hl ; fake-ok
    call skipbytes
    jp blockdone

