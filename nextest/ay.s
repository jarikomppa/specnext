	.module ay
	.globl _setaychip
	.globl _aywrite
	.area _CODE


;extern void aywrite(unsigned char reg, unsigned char val);
_aywrite::
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    hl
	push    af

	ld	    a,      4 (ix) ; reg
	ld      l,      5 (ix) ; val

    push    bc
    ld      bc,     #0xFFFD   ; turbo sound next control, if top bit is 0, rest is reg
    out     (c),    a
    ld      bc,     #0xBFFD   ; sound chip register write
	ld      a,      l
    out     (c),    a
    pop     bc    

    pop af
    pop hl
	pop ix
    ret


;extern void setaychip(unsigned char val)
_setaychip::
	ld	    hl, #2+0
	add	    hl, sp
	ld	    a, (hl)  ; val
	or      #0xf8    ; set top bits to 1
	
    ld      bc,     #0xFFFD   ; turbo sound next control
    out     (c),    a
	
	ret
	