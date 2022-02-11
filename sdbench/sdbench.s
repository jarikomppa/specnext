    DEVICE ZXSPECTRUMNEXT
; sdbench
; SD card speed benchmark
; by Jari Komppa, http://iki.fi/sol
; 2022

    MACRO PRINT msg
        ld hl, .message
        call printloop
        jr .done
.message:
            db msg,0
.done:
    ENDM

    MACRO STORENEXTREG regno, addr
        ld bc, 0x243B ; nextreg select
        ld a, regno
        out (c), a
        inc b         ; nextreg i/o
        in a, (c)
        ld (addr), a
    ENDM

    MACRO RESTORENEXTREG regno, addr
        ld a, (addr)
        nextreg regno, a
    ENDM

    MACRO STORENEXTREGMASK regno, addr, mask
        ld bc, 0x243B ; nextreg select
        ld a, regno
        out (c), a
        inc b         ; nextreg i/o
        in a, (c)
        and mask
        ld (addr), a
    ENDM


    MACRO PUSHALL
		push af
		push bc
		push de
		push hl
		push ix
		push iy
		ex af, af'
		exx
		push af
		push bc
		push de
		push hl
    ENDM

    MACRO POPALL
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
    ENDM


; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     0x2000

; --------------------------------------------------
; Main

    ld a, 1
    mirror a
    nop
    nop
    cp 0x80
    jp nz, notnext
    PUSHALL
    ld (spstore), sp
    STORENEXTREGMASK 7, regstore, 3
    nextreg 7, 3 ; 28mhz mode.

    PRINT "sdbench v0.3 by Jari Komppa\rhttp://iki.fi/sol\r\rChecking for data file..\r"    
    ld hl, filename
    ld b, 1 ; open, only existing files
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN
    jp nc, filefound
    PRINT "Creating 1MB data file sdbench.dat:"

    di

    ld hl, starttime
    call gettime

    ld hl, filename
    ld b, 2 + 8 ; open for writing, create if needed
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN
    jp c, file_create_fail
    ld (filehandle), a
    
    ld b, 0
writeloop:
    push bc
    ; hl = source address. We don't care. Just write garbage.    
    ld bc, 4096 ; 4096 * 256 = 1 meg
    ld a, (filehandle)
    rst     0x8
    .db     0x9e ; F_WRITE
    pop bc
    jp c, file_write_fail
    djnz writeloop

    ld a, (filehandle)
    rst     0x8
    .db     0x9b ; F_CLOSE

    ld hl, endtime
    call gettime

    ei

    call printdiff

    ld hl, filename
    ld b, 1 ; open, only existing files
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN
    jr nc, fileok

    PRINT "Huh, can't open the data file I just created.\r"
    jp done
filefound:
    PRINT "Using existing file. To test write speed, .rm sdbench.dat\r"    

fileok:
    rst     0x8
    .db     0x9b ; F_CLOSE

    PRINT "SD delay loops (100x):"

    di

    call streaming_delays

    ei

    PRINT "Streaming 100MB. This should take about a minute:"
    
    di

    ld hl, starttime
    call gettime

    .100 call streaming_test

    ld hl, endtime
    call gettime

    ei

    call printdiff

    PRINT "fread 10MB/512B. This should take about a minute:"

    di

    ld hl, starttime
    call gettime

    .10 call freading_test

    ld hl, endtime
    call gettime

    ei

    call printdiff


done:
    ld a, (filehandle)
    rst     0x8
    .db     0x9b ; F_CLOSE
    RESTORENEXTREG 7, regstore
    ld sp, (spstore)
    POPALL
    or a
    ret

; --------------------------------------------------
; Calculate difference between starttime and endtime, print result as min:sec
printdiff:
    ld hl, (starttime + 2)
    ld e, l ; seconds
    ld hl, (endtime + 2)
    ld a, l
    
    ld bc, (starttime) ; minutes
    ld hl, (endtime)

    or a
    sbc e
    jr nc, .minutes
    add a, 60
.minutes:    
    sbc hl, bc
    jr nc, .daycycle
    ld bc, 24*60
    add hl, bc
.daycycle:    
    ld c, a

    ; (if you try to measure over 24 hours, it's your problem)

    ; now hl:c has minutes:seconds

    ld de, 10000
    or a
    sbc hl, de
    add hl, de
    jr c, .notover10k
    jr .over10k
.notover10k
    ld de, 1000
    or a
    sbc hl, de
    add hl, de
    jr c, .notover1k
    jr .over1k
.notover1k    
    ld de, 100
    or a
    sbc hl, de
    add hl, de
    jr c, .notover100
    jr .over100
.notover100
    ld de, 10
    or a
    sbc hl, de
    add hl, de
    jr c, .notover10
    jr .over10
.notover10
    jp .over1

.over10k:
    ld a, '0'-1
    ld de, 10000
    or a
.l10k:
    inc a
    sbc hl, de
    jr nc, .l10k
    add hl, de
    rst 16

.over1k:
    ld a, '0'-1
    ld de, 1000
    or a
.l1k:
    inc a
    sbc hl, de
    jr nc, .l1k
    add hl, de
    rst 16

.over100:
    ld a, '0'-1
    ld de, 100
    or a
.l100:
    inc a
    sbc hl, de
    jr nc, .l100
    add hl, de
    rst 16

.over10:
    ld a, '0'-1
    ld de, 10
    or a
.l10:
    inc a
    sbc hl, de
    jr nc, .l10
    add hl, de
    rst 16

.over1:
    ld a, '0'-1
    ld de, 1
    or a
.l1:
    inc a
    sbc hl, de
    jr nc, .l1
    add hl, de
    rst 16

    ld a, ':'
    rst 16

    ld h, 0
    ld l, c

    ld a, '0'-1
    ld de, 10
    or a
.s10:
    inc a
    sbc hl, de
    jr nc, .s10
    add hl, de
    rst 16

    ld a, '0'-1
    ld de, 1
    or a
.s1:
    inc a
    sbc hl, de
    jr nc, .s1
    add hl, de
    rst 16

    ld a, "\r"
    rst 16
    ld a, "\r"
    rst 16
    
    ret

; --------------------------------------------------
; ms-dos timestamp format. Let's ignore day. We're only a dot.
; 15-11 Hours (0-23)
; 10-5 	Minutes (0-59)
; 4-0 	Seconds/2 (0-29) 
;
; I'm pretty sure large parts of this could be done with z80n mul..
;
; in/out hl
timestamptomins:
    ; hhhhhmmm mmmsssss
    
    ld a, h
    rrca ; mhhhhhmm
    rrca ; mmhhhhhm
    rrca ; mmmhhhhh
    and 0x1f
    ld b, a ; hours
    ld a, h
    rlca ; hhhhmmmh
    rlca ; hhhmmmhh
    rlca ; hhmmmhhh
    and 0x38 ; 0b0011 1000
    ld c, a
    ld a, l
    rlca ; mmsssssm
    rlca ; msssssmm
    rlca ; sssssmmm
    and 0x07
    add c
    ld h, 0
    ld l, a ; minutes
    ld a, b ; hours
    or a
    jr z, .nohours
    ld de, 60
.loop:   
    add hl, de
    djnz .loop
.nohours:
    ret

; --------------------------------------------------
; Get time from RTC
; hl = field to write to
gettime:
    ld (.min+1), hl
    inc hl
    inc hl
    ld (.sec+1), hl
    ; Read end time from RTC
    ; de = time in ms-dos format
    ; h = seconds (ms-dos format has 2 sec accuracy)
    ; l = 100ths, not actually supported by hardware
    rst     0x8
    .db     0x8e ; M_GETDATE
    jp c, nortc
    ld l, h
    ld h, 0
.sec:ld (0), hl
    ex hl, de
    call timestamptomins
.min:ld (0), hl
    ret

; --------------------------------------------------

filemap:
    BLOCK 24, 0
cardflags:
    db 0

startstream:
    ld a, (filehandle)
    ld hl, filemap
    ld de, 3
    rst 0x8
    db 0x85 ; DISK_FILEMAP
    ld (cardflags), a
    jp nc, .streamok1 ; call failed
    PRINT "Failed to map file.\r"
    jp done
.streamok1:
    ld a, d
    or e
    jp nz, .streamok2 ; too many entries
    PRINT "Data file is too fragmented. Please run .defrag on it.\r"
    jp done
.streamok2:
    ld bc, filemap
    or a
    sbc hl, bc
    jp nz, .streamok3 ; no entries
    PRINT "File map appears to have no entries.\r"
    jp done
.streamok3:
    ld hl, filemap
    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld c, (hl)
    inc hl
    ld b, (hl) ; BCDE=card address
    inc hl
    push bc    ; (stack)DE=card address            
    ld c, (hl)
    inc hl
    ld b, (hl) ; BC=number of 512-byte blocks
    pop hl     ; HLDE=card address from dot
    ld a, (cardflags)
    or a, 0x80 ; we'll wait for the start token
    rst 0x8
    db 0x86 ; DISK_STRMSTART
    ret nc
    PRINT "Failed to start streaming.\r"
    jp done

; --------------------------------------------------

endstream:
    ld a,(cardflags)
    rst 0x8
    db 0x87 ; DISK_STRMEND
    ret

; --------------------------------------------------

streambuf:
    BLOCK 512,0

stream512:
    ld hl, streambuf
    ld c, 0xeb
.waittoken: ; wait for the next block to be ready
    in a, (c)
    inc a
    jr z, .waittoken
    .512 ini ; (hl) = (c), hl++, b--
    in a, (c) ; skip crc 1/2
    nop       ; needs nop between
    in a, (c) ; skip crc 2/2
    ret

streamdelay512:
    ld c, 0xeb
.waittoken: ; wait for the next block to be ready
    inc hl
    in a, (c)
    inc a
    jr z, .waittoken
    push hl
    ld hl, streambuf
    .512 ini ; (hl) = (c), hl++, b--
    in a, (c) ; skip crc 1/2
    nop       ; needs nop between
    in a, (c) ; skip crc 2/2
    pop hl
    ret


fread512:
    ld a, (filehandle)
    ld hl, streambuf
    ld bc, 512
    rst     0x8
    .db     0x9d ; F_READ
    ret


streaming_test:
    ld hl, filename
    ld b, 1 ; open, only existing files
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN
    jp c, general_error
    ld (filehandle), a
    call startstream

    ld b, 8
.outer:
    push bc
    ld b, 0
.inner:
    push bc
    call stream512
    pop bc
    djnz .inner
    pop bc
    djnz .outer

    call endstream
    ld a, (filehandle)
    rst     0x8
    .db     0x9b ; F_CLOSE
    ret

; --------------------------------------------------
streaming_delays:
    ld hl, filename
    ld b, 1 ; open, only existing files
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN
    jp c, general_error
    ld (filehandle), a
    call startstream

    ld hl, 0
    ld b, 10
.loop:
    push bc
    .10 call streamdelay512
    pop bc
    djnz .loop

    ld (result), hl

    call endstream
    ld a, (filehandle)
    rst     0x8
    .db     0x9b ; F_CLOSE

    ld hl, (result)

    ld de, 10000
    or a
    sbc hl, de
    add hl, de
    jr c, .notover10k
    jr .over10k
.notover10k
    ld de, 1000
    or a
    sbc hl, de
    add hl, de
    jr c, .notover1k
    jr .over1k
.notover1k    
    ld de, 100
    or a
    sbc hl, de
    add hl, de
    jr c, .notover100
    jr .over100
.notover100
    ld de, 10
    or a
    sbc hl, de
    add hl, de
    jr c, .notover10
    jr .over10
.notover10
    jp .over1

.over10k:
    ld a, '0'-1
    ld de, 10000
    or a
.l10k:
    inc a
    sbc hl, de
    jr nc, .l10k
    add hl, de
    rst 16

.over1k:
    ld a, '0'-1
    ld de, 1000
    or a
.l1k:
    inc a
    sbc hl, de
    jr nc, .l1k
    add hl, de
    rst 16

.over100:
    ld a, '0'-1
    ld de, 100
    or a
.l100:
    inc a
    sbc hl, de
    jr nc, .l100
    add hl, de
    rst 16

.over10:
    ld a, '0'-1
    ld de, 10
    or a
.l10:
    inc a
    sbc hl, de
    jr nc, .l10
    add hl, de
    rst 16

.over1:
    ld a, '0'-1
    ld de, 1
    or a
.l1:
    inc a
    sbc hl, de
    jr nc, .l1
    add hl, de
    rst 16

    ld a, "\r"
    rst 16

    ret
; --------------------------------------------------

freading_test:
    ld hl, filename
    ld b, 1 ; open, only existing files
	ld  a,  '*'
	rst     0x8
	.db     0x9a ; F_OPEN

    ld b, 8
.outer:
    push bc
    ld b, 0
.inner:
    push bc
    call fread512
    pop bc
    djnz .inner
    pop bc
    djnz .outer

    ld a, (filehandle)
    rst     0x8
    .db     0x9b ; F_CLOSE
    ret

; --------------------------------------------------

general_error:
    ld hl, generalerrmsg
    call printloop
    jp done

file_create_fail:
    ld hl, filefailmsg
    call printloop
    jp done

file_write_fail:
    ld hl, filewritefailmsg
    call printloop
    jp done

nortc:
    ld hl, rtcfailmsg
    jp printloop

printloop:
    ei
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printloop

notnext:
    ld hl, notnextmsg
    jp printloop

rtcfailmsg:
    db "RTC read failed.\r",0

notnextmsg:
    db "This does not appear to be ZX Spectrum Next.",0

filefailmsg:
    db "Unable to create data file.", 0

filewritefailmsg:
    db "Unable to write data file.", 0

generalerrmsg:
    db "This shouldn't happen.", 0

starttime:
    dw 0, 0

endtime:
    dw 0, 0

spstore:
    dw 0

result:
    dw 0

filename:
    db "sdbench.dat",0

filehandle:
    db 0

regstore:
    db 0
