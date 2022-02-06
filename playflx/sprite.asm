
initsprites:
    ; A sprite is 16x16 bytes (256 bytes, or /2 for 4bit sprites)
    ld bc, PORT_SPRITE_SLOT_SELECT
    ld a, 0
    out (c), a
    ld bc, PORT_SPRITE_PATTERN_UPLOAD
    ld b, 16
    ld a, 0
.spriteuploadloop1:
    out (c), a
    djnz .spriteuploadloop1
    ld b, 16
    ld a, 255
.spriteuploadloop2:
    out (c), a
    djnz .spriteuploadloop2
    ld b, 16
    ld a, 0
.spriteuploadloop3:
    out (c), a
    djnz .spriteuploadloop3

    ld b, 256-16*3
    ld a, 0xe3 ; default transparent
.spriteuploadloop4:
    out (c), a
    djnz .spriteuploadloop4
    ld bc, PORT_SPRITE_ATTRIBUTE_UPLOAD
    ld a, 0
    out (c), a ; x ofs
    ld a, 0
    out (c), a ; y ofs
    ld a, 0
    out (c), a ; misc options
    ld a, 0x80

    out (c), a ; visible, select pattern
    ld a, 0
    out (c), a ; x ofs
    ld a, 0
    out (c), a ; y ofs
    ld a, 0
    out (c), a ; misc options
    ld a, 0x80
    out (c), a ; visible, select pattern
    nextreg NEXTREG_SPRITE_AND_LAYERS, 1 + 2; enable sprites, sprites in border    
    ret        

    ; in: hl
spritepos:
    ld bc, PORT_SPRITE_SLOT_SELECT
    ld a, 0
    out (c), a
    ld bc, PORT_SPRITE_ATTRIBUTE_UPLOAD
    ld a, 0
    out (c), a ; x ofs
    out (c), h ; y ofs
    ld bc, PORT_SPRITE_SLOT_SELECT

    ld a, 1
    out (c), a
    ld bc, PORT_SPRITE_ATTRIBUTE_UPLOAD
    ld a, 0
    out (c), a ; x ofs
    out (c), l ; y ofs
    ret
