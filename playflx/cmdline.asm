 
parsecmdline:    
    ld hl, (cmdline)
    ld a, h
    or l
    jr z, printhelp ; no commandline
    ld hl, cmdline-DOTDIFF;(cmdline)
paramloop:
    call get_sizedarg
    jr nc, printhelp ; no argument
    call check_options
    jr z, paramloop
    ret

printhelp:
    ei ; Need interrupts enabled or "scroll?" will hang
    ld hl, helptext
    call printmsg
    jp fail

helptext:
    db "PLAYFLX v0.6 by Jari Komppa\r"
    db "http://iki.fi/sol\r"
    db "Play .flx animations\r\r"
    db "SYNOPSIS:\r"
    db ".PLAYFLX [OPT] FLXFILE\r\r"
    db "OPTIONS:\r"
    db "-h --help\r"
    db "  Display this help\r"
    db "-l --loop\r"
    db "  Loop animation\r"
    db "-u --unskippable\r"
    db "  Ignore all user input\r"
    db "-k --keep\r"
    db "  Don't restore layer 2 register\r"
    db "-p --precache\r"
    db "  Fill all framebuffers at start\r"
    db "-g --game\r"
    db "  Game mode, read docs for info\r"
    db "-d --debug\r"
    db "  Show debug info\r"
    db 0
;       12345678901234567890123456789012    

; From bas2txt by Garry Lancaster
; ***************************************************************************
; * Parse an argument from the command tail                                 *
; ***************************************************************************
; Entry: (command_tail)=remaining command tail
; Exit:  Fc=0 if no argument
;        Fc=1: parsed argument has been copied to temparg and null-terminated
;        (command_tail)=command tail after this argument
;        BC=length of argument
; NOTE: BC is validated to be 0..255; if not, it does not return but instead
;       exits via show_usage.

get_sizedarg:
        ld      hl,(command_tail)
        ld      de,SCRATCH;temparg
        ld      bc,0                    ; initialise size to zero
get_sizedarg_loop:
        ld      a,(hl)
        inc     hl
        and     a
        ret     z                       ; exit with Fc=0 if $00
        cp      $0d
        ret     z                       ; or if CR
        cp      ':'
        ret     z                       ; or if ':'
        cp      ' '
        jr      z,get_sizedarg_loop     ; skip any spaces
        cp      '"'                     ; on for a quoted " arg
        jr      z,get_sizedarg_quoted   ; 
get_sizedarg_unquoted:
        ld      (de),a                  ; store next char into dest
        inc     de
        inc     c                       ; increment length
        jr      z,get_sizedarg_badsize  ; don't allow >255
        ld      a,(hl)
        and     a
        jr      z,get_sizedarg_complete ; finished if found $00
        cp      $0d
        jr      z,get_sizedarg_complete ; or CR
        cp      ':'
        jr      z,get_sizedarg_complete ; or ':'
        cp      '"'                     ; or '"' indicating start of next arg
        jr      z,get_sizedarg_complete ; 
        inc     hl
        cp      ' '
        jr      nz,get_sizedarg_unquoted; continue until space
get_sizedarg_complete:
        xor     a
        ld      (de),a                  ; terminate argument with NULL
        ld      (command_tail),hl       ; update command tail pointer
        scf                             ; Fc=1, argument found
        ret
get_sizedarg_quoted:
        ld      a,(hl)
        and     a
        jr      z,get_sizedarg_complete ; finished if found $00
        cp      $0d
        jr      z,get_sizedarg_complete ; or CR
        inc     hl
        cp      '"'                     ; if a quote ", need to check if escaped or not
        jr      z,get_sizedarg_checkendquote
        ld      (de),a                  ; store next char into dest
        inc     de
        inc     c                       ; increment length
        jr      z,get_sizedarg_badsize  ; don't allow >255
        jr      get_sizedarg_quoted
get_sizedarg_badsize:
        pop     af                      ; discard return address
        jp      printhelp
get_sizedarg_checkendquote:
        inc     c
        dec     c
        jr      z,get_sizedarg_complete ; definitely endquote if no chars yet
        dec     de
        ld      a,(de)
        inc     de
        cp      '\'                     ; was it escaped?
        jr      nz,get_sizedarg_complete; if not, was an endquote
        dec     de
        ld      a,'"'                   ; otherwise replace \ with "
        ld      (de),a
        inc     de
        jr      get_sizedarg_quoted


; From bas2txt by Garry Lancaster
; ***************************************************************************
; * Check for options                                                       *
; ***************************************************************************
; Entry: temparg contains argument, possibly option name
;        C=length of argument
; Exit:  C=length of argument (preserved if not an option)
;        Fz=1 if was an option (and has been processed)
;        Fz=0 if not an option

check_options:
        ld      a,(SCRATCH);(temparg)
        cp      '-'
        ret     nz                      ; exit with Fz=0 if not an option
        ld      hl,option_table
check_next_option:
        ld      a,(hl)
        inc     hl
        and     a
        jr      z,invalid_option        ; cause error if end of table
        cp      c
        jr      nz,skip_option          ; no match if lengths differ
        ld      b,a                     ; length to compare
        ld      de,SCRATCH;temparg
check_option_name_loop:
        ld      a,(de)
        inc     de
        cp      'A'
        jr      c,check_opt_notupper
        cp      'Z'+1
        jr      nc,check_opt_notupper
        or      $20                     ; convert uppercase to lowercase
check_opt_notupper:
        cp      (hl)
        jr      nz,option_mismatch
        inc     hl
        djnz    check_option_name_loop
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; DE=routine address
        ;pushval perform_option_end
        push    hl
        ld      hl, perform_option_end
        ex      (sp), hl
        ;/pushval
        push    de
        ret                             ; execute the option routine
perform_option_end:
        xor     a                       ; Fz=1, option was found
        ret
option_mismatch:
        ld      a,b                     ; A=remaining characters to skip
skip_option:
        add     hl, a                   ; skip the option name
        inc     hl                      ; and the routine address
        inc     hl
        jr      check_next_option

invalid_option:
        jp      printhelp


option_table:
        db opt0_a-opt0
opt0:   db "-h"
opt0_a: dw printhelp
        db opt1_a-opt1
opt1:   db "--help"
opt1_a: dw printhelp
        db opt2_a-opt2
opt2:   db "-l"
opt2_a: dw opt_loop
        db opt3_a-opt3
opt3:   db "--loop"
opt3_a: dw opt_loop
        db opt4_a-opt4
opt4:   db "-u"
opt4_a: dw opt_unskippable
        db opt5_a-opt5
opt5:   db "--unskippable"
opt5_a: dw opt_unskippable
        db opt6_a-opt6
opt6:   db "-k"
opt6_a: dw opt_keep
        db opt7_a-opt7
opt7:   db "--keep"
opt7_a: dw opt_keep
        db opt8_a-opt8
opt8:   db "-p"
opt8_a: dw opt_precache
        db opt9_a-opt9
opt9:   db "--precache"
opt9_a: dw opt_precache
        db opt10_a-opt10
opt10:  db "-g"
opt10_a:dw opt_game
        db opt11_a-opt11
opt11:  db "--game"
opt11_a:dw opt_game
        db opt12_a-opt12
opt12:  db "-d"
opt12_a:dw opt_debug
        db opt13_a-opt13
opt13:  db "--debug"
opt13_a:dw opt_debug
        ; end of table
        db 0

opt_debug:
    call initsprites
    ld hl, isr.debugcall
    ld (hl), 0xcd
    ld hl, showdebug
    ld (isr.debugcall+1), hl
    ret


opt_loop:
    ld hl, loopanim
    ld (loopjumppoint+1), hl
    ld hl, loopjumppoint
    ld (hl), 0xc3 ; jp
    ret

opt_unskippable:
    ; Turn the user animation stop jp to a ret
    ld ix, useranimationstop
    ld (ix + 0), 0xc9
    ret

opt_game:
    ; replace input handling with gamemode
    ld hl, gamemode 
    ld (isr.input_call+1), hl

    ; write out zero as keypress
    ld bc, 0x0106 ; array G
    ld hl, 0x0102 ; write index 2
    ld de, 0
    call intvar

    ; get and patch the input mode
    ld bc, 0x0106 ; array G
    ld hl, 0x0000 ; read index 0
    call intvar
    ld a, e ; input mode
    ; default input mode is qaop+space
    cp 1 ; wasd+space
    jp z, .wasd
    cp 2 ; zxpl+space
    jp z, .zxpl
    cp 3 ; kempston
    jp z, .kempston
    cp 4 ; sinclair left
    jp z, .sinclair1
    cp 5 ; sinclair right
    jp z, .sinclair2
    cp 6 ; cursor
    jp z, .cursor
    jp .inputmodedone

; qaop+space  udlrf - default
; wasd+space  uldrf
.wasd:
    ld ix, notdown0
    ld (ix+4), 0x40+8*1+7  ; bit 1 = 8*1    up
    ld (ix+16), 0x40+8*1+7 ; bit 1 = 8*1    down
    ld (ix+28), 0x40+8*0+7 ; bit 0 = 8*0    left
    ld (ix+40), 0x40+8*2+7 ; bit 2 = 8*2    right
    ld (ix+52), 0x40+8*0+7 ; bit 0 = 8*0    fire
    ld hl, keydata + 2 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 1 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 1 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 1 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 7 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone
; zxpl+space  lrudf
.zxpl:
    ld ix, notdown0
    ld (ix+4), 0x40+8*0+7  ; bit 0 = 8*0    up
    ld (ix+16), 0x40+8*1+7 ; bit 1 = 8*1    down
    ld (ix+28), 0x40+8*1+7 ; bit 1 = 8*1    left
    ld (ix+40), 0x40+8*2+7 ; bit 2 = 8*2    right
    ld (ix+52), 0x40+8*0+7 ; bit 0 = 8*0    fire
    ld hl, keydata + 5 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 6 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 0 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 0 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 7 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone
; kempston 000FUDLR (bits)
.kempston:
    ld a, 9
    ld (anykey + 4), a ; include kempston in "anykey"
    ld ix, notdown0
    ld (ix+4), 0x40+8*3+7  ; bit 3 = 8*3    up
    ld (ix+16), 0x40+8*2+7 ; bit 2 = 8*2    down
    ld (ix+28), 0x40+8*1+7 ; bit 1 = 8*1    left
    ld (ix+40), 0x40+8*0+7 ; bit 0 = 8*0    right
    ld (ix+52), 0x40+8*4+7 ; bit 4 = 8*4    fire
    ld hl, keydata + 8 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 8 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 8 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 8 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 8 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone
; sinclair left 12345 lrduf
.sinclair1:
    ld ix, notdown0
    ld (ix+4), 0x40+8*3+7  ; bit 3 = 8*3    up
    ld (ix+16), 0x40+8*2+7 ; bit 2 = 8*2    down
    ld (ix+28), 0x40+8*0+7 ; bit 0 = 8*0    left
    ld (ix+40), 0x40+8*1+7 ; bit 1 = 8*1    right
    ld (ix+52), 0x40+8*4+7 ; bit 4 = 8*4    fire
    ld hl, keydata + 3 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 3 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 3 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 3 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 3 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone
; sinclair right 67890 lrduf
.sinclair2:
    ld ix, notdown0
    ld (ix+4), 0x40+8*1+7  ; bit 1 = 8*1    up
    ld (ix+16), 0x40+8*2+7 ; bit 1 = 8*1    down
    ld (ix+28), 0x40+8*4+7 ; bit 0 = 8*0    left
    ld (ix+40), 0x40+8*3+7 ; bit 2 = 8*2    right
    ld (ix+52), 0x40+8*0+7 ; bit 0 = 8*0    fire
    ld hl, keydata + 4 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 4 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 4 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 4 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 4 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone

; cursor 56780 ldurf
.cursor:
    ld ix, notdown0
    ld (ix+4), 0x40+8*3+7  ; bit 1 = 8*1    up
    ld (ix+16), 0x40+8*4+7 ; bit 1 = 8*1    down
    ld (ix+28), 0x40+8*4+7 ; bit 0 = 8*0    left
    ld (ix+40), 0x40+8*2+7 ; bit 2 = 8*2    right
    ld (ix+52), 0x40+8*0+7 ; bit 0 = 8*0    fire
    ld hl, keydata + 4 ; up
    ld (notdown0+1), hl
    ld hl, keydata + 4 ; down
    ld (notdown1+1), hl
    ld hl, keydata + 3 ; left
    ld (notdown2+1), hl
    ld hl, keydata + 4 ; right
    ld (notdown3+1), hl
    ld hl, keydata + 4 ; fire
    ld (notdown4+1), hl
    jp .inputmodedone

.inputmodedone:    

    ; get and patch the valid inputs
    ld bc, 0x0106 ; array G
    ld hl, 0x0001 ; read index 1
    call intvar
    bit 0, e
    jr z, .input1ok
    ld ix, notdown0
    ld (ix+5), 0x18
.input1ok:
    bit 1, e
    jr z, .input2ok
    ld ix, notdown1
    ld (ix+5), 0x18
.input2ok:
    bit 2, e
    jr z, .input3ok
    ld ix, notdown2
    ld (ix+5), 0x18
.input3ok:
    bit 3, e
    jr z, .input4ok
    ld ix, notdown3
    ld (ix+5), 0x18
.input4ok:
    bit 4, e
    jr z, .input5ok
    ld ix, notdown4
    ld (ix+5), 0x18
.input5ok:
    ret

opt_keep:
; C2EA 3A FB C3    >        ld a, (regstore + 12)
; C2ED ED 92 12    >        nextreg NEXTREG_LAYER2_RAMPAGE, a
    ld ix, restore_layer2_rampage
    ld (ix+3), 0
    ld (ix+4), 0
    ld (ix+5), 0
    ret

opt_precache:
    ld hl, startanim
    ld (hl), 0 
    ret

command_tail:
    dw cmdline-DOTDIFF
