	.module esxdos
	.globl _fopen
	.globl _fclose
	.globl _fread
	.globl _fwrite
	.globl _writenextreg
	.globl _readnextreg
	.globl _allocpage
	.globl _freepage
	.globl _makepath
	.globl _conprint
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
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    hl
	push    af

	ld	    a,      4 (ix) ; reg
	ld      l,      5 (ix) ; val

    push    bc
    ld      bc,     #0x243B   ; nextreg select
    out     (c),    a
    inc     b                 ; nextreg i/o
	ld      a,      l
    out     (c),    a
    pop     bc    

    pop af
    pop hl
	pop ix
    ret


;extern unsigned char readnextreg(unsigned char reg);
_readnextreg::
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    af

	ld	    a,      4 (ix) ; reg

    push    bc
    ld      bc,     #0x243B   ; nextreg select
    out     (c),    a
    inc     b                 ; nextreg i/o
    in      a,      (c)
    ld      l,      a
    pop     bc    

    pop af
	pop ix
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
_makepath::
    pop de  ; return address
    pop hl  ; char *pathspec
    push hl ; restore stack
    push de
    ld iy, (_osiy)
   
    ld a, #0x02 ; make path
    exx                             ; place parameters in alternates
    ld      de, #0x01b1             ; IDE_PATH
    ld      c, #7                   ; "usually 7, but 0 for some calls"
    rst     #0x8
    .db     #0x94                   ; +3dos call

	ret
    

;extern void conprint(char *txt) __z88dk_fastcall;
_conprint:
    ld iy, (_osiy)
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jp _conprint
_endof_esxdos:	