	.module crt0
	.globl _memcpy
	.globl _mulby10
	.area _CODE

; extern unsigned short mulby10(unsigned short input) __z88dk_fastcall
; z80n opcode for 'mul' is .db 0xed 0x30; multiply d by e, result in de
; alternate would be to shift by 3 and 1 and add them together, but
; 16 bit shifts are a bit of a pain on z80..
; input in hl, output in hl
_mulby10::
    ld d, h  ; high byte
    ld e, #10
    .db 0xed ; z80n mul
    .db 0x30
    ld h, e  ; hl is now high*10 << 8
    ld d, l  ; low byte
    ld e, #10
    .db 0xed ; z80n mul
    .db 0x30
    ld l, #0
    add hl, de  ; de has low*10, added to the high value, we have result
    ret

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

_endof_std: