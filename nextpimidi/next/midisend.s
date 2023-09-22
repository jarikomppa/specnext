    DEVICE ZXSPECTRUMNEXT
; midisend
; nextpi midi send command
; by Jari Komppa, http://iki.fi/sol
; 2023
;
    INCLUDE nextdefs.asm
; Dot commands always start at $2000, with HL=address of command tail
; (terminated by $00, $0d or ':').
    org     0x2000

    ld a, 1
    mirror a
    nop
    nop
    cp 0x80
    jp nz, notnext

    PUSHALL

; Nextpi GPIO
; ===========
; - specnext regs 0x90-0x93 - 32 bits
; - Pins 0-1 are dead (internal to pi0)
; - Pins 2-3 I2C (-> RTC, probably doesn't work?)
; - Pins 7-11 SPI (-> SD, very likely doesn't work)
; - Pins 14-15 UART (-> term, pisend, etc)
; - Pins 18-21 I2S (-> audio, works and is used)
; - Pins 28-31 do not exist in pi0 hardware
;
; - Pins 24-27 reserved for app control
; 
; leaves..
; 
; 0x90------------------ 0x91------------------- 0x92------------------- 0x93------------------- (output enable)
; 0x98------------------ 0x99------------------- 0x9A------------------- 0x9B------------------- (i/o)
; 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
; XX XX XX XX          XX XX XX XX XX       XX XX       XX XX XX XX                   XX XX XX XX
; -dead-I2C-          -SPI-----------       -UART       -I2S-------       -AppCtl---- -no hardware
;             AA BB CC                            DD DD DD DD DD DD DD DD EE FF GG HH
;            
; A = pi-side 1 bit data counter
; B = next-side 1 bit data counter
; C = transfer direction, 0 = pi->next, 1 = next->pi. Controlled by next.
; D = 8 bits of data (overlaps with I2S, so no sound while this is going)
; E,F,G,H = App control. If a pattern of 0 0 0 0 ->  1 1 1 1 is seen, 
;           service should clean up after itself and die.
;
; AppCtl:
;  0 -> 15: quit
; 10 -> 12: move to receive mode
; 10 ->  3: move to send mode
; 10 ->  6: move to idle mode

    STORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    STORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    nextreg NEXTREG_CPU_SPEED, 3 ; 28mhz mode.
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, 32 ; set pin 5 as write access
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, 0xff ; write all GPIO bits in 0x9A
    nextreg NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, 1+2+4+8 ; set first four bits as write
   
    ld bc, outdata
parseloop:
    ld a, (hl)
    cp ' '
    jr nz, .notspace
    inc hl
    jr parseloop
.notspace:    
    cp 0
    jp z, .parsedone
    cp ':'
    jp z, .parsedone
    cp 0x0d
    jp z, .parsedone
    cp '-'
    jr nz, .notoption
    inc hl
    ld a, (hl)
    cp 'q'
    jr nz, .notquiet
    ld a, 1
    ld (quiet), A
    inc hl
    jr parseloop
.notquiet:
    cp 's'
    jr nz, .notskipsetup
    ld a, 1
    ld (skipsetup), a
    inc hl
    jr parseloop
.notskipsetup:    
    cp 'i'
    jr nz, .nogoidle
    ld a, 1
    ld (goidle), a
    inc hl
    jr parseloop
.nogoidle:
    ; unknown option
    jp printhelp
.notoption    
    cp '0'
    jp c, printhelp ; less than '0'.. must be error
    cp '9' + 1
    jp c, .parsenumber
    cp 'A'
    jp c, printhelp ; not 0..9, less than 'A'.. must be error
    cp 'Z' + 1
    jr c, .parseintvar
    cp 'a'
    jp c, printhelp ; not A-Z, less than 'a', error
    cp 'z' + 1
    jr c, .parseintvar
    jp printhelp ; Not eol, not space, not -, not 0..9, not a..z, not A..Z .. error
.parseintvar:
    ld a, (hl)
    or 'a'-'A' ; if it happens to be upper case, now it's lower
    sub 'a'
    call readintvar
    ld a, d
    ld (bc), a
    inc bc
    inc hl
    jp parseloop
.parsenumber:
    ld de, 0
.nextdigit:
    sla d ; *2
    ld a, d
    sla a ; *4
    sla a ; *8
    add a, d ; *8 + *2 = *10
    add a, (hl)
    sub a, '0'
    ld d, a
    inc hl
    ld a, (hl)
    cp '0'
    jr c, .numberdone ; below 0
    cp '9' + 1
    jr c, .nextdigit ; 0..9
.numberdone:
    ld a, d
    ld (bc), a
    inc bc
    jp parseloop
.parsedone:

    ld a, (skipsetup)
    cp 1
    jr z, nogpiosetup

    ; switch to "send midi" mode
    nextreg NEXTREG_PI_GPIO_3, 10
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 3
nogpiosetup:

    ; Read current send counter
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a
    and 32
    ld (flipflop), a
    nextreg NEXTREG_PI_GPIO_0, a

    ld a, (skipsetup)
    cp 1
    jr z, noinitialsend

    push hl ; enough stuff on stack for server failure rewind
    ld a, 'M'
    call write_gpio
    ld a, 'I'
    call write_gpio
    ld a, 'D'
    call write_gpio
    pop hl
noinitialsend:


    ld hl, outdata
payload:
    ld a, (hl)
    cp 0xff
    jr z, cleanup
    call write_gpio
    inc hl
    jr payload
cleanup:

    ld a, (goidle)
    cp a, 1
    jr nz, skipgoidle
    ; switch to "idle" mode
    nextreg NEXTREG_PI_GPIO_3, 10
    halt
    halt
    nextreg NEXTREG_PI_GPIO_3, 6
skipgoidle:

    RESTORENEXTREG NEXTREG_CPU_SPEED, store_NEXTREG_CPU_SPEED
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_0, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_2, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2
    RESTORENEXTREG NEXTREG_PI_GPIO_OUTPUT_ENABLE_3, store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3

    POPALL
    ret

printhelp:
    ld hl, helptext
    call printmsg
    jp cleanup

server_failed:
    ld hl, noserver
    call printmsg
    jp cleanup

write_gpio:
    push af
    push bc    
    ld bc, 0
.checkloop:
    call check_gpio
    jr z, .ready
    djnz .checkloop
    dec c
    jr nz, .checkloop
    pop bc
    pop af
    pop hl ; rewind stack
    jp server_failed
.ready:
    pop bc
    pop af
    nextreg NEXTREG_PI_GPIO_2, a
    ld a, (flipflop)
    xor 32 ; flip bit
    ld (flipflop), a
    nextreg NEXTREG_PI_GPIO_0, a
    ret

; returns z if data is ready
check_gpio:
    push bc
    push de
    ld e, a
    ld a, (flipflop) ; current flag
    ld d, a
    ld bc, 0x243B ; nextreg select
    ld a, NEXTREG_PI_GPIO_0
    out (c), a
    inc b         ; nextreg i/o
    in a, (c)
    sla a
    and 32
    cp d
    ld a, e
    pop de
    pop bc
    ret

; intvar in a, value in de
readintvar:
    push af
    push bc
    push hl
    ld de, 0
    ld c, a ; intvar number
    ld b, 0 ; not array
    ld hl, 0x0000 ; read (array index 0)
    exx                            ; place parameters in alternates
    ld      de, 0x01c9             ; IDE_INTEGER_VAR
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call    
    pop hl
    pop bc
    pop af
    ret

notnext:
    ld hl, notnextmsg
printmsg:
    ld a, (quiet)
    cp 1
    ret z
    ld a, (hl)
    and a, a
    ret z
    rst 16
    inc hl
    jr printmsg

notnextmsg:
    db "This does not appear to be ZX Spectrum Next.",0

noserver:
    db "The nextpi-usbmidi server does not seem to be running.\r",0
quitting:
    db "All done.\r",0
helptext:
       ;12345678901234567890123456789012
    db "MIDISEND v0.1 by Jari Komppa\r"
    db "http://iki.fi/sol\r"
    db "MIDI command sender\r\r"
    db "SYNOPSIS:\r"
    db ".midisend (command bytes)\r\r"
    db "where command bytes are\r"
    db "triplets of values and/or\r"
    db "intvars, eg.\r"
    db ".midisend 144 a 70\r"
    db "sends channel 1 note on for\r"
    db "note in intvar a at volume 70\r\r"
    db "-s to skip gpio setup\r"
    db "-i to move to idle after send\r"
    db "-q to suppress printouts\r\r"
    db 0

flipflop:
    db 32

store_NEXTREG_CPU_SPEED:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_0:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_2:
    db 0
store_NEXTREG_PI_GPIO_OUTPUT_ENABLE_3:
    db 0
quiet:
    db 0
skipsetup:
    db 0
goidle:
    db 0
outdata: ; 64 simultaneous commands ought to be enough?
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff
    db 0xff