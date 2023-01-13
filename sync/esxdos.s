	.module esxdos
	.globl _fopen
	.globl _fclose
	.globl _fread
	.globl _fwrite
	.globl _writenextreg
	.globl _readnextreg
	.globl _allocpage
	.globl _freepage
;	.globl _makepath
	.globl _conprint
	.globl _println
	.globl _memcmp
	.globl _strinstr
	.globl _parse_cmdline
	.globl _createfilewithpath
	.area _CODE

; TODO: AF, BC, DE, HL changed by esxdos calls, need to preserve?

;       B=access modes, a combination of:
;         any/all of:
;esx_mode_read           $01    request read access
;esx_mode_write          $02    request write access
;esx_mode_use_header     $40    read/write +3DOS header
;         plus one of:
;esx_mode_open_exist     $00    only open existing file
;esx_mode_open_creat     $08    open existing or create file
;esx_mode_creat_noexist  $04    create new file, error if exists
;esx_mode_creat_trunc    $0c    create new file, delete existing

;extern unsigned char fopen(unsigned char *fn, unsigned char mode);
_fopen::
	push	ix
	ld	ix, #0
	add	ix, sp
    ld iy, (_osiy)
	ld	l,  4 (ix) ; fn
	ld	h,  5 (ix)
	ld  b,  6 (ix) ; mode
	ld  a,  #'*'
	rst     #0x8
	.db     #0x9a
	ld      hl, #0
	jr      c, openfail
	ld      l, a
openfail:	
	pop ix
    ret

;extern void fclose(unsigned char handle);
_fclose::
    ld iy, (_osiy)
	ld	hl, #2+0
	add	hl, sp
	ld	a, (hl) ; handle
    rst     #0x8
    .db     #0x9b
    ret

;extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
_fread::
	push	ix
	ld	ix, #0
	add	ix, sp
    ld iy, (_osiy)
	ld	a,  4 (ix) ; handle
	ld	l,  7 (ix) ; bytes
	ld	h,  8 (ix)
	ld	c,  l
	ld	b,  h
	ld	l,  5 (ix) ;buf
	ld	h,  6 (ix)
    rst     #0x8
    .db     #0x9d
    ld  h, b
    ld  l, c
	pop	ix
	ret
    
;extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
_fwrite::
	push	ix
	ld	ix, #0
	add	ix, sp
    ld iy, (_osiy)
	ld	a,  4 (ix) ; handle
	ld	l,  7 (ix) ; bytes
	ld	h,  8 (ix)
	ld	c,  l
	ld	b,  h
	ld	l,  5 (ix) ; buf
	ld	h,  6 (ix)
    rst     #0x8
    .db     #0x9e
	pop	ix
	ret

;extern void writenextreg(unsigned char reg, unsigned char val);
_writenextreg::
	pop af
	pop hl
	push hl
	push af
	ld bc, #0x243B
	ld a, l
	out (c), a
	inc b
	ld a, h
	out (c),a
	ret

;extern unsigned char readnextreg(unsigned char reg);
_readnextreg::
	pop af
	pop hl
	push hl
	push af
	ld bc, #0x243B
	ld a, l
	out (c), a
	inc b
	ld a, h
	in a,(c)
	ld l, a
	ret

; Note: most likely requires most of the normal banks to be mapped to work

;extern unsigned char allocpage()
_allocpage::
    ld iy, (_osiy)
    ld      hl, #0x0001 ; alloc zx memory
    exx                             ; place parameters in alternates
    ld      de, #0x01bd             ; IDE_BANK
    ld      c, #7                   ; "usually 7, but 0 for some calls"
    rst     #0x8
    .db     #0x94                   ; +3dos call
    ld      l, #0
	jr      nc, allocfail
	ld      l, e
allocfail:	
	ret

;extern void freepage(unsigned char page)
_freepage::
    ld iy, (_osiy)
	ld	    hl, #2+0
	add	    hl, sp
	ld	    e, (hl)  ; page
    ld      hl, #0x0003 ; free zx memory
    exx                             ; place parameters in alternates
    ld      de, #0x01bd             ; IDE_BANK
    ld      c, #7                   ; "usually 7, but 0 for some calls"
    rst     #0x8
    .db     #0x94                   ; +3dos call
	ret

; extern void makepath(char *pathspec); // must be 0xff terminated!
;_makepath::
;    pop de  ; return address
;    pop hl  ; char *pathspec
;    push hl ; restore stack
;    push de
;    ld iy, (_osiy)
   
;    ld a, #0x02 ; make path
;    exx                             ; place parameters in alternates
;    ld      de, #0x01b1             ; IDE_PATH
;    ld      c, #7                   ; "usually 7, but 0 for some calls"
;    rst     #0x8
;    .db     #0x94                   ; +3dos call

;	ret
    

;extern void conprint(char *txt) __z88dk_fastcall;
; hl = pointer to text
_conprint:
    ld iy, (_osiy)
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jp _conprint

;hl = pointer to text
_println:
	ld de, #0x5c8c
	ld a, #0xff
	ld (de), a ; disable scroll? prompt
	call _conprint
	ld a, #0x0d ; newline
	rst 16
	ret

; stack: retaddr, ptr_a, ptr_b, len, return l
_memcmp:
	pop af ; retaddr
	pop hl ; ptr_a
	pop de ; ptr_b
	pop bc ; len
	push bc
	push de
	push hl
	push af ; stack restored
	ld b,c

memcmploop:
	ld a, (de)
	cp (hl)
	jr nz, memcmpmismatch
	inc de
	inc hl
	djnz memcmploop
	ld l, #0
	ret
memcmpmismatch:
	ld l, #1
	ret

; b = len, hl = ptr_a, de = ptr_b, stack = retaddr
; returns nz for mismatch, destroys b, de, hl
asm_memcmp:
	ld a, (de)
	cp (hl)
	ret nz
	inc de
	inc hl
	djnz asm_memcmp
	ret

; stack: retaddr, ptr_a, ptr_b, len_a, len_b
; is b in a?, return value in l
strinstr_ixstore:
    .word 0
_strinstr:
    ld (#strinstr_ixstore), ix
    pop af ; retaddr
    pop de ; ptr_a
    pop ix ; ptr_b
    pop hl ; len_a
    pop bc ; len_b
    push bc
    push hl
    push ix
    push de
    push af ; stack restored
    ld a, c
    or a
    jr z, strinstr_found
    sbc hl, bc ; hl = indices to check
    jr c, strinstr_notfound ; len_b > len_a
strinstr_loop:
    push hl
    push de
    ld b, c ; b = how much to check (len_b)
    push ix
    pop hl ; hl, de = strings to compare, c = len, stack: indices to check
    call asm_memcmp ; returns z for match
    pop de ; ptr_a
    pop hl ; indices to check
    jr z, strinstr_found
    inc de ; move forward in ptr_a
    ld a, h
    or l
    jr z, strinstr_notfound
    dec hl
    jp strinstr_loop
strinstr_found:
    ld l, #1
    ld ix, (#strinstr_ixstore)
    ret
strinstr_notfound:
    ld l, #0
    ld ix, (#strinstr_ixstore)
    ret

; unsigned char parse_cmdline(char *f) __z88dk_fastcall
_parse_cmdline:
	ex de,hl
	ld c, #0
	ld hl, (_cmdline)
	ld a, h
	or l
	jr z, cmdline_done
	ld b, #200 ; 2x100
cmdline_loop:
	ld a, (hl)
	or a ; cp #0
	jr z, cmdline_done
	cp #0xd
	jr z, cmdline_done
	cp #':'
	jr z, cmdline_done
	ldi ; also decrements bc	
	ld c, #1 ; string length > 0
	djnz cmdline_loop
cmdline_done:
	xor a
	ld (de), a
	ld l, c
	ret

;unsigned char createfilewithpath_(char * fn) __z88dk_fastcall	
; hl = filename
_createfilewithpath:
    ld iy, (_osiy)
	ld b, #0x0e; mode = 2 + 0xc, write + create new file, delete existing
	ld a, #'*'
	push hl
	rst #0x8
	.db #0x9a ; esxdos: fopen
	pop hl
	jr  c, cfwp_openfail
	ld  l, a; a = file handle
	ret
cfwp_openfail:
	ld d, h
	ld e, l
cfwp_loop:
	ld a, (hl)
	or a
	jr z, cfwp_pathdone
	cp #'/'
    jr nz, cfwp_notslash
	ld (hl), #0xff ; replace slash with path terminator
	ex de,hl
	push de
	push hl
    ld a, #0x02    ; make path
    exx            ; place parameters in alternates
    ld de, #0x01b1 ; IDE_PATH
    ld c, #7       ; "usually 7, but 0 for some calls"
    rst #0x8
    .db #0x94      ; +3dos call
	pop hl
	pop de
	ex de,hl
	ld (hl), #'/' ; restore slash
cfwp_notslash:
    inc hl
	jp cfwp_loop	
cfwp_pathdone:	
	ex de,hl
	ld b, #0x0e; mode = 2 + 0xc, write + create new file, delete existing
	ld a, #'*'
	rst #0x8
	.db #0x9a ; esxdos: fopen
	jr  nc, cfwp_openok
	xor a
cfwp_openok:	
	ld  l, a; a = file handle
	ret

_endof_esxdos:	