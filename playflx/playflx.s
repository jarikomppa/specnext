    DEVICE ZXSPECTRUMNEXT
; PlayFLX
; FLX video player
; by Jari Komppa, http://iki.fi/sol
; 2021

; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     2000h
    nextreg 7, 3 ; 28mhz mode. TODO: clean up
    ld  hl, hello
    call    printmsg
    call    printnewline
    ld  hl, fn
    ld  b,  1       ; open existing
    call    fopen
    jp  c,  fail
    jp      openok
okmsg:
    db "(ok)",0
openok:
    ld (filehandle), a
    ld hl, okmsg
    call printmsg

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

    ld hl, okmsg
    call printmsg

    ld a, (filehandle)
    call freadword
    ld (frames), hl
    ld a, (filehandle)
    call freadword
    ld (speed), hl

    ld a, (filehandle)
    ld hl, (palette)
    ld bc, 512
    call fread

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

    ld a, (filehandle)
    call fclose

    ret

SAMEFRAME: ;chunktype = 0;  printf("s"); break;
BLACKFRAME: ;chunktype = 13;  printf("b"); break;
RLEFRAME: ;chunktype = 15; printf("r"); break;
DELTA8FRAME: ;chunktype = 12; printf("d"); break;
DELTA16FRAME: ;chunktype = 7;  printf("D"); break;
FLI_COPY: ;chunktype = 16; printf("c"); break;
ONECOLOR: ;chunktype = 101;  printf("o"); break;
LINEARRLE8: ;chunktype = 102; printf("l"); break;
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
    BLOCK 512, 0
scratch:
    BLOCK 1024, 0

failmsg:
    db "something failed",0
fail:
    ld hl, failmsg
    call printmsg
    ld a, (filehandle)
    call fclose

    ret

hello:
    db "hello world", 0
fn:
    db "/flx/cube1.flx", 0

printmsg:
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printmsg

hex:
    db "0123456789ABCDEF"
newline:
    db "\r", 0

;; ab 00 ab -> ba b0 0a
printbyte:
    ld b, a
    and 0xf
    ld c, a
    ld a, b
    .4 rrca
    and 0xf
    ld b, a
    ; b and c contain nibbles now
    ld hl, hex
    ld d, 0
    ld e, b
    add hl, de
    ld a, (hl)
    ld hl, scratch
    ld (hl), a
    ld hl, hex
    ld e, c
    add hl, de
    ld a, (hl)
    ld hl, scratch+1
    ld (hl), a
    ld a, 0
    inc hl
    ld (hl), a
    ld hl, scratch
    jp printmsg

printword:
    push hl
    ld a, h
    call printbyte
    pop hl
    ld a, l
    jp printbyte

printnewline:
    ld hl, newline
    jp printmsg

;extern unsigned char allocpage()
; output l = page, 0 = error
allocpage:
    ld      hl, 0x0001 ; alloc zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
    ld      l, 0
	jr      nc, allocfail
	ld      l, e
allocfail:	
	ret

;extern unsigned char reservepage(unsigned char page)
; e = page, output l = page, 0 = error
reservepage:
    ld      hl, 0x0002 ; reserve zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
    ld      l, 0
	jr      nc, reservefail
	ld      l, e
reservefail:	
	ret

;extern void freepage(unsigned char page)
; e = page
freepage:
    ld      hl, 0x0003 ; free zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
	ret

; hl = filename
; b = mode
;       esx_mode_read           $01    request read access
;       esx_mode_write          $02    request write access
;       esx_mode_use_header     $40    read/write +3DOS header
;                plus one of:
;       esx_mode_open_exist     $00    only open existing file
;       esx_mode_open_creat     $08    open existing or create file
;       esx_mode_creat_noexist  $04    create new file, error if exists
;       esx_mode_creat_trunc    $0c    create new file, delete existing
; output: a = handle, carry = failure
fopen:
	ld  a,  '*'
	rst     0x8
	.db     0x9a
    ret

;extern void fclose(unsigned char handle);
; a = handle
fclose:
    rst     0x8
    .db     0x9b
    ret

;extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
; a = handle
; hl = buf
; bc = bytes
; output: bc = bytes
fread:
    rst     0x8
    .db     0x9d
	ret

fskipbytes:
    ld bc, 1024
    or a
    sbc hl, bc
    jr c, lastblock
    push hl
    jr skipread
lastblock:
    add hl, bc
    ld b, h
    ld c, l
    ld hl, 0
    push hl
skipread:
    ld hl, scratch
    ld a, (filehandle)
    call fread
    pop hl
    inc h
    dec h
    jr nz, fskipbytes
    inc l
    dec l
    jr nz, fskipbytes
    ret


freadbyte:
    ld hl, scratch
    ld bc, 1
    call fread
    ld a, (scratch)
    ret
freadword:
    ld hl, scratch
    ld bc, 2
    call fread
    ld hl, (scratch)
    ret
freaddword:
    ld hl, scratch
    ld bc, 4
    call fread
    ret



;extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
; a = handle
; bc = bytes
; hl = buf
fwrite:
    rst     0x8
    .db     0x9e
	ret

;extern void fseek(unsigned char handle, unsigned long ofs);
; a = handle
; offset = BCDE
; mode = l (0 = set, 1 = fwd, 2 = bwd)
fseek:
    rst     0x8
    .db     0x9f
	ret
