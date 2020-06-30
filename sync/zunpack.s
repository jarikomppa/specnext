	.module zunpack
	.globl _zunpack
	.area _CODE

;nextsync.c:450: unsigned short zunpack(unsigned short len, unsigned char *dp, unsigned char *scratch, unsigned char filehandle)
;	---------------------------------
; Function zunpack
; ---------------------------------
;
; af bc de hl ix iy
;
; filehandle = 10
; scratch    = 8, 9
; dp         = 6, 7
; len        = 4, 5
; retaddr    = 2, 3
; ix         = 1, 0
; received   = -1, -2
; p          = bc
;
; generated by sdcc:
; -3, -4 = temp for unpackoffset?
; -5, -6 = len masked with 1024?
; -7, -8 = copy of len ?
; 

_zunpack::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
;nextsync.c:454: unsigned short p = 0;
	ld	bc, #0x0000
;nextsync.c:455: unsigned short received = 0;
	xor	a, a
	ld	-2 (ix), a
	ld	-1 (ix), a
;nextsync.c:464: while (p < len)
	ld	a, 4 (ix)
	ld	-8 (ix), a
	ld	a, 5 (ix)
	ld	-7 (ix), a
	ld	-6 (ix), #0x00
	ld	a, -7 (ix)
	and	a, #0x04
	ld	-5 (ix), a
mainloop:
	ld	a, c
	sub	a, -8 (ix)
	ld	a, b
	sbc	a, -7 (ix)
	jp	NC, done
;nextsync.c:466: switch (unpackstate)
	ld	iy, #_unpackstate
	ld	a, 0 (iy)
	or	a, a
	jr	Z,read_size_byte
	ld	a, 0 (iy)
	dec	a
	jr	Z,copy_literals
	ld	a, 0 (iy)
	sub	a, #0x02
	jp	Z,read_offset_byte
	ld	a, 0 (iy)
	sub	a, #0x03
	jp	Z,copy_history
	jp	check_write_out
;nextsync.c:468: case 0: // size byte
read_size_byte:
;nextsync.c:469: unpacksize = dp[p];
	ld	l, 6 (ix)
	ld	h, 7 (ix)
	add	hl, bc
	ld	a, (hl)
	ld	(_unpacksize+0), a
	xor	a, a
	ld	(_unpacksize+1), a
;nextsync.c:470: p++;
	inc	bc
;nextsync.c:471: if ((unpacksize & 0xc0) != 0xc0)
	ld	hl, (_unpacksize)
	ld	a, l
	and	a, #0xc0
	ld	e, a
	ld	d, #0x00
	ld	a, e
	sub	a, #0xc0
	or	a, d
	jr	Z,found_literal_count
;nextsync.c:473: unpackstate = 2;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x02
;nextsync.c:474: goto mainloop;
	jr	mainloop
found_literal_count:
;nextsync.c:476: unpacksize &= 0x3f;
	ld	a, l
	and	a, #0x3f
	ld	l, a
	ld	h, #0x00
	ld	(_unpacksize), hl
;nextsync.c:477: unpackstate = 1;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x01
;nextsync.c:480: while (p < len && !(unpackd & 1024) && unpacksize >= 0)
copy_literals:
;nextsync.c:471: if ((unpacksize & 0xc0) != 0xc0)
	ld	hl, (_unpacksize)
;nextsync.c:480: while (p < len && !(unpackd & 1024) && unpacksize >= 0)
	ld	a, h
	rlca
	and	a,#0x01
	ld	e, a
	ld	a, c
	sub	a, -8 (ix)
	ld	a, b
	sbc	a, -7 (ix)
	jr	NC,done_copy_literals
	ld	hl, (_unpackd)
	bit	2, h
	jr	NZ,done_copy_literals
	bit	0, e
	jr	NZ,done_copy_literals
;nextsync.c:482: scratch[unpackd] = dp[p];
	ld	a, 8 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	e, a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	d, a
	ld	l, 6 (ix)
	ld	h, 7 (ix)
	add	hl, bc
	ld	a, (hl)
	ld	(de), a
;nextsync.c:483: p++;
	inc	bc
;nextsync.c:484: unpackd++;
	ld	hl, (_unpackd)
	inc	hl
	ld	(_unpackd), hl
;nextsync.c:485: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
	jr	copy_literals
done_copy_literals:
;nextsync.c:487: if (unpacksize < 0)
	ld	a, e
	or	a, a
	jp	Z, check_write_out
;nextsync.c:489: unpackstate = 0;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x00
;nextsync.c:491: break;
	jp	check_write_out
;nextsync.c:492: case 2: // offset byte
read_offset_byte:
;nextsync.c:493: unpackoffset = ((signed short)-256) | (signed char)dp[p];
	ld	l, 6 (ix)
	ld	h, 7 (ix)
	add	hl, bc
	ld	e, (hl)
	ld	a, e
	rla
	sbc	a, a
	ld	hl,#_unpackoffset + 0
	ld	(hl), e
	or	a, #0xff
	ld	(_unpackoffset+1), a
;nextsync.c:494: p++;
	inc	bc
;nextsync.c:495: unpackoffset = (unpackd + unpackoffset + 1024) & 1023;
	ld	de, (_unpackoffset)
	ld	iy, #_unpackd
	ld	a, 0 (iy)
	add	a, e
	ld	e, a
	ld	a, 1 (iy)
	adc	a, d
	ld	d, a
	ld	hl, #0x0400
	add	hl, de
	ld	a, h
	and	a, #0x03
	ld	h, a
	ld	(_unpackoffset), hl
;nextsync.c:496: unpackstate = 3;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x03
;nextsync.c:497: unpacksize += 3;
	ld	hl, (_unpacksize)
	inc	hl
	inc	hl
	inc	hl
	ld	(_unpacksize), hl
;nextsync.c:500: while (!(unpackd & 1024) && unpacksize >= 0)
copy_history:
	ld	de, (_unpackd)
;nextsync.c:471: if ((unpacksize & 0xc0) != 0xc0)
	ld	hl, (_unpacksize)
;nextsync.c:480: while (p < len && !(unpackd & 1024) && unpacksize >= 0)
	ld	a, h
	rlca
	and	a,#0x01
;nextsync.c:500: while (!(unpackd & 1024) && unpacksize >= 0)
	bit	2, d
	jr	NZ,done_copy_history
	bit	0, a
	jr	NZ,done_copy_history
;nextsync.c:502: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
;nextsync.c:503: scratch[unpackd] = scratch[unpackoffset];
	ld	a, 8 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	e, a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	d, a
	ld	a, 8 (ix)
	ld	hl, #_unpackoffset
	add	a, (hl)
	ld	-4 (ix), a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	-3 (ix), a
	ld	l, -4 (ix)
	ld	h, -3 (ix)
	ld	a, (hl)
	ld	(de), a
;nextsync.c:504: unpackd++;
	ld	hl, (_unpackd)
	inc	hl
	ld	(_unpackd), hl
;nextsync.c:505: unpackoffset++;
	ld	hl, (_unpackoffset)
	inc	hl
;nextsync.c:506: unpackoffset &= 1023;
	ld	(_unpackoffset), hl
	ld	a, h
	and	a, #0x03
	ld	h, a
	ld	(_unpackoffset), hl
	jr	copy_history
done_copy_history:
;nextsync.c:508: if (unpacksize < 0)
	or	a, a
	jr	Z,check_write_out
;nextsync.c:510: unpackstate = 0;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x00
;nextsync.c:513: }
check_write_out:
;nextsync.c:515: if (unpackd & 1024 || (!(len & 1024) && p == len))
	ld	hl, (_unpackd)
	bit	2, h
	jr	NZ,do_write_out
	ld	a, -5 (ix)
	or	a, -6 (ix)
	jp	NZ, mainloop
	pop	hl
	push	hl
	cp	a, a
	sbc	hl, bc
	jp	NZ,mainloop
do_write_out:
;nextsync.c:517: fwrite(filehandle, scratch, unpackd);
	push	bc
	ld	hl, (_unpackd)
	push	hl
	ld	l, 8 (ix)
	ld	h, 9 (ix)
	push	hl
	ld	a, 10 (ix)
	push	af
	inc	sp
	call	_fwrite
	pop	af
	pop	af
	inc	sp
	pop	bc
;nextsync.c:518: received += unpackd;
	ld	a, -2 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	-2 (ix), a
	ld	a, -1 (ix)
	inc	hl
	adc	a, (hl)
	ld	-1 (ix), a
;nextsync.c:519: unpackd &= 1023;
	ld	hl, (_unpackd)
	ld	a, h
	and	a, #0x03
	ld	h, a
	ld	(_unpackd), hl
	jp	mainloop
done:
;nextsync.c:522: return received;
	ld	l, -2 (ix)
	ld	h, -1 (ix)
;nextsync.c:523: }
	ld	sp, ix
	pop	ix
	ret


_endof_zunpack: