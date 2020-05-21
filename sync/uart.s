	.module uart
	.globl _writeuarttx
	.globl _readuarttx
	.globl _writeuartrx
	.globl _readuartrx
	.globl _writeuartctl
	.globl _readuartctl
	.globl _setupuart
	.globl _checksum
	.area _CODE


; Snippet based on code from uart22e.asm by Tim Gilberts, Victor Trucco and Jim Bagley
;extern void setupuart(unsigned char rateindex) __z88dk_fastcall
_setupuart::
            ld a, l
			
;Now we calculate the prescaler value to set for our VGA timing.
		
			LD D,#0
			SLA A		; *2
			RL D
			SLA A		; *4
			RL D
			SLA A		; *8
			RL D
			SLA A		; *16
			RL D	
			LD E,A		
			LD HL,#BaudPrescale	; HL now points at the BAUD to use.
			ADD HL,DE

			LD BC,#9275	;Now adjust for the set Video timing.
			LD A,#17
			OUT (C),A
			LD BC,#9531
			IN A,(C)	;get timing adjustment
			LD E,A
			RLC E		;*2 guaranteed as <127
			LD D,#0
			ADD HL,DE

			LD E,(HL)
			INC HL
			LD D,(HL)
			EX DE,HL

			PUSH HL		; This is prescaler
			PUSH AF		; and value
						
			LD BC,#0x143B ;RX_SETBAUD
			LD A,L
			AND #0x7f	; Res BIT 7 to request write to lower 7 bits
			OUT (C),A
			LD A,H
			RL L		; Bit 7 in Carry
			RLA		; Now in Bit 0
			OR #0x80; Set MSB to request write to upper 7 bits
			OUT (C),A

			POP AF
			pop hl
			ret

BaudPrescale:
	.dw 243,248,256,260,269,278,286,234 ; Was 0 - 115200 adjust for 0-7
	.dw 486,496,512,521,538,556,573,469 ; 56k
	.dw 729,744,767,781,807,833,859,703 ; 38k
	.dw 896,914,943,960,992,1024,1056,864 ; 31250 (MIDI)
	.dw 1458,1488,1535,1563,1615,1667,1719,1406 ; 19200
	.dw 2917,2976,3069,3125,3229,3333,3438,2813 ; 9600
	.dw 5833,5952,6138,6250,6458,6667,6875,5625 ; 4800
	.dw 11667,11905,12277,12500,12917,13333,13750,11250 ; 2400
	.dw 122,124,128,130,135,139,143,117 ; 230400 -8
	.dw 61,62,64,65,67,69,72,59 ;460800 -9
	.dw 49,50,51,52,54,56,57,47 ;576000 -10
	.dw 30,31,32,33,34,35,36,29 ;921600 -11
	.dw 24,25,26,26,27,28,29,23 ;1152000 -12
	.dw 19,19,20,20,21,21,22,18 ;1500000 -13
	.dw 14,14,15,15,16,16,17,14 ;2000000 -14
	

;extern void writeuarttx(unsigned char val);
_writeuarttx::
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    af

	ld	    a,      4 (ix) ; val

    push    bc
    ld      bc,     #0x133b   ; uart tx
    out     (c),    a
    pop     bc    

    pop af
	pop ix
    ret

;extern unsigned char readuarttx();
_readuarttx::
	push    af

	ld	    a,      4 (ix) ; reg

    push    bc
    ld      bc,     #0x133b   ; uart tx
    in      a,      (c)
    ld      l,      a
    pop     bc    

    pop af
    ret

;extern void writeuartrx(unsigned char val);
_writeuartrx::
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    af

	ld	    a,      4 (ix) ; val

    push    bc
    ld      bc,     #0x143b   ; uart rx
    out     (c),    a
    pop     bc    

    pop af
	pop ix
    ret

;extern unsigned char readuartrx();
_readuartrx::
	push    af

	ld	    a,      4 (ix) ; reg

    push    bc
    ld      bc,     #0x143b   ; uart tx
    in      a,      (c)
    ld      l,      a
    pop     bc    

    pop af
    ret

;extern void writeuartctl(unsigned char val);
_writeuartctl::
	push	ix
	ld	    ix,     #0
	add	    ix,     sp
	push    af

	ld	    a,      4 (ix) ; val

    push    bc
    ld      bc,     #0x153b   ; uart ctl
    out     (c),    a
    pop     bc    

    pop af
	pop ix
    ret

;extern unsigned char readuartctl();
_readuartctl::
	push    af

	ld	    a,      4 (ix) ; reg

    push    bc
    ld      bc,     #0x153b   ; uart ctl
    in      a,      (c)
    ld      l,      a
    pop     bc    

    pop af
    ret

;extern unsigned short receive(char *b)
_receive::
; bc port
; hl count
; de outbuf
    pop hl  ; return address
    pop de  ; char *b
    push de ; restore stack
    push hl
    ld hl, #0 ; count
    ld bc, #0x133b   ; uart tx
nextbyte:
    in a, (c)
    and a, #0x01
    jr z, done   ; nothing incoming, done
    inc b        ; to uart rx
    in a, (c)
    ld (de), a   ; store to buffer
    and a, #0x07
    out (254), a ; blinky
    inc de       ; inc buffer idx
    inc hl       ; inc count
    dec b        ; back to tx
    jp nextbyte
done:     
    xor a
    out (254), a ; blinky
    ret        ; hl = count


;extern char checksum(char *dp, unsigned short len)
_checksum::
    pop de ; return address
    pop hl ; datapointer
    pop bc ; len
    push bc ; restore stack
    push hl
    push de
    
    ld de, #0 ; checksums
checkloop:
    ld a, b
    or a, c    
    jp z, zerocount ; if count (bc) is zero, we're done
    push bc ; ran out of regs
    
    ld a, d         ; d is the running xor checksum
    ld b, (hl)
    xor b
    ld d, a
    
    ld a, e         ; e is the sum of running xors
    add a, d
    ld e, a        
    
    pop bc
    inc hl
    dec bc
    jp checkloop
zerocount:
    ld b, (hl)      ; Load the checksums from after the data
    inc hl
    ld c, (hl)
    ld h, b
    ld l, c
    or a
    sbc hl, de
    ld a, h
    or l
    ld l, a
    
    ret
    


;	push	ix
;	ld	ix,#0
;	add	ix,sp
;;nextsync.c:416: gPort254 = 5;
;	ld	a, #0x05
;	out	(254), a
;;nextsync.c:417: checksum1 = 0;
;;nextsync.c:418: checksum2 = 0;
;	ld	bc, #0x0000
;;nextsync.c:419: for (i = 0; i < len; i++)
;	ld	de, #0x0000
;00106$:
;	ld	a, e
;	sub	a, 6 (ix)
;	ld	a, d
;	sbc	a, 7 (ix)
;	jr	NC,00101$
;;nextsync.c:421: checksum1 ^= dp[i];
;	ld	l, 4 (ix)
;	ld	h, 5 (ix)
;	add	hl, de
;	ld	a, (hl)
;	xor	a, c
;;nextsync.c:422: checksum2 += checksum1;
;	ld	c, a
;	add	a,b
;	ld	b, a
;;nextsync.c:419: for (i = 0; i < len; i++)
;	inc	de
;	jr	00106$
;00101$:
;;nextsync.c:425: gPort254 = 0;
;	ld	a, #0x00
;	out	(254), a
;;nextsync.c:426: if (checksum1 == dp[len] &&
;	ld	a, 4 (ix)
;	add	a, 6 (ix)
;	ld	l, a
;	ld	a, 5 (ix)
;	adc	a, 7 (ix)
;	ld	h, a
;	ld	e, (hl)
;	ld	a, c
;	sub	a, e
;	jr	NZ,00103$
;;nextsync.c:427: checksum2 == dp[len+1])
;	ld	e, 6 (ix)
;	ld	d, 7 (ix)
;	inc	de
;	ld	l, 4 (ix)
;	ld	h, 5 (ix)
;	add	hl, de
;	ld	c, (hl)
;	ld	a, b
;	sub	a, c
;;nextsync.c:428: return 1;
;;nextsync.c:429: return 0;
;	ld	l, #0x01
;	jr	Z,00108$
;00103$:
;	ld	l, #0x00
;00108$:
;;nextsync.c:430: }
;	pop	ix
;	ret
	
_endof_uart: