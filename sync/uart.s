	.module uart
	.globl _checksum
	.globl _receive
    .globl _flush_uart
    .globl _flush_uart_hard
	.area _CODE

; Read from UART until empty. No paramters.
_flush_uart:
    ld bc, #0x133b ; UART_TX
    in a, (c) ; on read, bottom bit of UART_TX says "data available"
	rrca
	ret	nc
    inc b  ; UART_RX
	in	a, (c) ; read and discard
	jr _flush_uart

; Read from UART until it's been silent for n milliseconds. No parameters
_flush_uart_hard:
    ld bc, #0x133b ; UART_TX
    ld hl, #10000   ; Busy loop is ~12+4+12+6+4+4+12=54 clocks, 10k@28MHz = ~20ms
fuh_checkinput:
    in a, (c)
    rrca
    jr nc, fuh_loop
    inc b   ; UART_RX
    in a, (c)
    jr _flush_uart_hard
fuh_loop:
    dec hl
    ld a, h
    or a, l
    jr nz, fuh_checkinput    
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
    
; Optimized inner loop snippet from Ped7g from SpectrumNext discord    
;; IN: HL = memory buffer, BC = size (0..1024) (65535 is real max)
;; OUT: E = xor[buffer], D = sum{intermmediate xors}
;;     HL = HL + size, BC = 0
checksum_block:
    ld      de,#0       ; clear checksum
    ; check size > 0 and swap B<->C
    ld      a,b
    ld      b,c
    ld      c,a
    inc     c
    inc     b
    djnz    loop       ; for non-zero low byte enter the main loop (C is ready too)
    jr      loop_entry ; no partial loop, only 256x blocks, so --C first
loop:
    ld      a,e
    xor     (hl)
    inc     hl
    ld      e,a
    add     a,d
    ld      d,a
    djnz    loop
loop_entry:
    dec     c
    jr      nz, loop
; /snippet

    ld c, (hl)      ; Load the checksums from after the data
    inc hl
    ld b, (hl)
    ld h, b
    ld l, c
    or a
    sbc hl, de
    ld a, h
    or l
    ld l, a
    
    ret    
	
_endof_uart: