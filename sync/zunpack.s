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
; ix         = 1, 0 ?
; received   = -1, -2
; p          = -3, -4
;
_zunpack::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
;nextsync.c:454: unsigned short p = 0;
	xor	a, a
	ld	-4 (ix), a
	ld	-3 (ix), a
;nextsync.c:455: unsigned short received = 0;
	xor	a, a
	ld	-2 (ix), a
	ld	-1 (ix), a
;nextsync.c:464: while (p < len)
	ld	a, 4 (ix)
	ld	-12 (ix), a
	ld	a, 5 (ix)
	ld	-11 (ix), a
	ld	-10 (ix), #0x00
	ld	a, -11 (ix)
	and	a, #0x04
	ld	-9 (ix), a
mainloop:
	ld	a, -4 (ix)
	sub	a, -12 (ix)
	ld	a, -3 (ix)
	sbc	a, -11 (ix)
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
;	ld	a, 0 (iy)
;	sub	a, #0x03
;	jp	Z,copy_from_history
;	jp	check_write_out
    jp	copy_from_history
;nextsync.c:468: case 0: // size byte
read_size_byte:
;nextsync.c:469: unpacksize = dp[p];
	ld	a, 6 (ix)
	add	a, -4 (ix)
	ld	c, a
	ld	a, 7 (ix)
	adc	a, -3 (ix)
	ld	b, a
	ld	a, (bc)
	ld	(_unpacksize+0), a
	xor	a, a
	ld	(_unpacksize+1), a
;nextsync.c:470: p++;
	inc	-4 (ix)
	jr	NZ,p_no_overflow
	inc	-3 (ix)
p_no_overflow:
;nextsync.c:471: if ((unpacksize & 0xc0) != 0xc0)
	ld	hl, (_unpacksize)
	ld	a, l
	and	a, #0xc0
	ld	c, a
	ld	b, #0x00
	ld	a, c
	sub	a, #0xc0
	or	a, b
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
	ld	-5 (ix), a
	ld	a, -4 (ix)
	sub	a, -12 (ix)
	ld	a, -3 (ix)
	sbc	a, -11 (ix)
	jr	NC,copy_literals_done
	ld	hl, (_unpackd)
	ld	-7 (ix), l
	ld	-6 (ix), h
	bit	2, -6 (ix)
	jr	NZ,copy_literals_done
	bit	0, -5 (ix)
	jr	NZ,copy_literals_done
;nextsync.c:482: scratch[unpackd] = dp[p];
	ld	a, 8 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	c, a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	b, a
	ld	a, 6 (ix)
	add	a, -4 (ix)
	ld	e, a
	ld	a, 7 (ix)
	adc	a, -3 (ix)
	ld	d, a
	ld	a, (de)
	ld	(bc), a
;nextsync.c:483: p++;
	inc	-4 (ix)
	jr	NZ,p_no_overflow2
	inc	-3 (ix)
p_no_overflow2:
;nextsync.c:484: unpackd++;
	ld	hl, (_unpackd)
	inc	hl
	ld	(_unpackd), hl
;nextsync.c:485: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
	jr	copy_literals
copy_literals_done:
;nextsync.c:487: if (unpacksize < 0)
	ld	a, -5 (ix)
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
	ld	a, 6 (ix)
	add	a, -4 (ix)
	ld	l, a
	ld	a, 7 (ix)
	adc	a, -3 (ix)
	ld	h, a
	ld	c, (hl)
	ld	a, c
	rla
	sbc	a, a
	ld	hl,#_unpackoffset + 0
	ld	(hl), c
	or	a, #0xff
	ld	(_unpackoffset+1), a
;nextsync.c:494: p++;
	inc	-4 (ix)
	jr	NZ,p_no_overflow3
	inc	-3 (ix)
p_no_overflow3:
;nextsync.c:495: unpackoffset = (unpackd + unpackoffset + 1024) & 1023;
	ld	bc, (_unpackoffset)
	ld	hl, (_unpackd)
	add	hl, bc
	ld	a, h
	add	a, #0x04
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
copy_from_history:
	ld	bc, (_unpackd)
;nextsync.c:471: if ((unpacksize & 0xc0) != 0xc0)
	ld	hl, (_unpacksize)
;nextsync.c:480: while (p < len && !(unpackd & 1024) && unpacksize >= 0)
	ld	a, h
	rlca
	and	a,#0x01
;nextsync.c:500: while (!(unpackd & 1024) && unpacksize >= 0)
	bit	2, b
	jr	NZ,copy_from_history_done
	bit	0, a
	jr	NZ,copy_from_history_done
;nextsync.c:502: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
;nextsync.c:503: scratch[unpackd] = scratch[unpackoffset];
	ld	a, 8 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	c, a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	b, a
	ld	a, 8 (ix)
	ld	hl, #_unpackoffset
	add	a, (hl)
	ld	e, a
	ld	a, 9 (ix)
	inc	hl
	adc	a, (hl)
	ld	d, a
	ld	a, (de)
	ld	(bc), a
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
	jr	copy_from_history
copy_from_history_done:
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
	ld	a, -9 (ix)
	or	a, -10 (ix)
	jp	NZ, mainloop
	ld	a, -4 (ix)
	sub	a, -12 (ix)
	jp	NZ,mainloop
	ld	a, -3 (ix)
	sub	a, -11 (ix)
	jp	NZ,mainloop
do_write_out:
;nextsync.c:517: fwrite(filehandle, scratch, unpackd);
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
	ld	-8 (ix), l
	ld	-7 (ix), h
	ld	a, -8 (ix)
	ld	-6 (ix), a
	ld	a, -7 (ix)
	and	a, #0x03
	ld	-5 (ix), a
	ld	l, -6 (ix)
	ld	h, -5 (ix)
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