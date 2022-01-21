; ------------------------------------------------------------------------
LINEARRLE8: ;chunktype = 102; printf("l"); break;
    ; [runbytes][runvalue]
    ; op >= 0 [copybytes][..bytes..]
    ; op < 0  [-runbytes][runvalue]
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
decodeloopLRLE8:
    push hl

    ; [runbytes][runvalue]
    call readbyte
    ld b, 0
    ld c, a
    call readbyte
    push de
    push bc
    call screenfill
    pop bc
    pop de
    
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
    jp p, copyLRLE8
    ; op < 0  [-runbytes][runvalue]
    neg
    ld b, 0
    ld c, a
    call readbyte
    push de
    push bc
    call screenfill
    pop bc
    pop de
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp decodeloopLRLE8

copyLRLE8:
    ; op >= 0 [copybytes][..bytes..]
    ld b, 0
    ld c, a
    push de
    push bc
    call screencopyfromfile
    pop bc
    pop de
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a ; clear carry
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jr decodeloopLRLE8

; ------------------------------------------------------------------------
LINEARRLE16: 
    ; op >= 0, copy op bytes
    ; op < 0, run -op bytes except if op == -128, read next 2 bytes and run with that

    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
decodeloopLRLE16:
    push hl

    call readbyte
    or a
    jp p, copyLRLE16
    ; op < 0, run -op bytes except if op == -128, read next 2 bytes and run with that
    cp -128
    jr z, longrunLRLE16
    neg
    ld b, 0
    ld c, a
    jr runLRLE16
longrunLRLE16:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl    ; fake-ok
runLRLE16:    
    call readbyte
    push de
    push bc
    call screenfill
    pop bc
    pop de
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp decodeloopLRLE16

copyLRLE16:
    ; op >= 0, copy op bytes
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
    or a ; clear carry
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jr decodeloopLRLE16

; ------------------------------------------------------------------------
LINEARDELTA8: ;chunktype = 104; printf("e"); break;
; op >= 0: [op][runbyte]   - run op bytes
; op <  0: [-op]           - copy -op bytes from prevframe
; op >  0: [op][..bytes..] - copy op bytes from file
; op <= 0: [-op]           - copy -op bytes from prevframe
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLD8:
    push hl

    call readbyte
    or a
    jp m, copyprevLD8_a
; op >= 0: [op][runbyte]   - run op bytes
    ld b, 0
    ld c, a
    call readbyte
    push de
    push bc
    call screenfill
    pop bc
    pop de
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
tockLD8:
    push hl
    call readbyte
    or a
    jp m, copyprevLD8_b
; op >  0: [op][..bytes..] - copy op bytes from file
    ld b, 0
    ld c, a
    push de
    push bc
    call screencopyfromfile
    pop bc
    pop de

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a ; clear carry
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jr tickLD8

copyprevLD8_a:
; op <  0: [-op]           - copy -op bytes from prevframe
    neg
    ld b, 0
    ld c, a
    push bc
    push de
    ld ix, de ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jr tockLD8

copyprevLD8_b:
; op <  0: [-op]           - copy -op bytes from prevframe
    neg
    ld b, 0
    ld c, a
    push bc
    push de
    ld ix, de ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp tickLD8


; ------------------------------------------------------------------------
LINEARDELTA16: ;chunktype = 105; printf("E"); break;
; op >= 0: [op][runbyte]   - run op bytes, if 127, read 2 more bytes for run length
; op <  0: [-op]           - copy -op bytes from prevframe, if -128, read 2 more bytes for run length
; op >  0: [op][..bytes..] - copy op bytes from file
; op <= 0: [-op]           - copy -op bytes from prevframe, if -128, read 2 more bytes for run length
    call readword ; hl = bytes in block
    ld de, 0 ; screen offset
tickLD16:
    push hl

    call readbyte
    or a
    jp m, copyprevLD16_a
; op >= 0: [op][runbyte]   - run op bytes, if 127, read 2 more bytes for run length
    cp 127
    jr z, golongrunLD16
    ld b, 0
    ld c, a
    jr gorunLD16
golongrunLD16:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl ; fake-ok
gorunLD16:    
    call readbyte
    push de
    push bc
    call screenfill
    pop bc
    pop de
    
    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
tockLD16:
    push hl
    call readbyte
    or a
    jp m, copyprevLD16_b    
; op >  0: [op][..bytes..] - copy op bytes from file    
    ld b, 0
    ld c, a
    push de
    push bc
    call screencopyfromfile
    pop bc
    pop de

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    or a ; clear carry
    sbc hl, bc
    ld a, h
    or a, l
    jp z, blockdone
    jr tickLD16

copyprevLD16_a:
; op <  0: [-op]           - copy -op bytes from prevframe, if -128, read 2 more bytes for run length
    cp -128
    jr z, longcopyLD16a    
    neg
    ld b, 0
    ld c, a
    jr docopyLD16a
longcopyLD16a:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl      ; fake-ok  
docopyLD16a:    
    push bc
    push de
    ld ix, de ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jr tockLD16

copyprevLD16_b:
; op <  0: [-op]           - copy -op bytes from prevframe, if -128, read 2 more bytes for run length
    cp -128
    jr z, longcopyLD16b
    neg
    ld b, 0
    ld c, a
    jr docopyLD16b
longcopyLD16b:
    pop hl
    dec hl
    dec hl
    push hl
    call readword
    ld bc, hl      ; fake-ok  
docopyLD16b:    
    push bc
    push de
    ld ix, de ; fake-ok
    call screencopyfromprevframe
    pop de
    pop bc

    ex de, hl  ;
    add hl, bc ; add de, bc
    ex de, hl  ;

    pop hl
    dec hl
    ld a, h
    or a, l
    jp z, blockdone
    jp tickLD16


LZ1: ; retired
; [16 bit offset][8 bit runlen] copy from previous
; op >= 0 [op][.. op bytes ..]
; op <  0 [-op][run byte]

LZ2: ; retired
; [16 bit offset][16 bit len] copy from previous
; op >= 0 [op][.. op bytes ..]
; op <  0 [-op][run byte]


LZ2B: ; retired
; op <  0 [-(op << 8 | 8 bits) runlen][16 bit ofs] copy from previous
; op >= 0 [op][run byte]
; [copylen][.. bytes ..]
