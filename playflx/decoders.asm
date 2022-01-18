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
    ;call screenfill
    ld hl, de ; fake-ok
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
    ;call screenfill
    ld hl, de ; fake-ok
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




SAMEFRAME: ;chunktype = 0;  printf("s"); break;
BLACKFRAME: ;chunktype = 13;  printf("b"); break;
RLEFRAME: ;chunktype = 15; printf("r"); break;
DELTA8FRAME: ;chunktype = 12; printf("d"); break;
DELTA16FRAME: ;chunktype = 7;  printf("D"); break;
FLI_COPY: ;chunktype = 16; printf("c"); break;
ONECOLOR: ;chunktype = 101;  printf("o"); break;
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
