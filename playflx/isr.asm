; setup isr in mmu 7
; de = isr routine
setupisr7::
    push de
    ld  de,     0xfe00 ; im2 vector table start
    ld  hl,     0xfdfd ; where interrupt will point at
    ld  a,      d 
    ld  i,      a        ; interrupt will hop to 0xfe?? where ?? is random 
    ld  a,      l        ; we need 257 copies of the address
.rep:
    ld  (de),   a 
    inc e
    jr  nz,     .rep
    inc d          ; just one more
    ld  (de),   a
    ; isr table built, now a short trampoline to jump to our routine
    pop de
    ld  (hl),   0xc3 ; 0xc3 = JP
    inc hl
    ld  (hl),   e
    inc hl
    ld  (hl),   d
    im  2           ; set the interrupt mode (remember to change back before you exit)
    ret

closeisr7::
    im  1
    ret
