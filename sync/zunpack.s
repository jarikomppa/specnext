	.module zunpack
	.globl _zunpack
	.area _CODE

;nextsync.c:450: unsigned short zunpack(unsigned short len, unsigned char *dp, unsigned char *scratch, unsigned char filehandle)
;	---------------------------------
; Function zunpack
; ---------------------------------
_zunpack::
	push	ix
	ld	ix,#0
	add	ix,sp

;	push	af
;	push	af
;nextsync.c:452: unsigned short p = 0;
;	ld	hl, #0x0000
;	ex	(sp), hl
;nextsync.c:453: unsigned short received = 0;
;	xor	a, a
;	ld	-2 (ix), a
;	ld	-1 (ix), a
    
    ld hl, #0
    push hl
    push hl

;nextsync.c:461: while (p < len)
mainloop:
	ld	a, -4 (ix)
	sub	a, 4 (ix)
	ld	a, -3 (ix)
	sbc	a, 5 (ix)
	jp	NC, done
;nextsync.c:463: switch (unpackstate)
	ld	iy, #_unpackstate
	ld	a, 0 (iy)
	or	a, a
	jr	Z,read_sizebyte
	ld	a, 0 (iy)
	dec	a
	jr	Z,copy_literals
	ld	a, 0 (iy)
	sub	a, #0x02
	jp	Z,read_offsetbyte
;	ld	a, 0 (iy)
;	sub	a, #0x03
;	jp	Z,copy_history
;	jp	check_output
    jp copy_history
;nextsync.c:465: case 0: // size byte
read_sizebyte:
;nextsync.c:466: unpacksize = dp[p];
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
;nextsync.c:467: if ((unpacksize & 0xc0) == 0xc0)
	ld	hl, (_unpacksize)
	ld	a, l
	and	a, #0xc0
	ld	c, a
	ld	b, #0x00
	ld	a, c
	sub	a, #0xc0
	or	a, b
	jr	NZ,set_state_2
;nextsync.c:469: unpacksize &= 0x3f;
	ld	a, l
	and	a, #0x3f
	ld	l, a
	ld	h, #0x00
	ld	(_unpacksize), hl
;nextsync.c:470: unpackstate = 1;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x01
	jr	inc_p
set_state_2:
;nextsync.c:474: unpackstate = 2;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x02
inc_p:
;nextsync.c:476: p++;
	inc	-4 (ix)
	jp	NZ,check_output
	inc	-3 (ix)
;nextsync.c:477: break;
	jp	check_output
;nextsync.c:479: while (p < len && unpackd < 1024 && unpacksize >= 0)
copy_literals:
	pop	bc
	push	bc
copy_literals_loop:
;nextsync.c:467: if ((unpacksize & 0xc0) == 0xc0)
	ld	hl, (_unpacksize)
;nextsync.c:479: while (p < len && unpackd < 1024 && unpacksize >= 0)
	ld	a, h
	rlca
	and	a,#0x01
	ld	e, a
	ld	a, c
	sub	a, 4 (ix)
	ld	a, b
	sbc	a, 5 (ix)
	jr	NC,copy_literals_done
	ld	hl, (_unpackd)
	ld	a, h
	sub	a, #0x04
	jr	NC,copy_literals_done
	bit	0, e
	jr	NZ,copy_literals_done
;nextsync.c:481: scratch[unpackd] = dp[p];
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
;nextsync.c:482: p++;
	inc	bc
;nextsync.c:483: unpackd++;
	ld	hl, (_unpackd)
	inc	hl
	ld	(_unpackd), hl
;nextsync.c:484: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
	jr	copy_literals_loop
copy_literals_done:
	inc	sp
	inc	sp
	push	bc
;nextsync.c:486: if (unpacksize < 0)
	ld	a, e
	or	a, a
	jp	Z, check_output
;nextsync.c:488: unpackstate = 0;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x00
;nextsync.c:490: break;
	jp	check_output
;nextsync.c:491: case 2: // offset byte
read_offsetbyte:
;nextsync.c:492: unpackoffset = -256 | (signed char)dp[p];
	ld	a, 6 (ix)
	add	a, -4 (ix)
	ld	l, a
	ld	a, 7 (ix)
	adc	a, -3 (ix)
	ld	h, a
	ld	l, (hl)
	ld	a, l
	rla
	sbc	a, a
	or	a, #0xff
	ld	h, a
	ld	(_unpackoffset), hl
;nextsync.c:493: unpackoffset = (unpackd + unpackoffset + 1024) & 1023;
	ld	bc, (_unpackoffset)
	ld	hl, (_unpackd)
	add	hl, bc
	ld	a, h
	add	a, #0x04
	and	a, #0x03
	ld	h, a
	ld	(_unpackoffset), hl
;nextsync.c:494: unpackstate = 3;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x03
;nextsync.c:495: unpacksize += 3;
	ld	hl, (_unpacksize)
	inc	hl
	inc	hl
	inc	hl
	ld	(_unpacksize), hl
;nextsync.c:496: p++;
	inc	-4 (ix)
	jr	NZ,check_output
	inc	-3 (ix)
;nextsync.c:497: break;
	jr	check_output
;nextsync.c:499: while (unpackd < 1024 && unpacksize >= 0)
copy_history:
	ld	bc, (_unpackd)
;nextsync.c:467: if ((unpacksize & 0xc0) == 0xc0)
	ld	hl, (_unpacksize)
;nextsync.c:479: while (p < len && unpackd < 1024 && unpacksize >= 0)
	ld	a, h
	rlca
	and	a,#0x01
	ld	e, a
;nextsync.c:499: while (unpackd < 1024 && unpacksize >= 0)
	ld	a, b
	sub	a, #0x04
	jr	NC,copy_history_done
	bit	0, e
	jr	NZ,copy_history_done
;nextsync.c:501: scratch[unpackd] = scratch[unpackoffset];
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
;nextsync.c:502: unpackd++;
	ld	hl, (_unpackd)
	inc	hl
	ld	(_unpackd), hl
;nextsync.c:503: unpackoffset++;
	ld	hl, (_unpackoffset)
	inc	hl
;nextsync.c:504: unpackoffset &= 1023;
	ld	(_unpackoffset), hl
	ld	a, h
	and	a, #0x03
	ld	h, a
	ld	(_unpackoffset), hl
;nextsync.c:505: unpacksize--;
	ld	hl, (_unpacksize)
	dec	hl
	ld	(_unpacksize), hl
	jr	copy_history
copy_history_done:
;nextsync.c:507: if (unpacksize < 0)
	ld	a, e
	or	a, a
	jr	Z,check_output
;nextsync.c:509: unpackstate = 0;
	ld	hl,#_unpackstate + 0
	ld	(hl), #0x00
;nextsync.c:512: }
check_output:
;nextsync.c:514: if (unpackd == 1024 || p == len)
	ld	hl, (_unpackd)
	ld	a, l
	or	a, a
	jr	NZ,unpackd_not_1024
	ld	a, h
	sub	a, #0x04
	jr	Z,unpackd_is_1024
unpackd_not_1024:
	ld	a, 4 (ix)
	sub	a, -4 (ix)
	jp	NZ,mainloop
	ld	a, 5 (ix)
	sub	a, -3 (ix)
	jp	NZ,mainloop
unpackd_is_1024:
;nextsync.c:516: fwrite(filehandle, scratch, unpackd);
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
;nextsync.c:517: received += unpackd;
	ld	a, -2 (ix)
	ld	hl, #_unpackd
	add	a, (hl)
	ld	-2 (ix), a
	ld	a, -1 (ix)
	inc	hl
	adc	a, (hl)
	ld	-1 (ix), a
;nextsync.c:518: unpackd = 0;
	ld	hl, #0x0000
	ld	(_unpackd), hl
	jp	mainloop
done:
;nextsync.c:521: return received;
	pop	bc
	pop	hl
	push	hl
	push	bc
;nextsync.c:522: }
	ld	sp, ix
	pop	ix
	ret

_endof_zunpack: