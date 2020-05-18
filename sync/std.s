	.module crt0
	.globl _memcpy
	.area _CODE

; extern void memcpy(char *dest, const char *source, unsigned short count)
_memcpy::
	pop	iy	; return
	pop	de	; destination
	pop	hl	; source
	pop	bc	; count
	push	bc
	push	hl
	push	de
	ld	a,b
	or	c
	jr	Z, empty_memcpy	
	ldir
empty_memcpy:
	jp	(iy)
