	.module isr
	.globl _setupisr0
	.globl _setupisr7
	.globl _closeisr7
	.globl _di
	.globl _ei
	.globl _framecounter
	.area _CODE

_framecounter:
    .word 0

isr::
    di
	push af
	push bc
	push de
	push hl
	push ix
	push iy
	ex af, af'
	push af
	exx
	push bc
	push de
	push hl
	
	ld hl, (_framecounter)
	inc hl
	ld (_framecounter), hl

	call _isr

	pop hl
	pop de
	pop bc
	pop af
	exx
	ex af,af'
	pop iy
	pop ix
	pop hl
	pop de
	pop bc
	pop af
    ei
    reti

;extern void setupisr0()
_setupisr0::
    ld  a, #0xc3
    ld  (0x38), a
    ld  hl, #isr
    ld  (0x39), hl
	ret

;extern void setupisr7()
_setupisr7::
    ld  de,     #0xfe00 ; im2 vector table start
    ld  hl,     #0xfdfd ; where interrupt will point at
    ld  a,      d 
    ld  i,      a        ; interrupt will hop to 0xfe?? where ?? is random 
    ld  a,      l        ; we need 257 copies of the address
rep:
    ld  (de),   a 
    inc e
    jr  nz,     rep
    inc d          ; just one more
    ld  (de),   a
    ld de, #isr
    ld  (hl),   #0xc3 ; 0xc3 = JP
    inc hl
    ld  (hl),   e
    inc hl
    ld  (hl),   d
    im  2           ; set the interrupt mode (remember to change back before you exit)
    ret

;extern void closeisr7()
_closeisr7::
    im  1
    ret

	
;extern void ei()
_ei::
    ei
    ret

;extern void di()
_di::
    di
    ret
    