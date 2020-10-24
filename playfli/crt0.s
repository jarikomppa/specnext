		.module crt0
		.globl _heap
		.globl _cmdline
		.globl _scr_x
		.globl _scr_y
		.globl _dbg
		.globl _osiy

		.area _HEADER(ABS)
_crt0_entry:	
		di

        ld (_cmdline-0x2000),hl 
        ld (_osiy-0x2000), iy

		;; store all regs
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
		ld (#store_sp-0x2000),sp		; store SP

        ld      hl, #0x0001 ; alloc zx memory
        exx                             ; place parameters in alternates
        ld      de, #0x01bd             ; IDE_BANK
        ld      c, #7                   ; "usually 7, but 0 for some calls"
        rst     #0x8
        .db     #0x94                   ; +3dos call
    	jp      nc, allocfail
        ld      (_pagehandle2-0x2000),de

        ld      hl, #0x0001 ; alloc zx memory
        exx                             ; place parameters in alternates
        ld      de, #0x01bd             ; IDE_BANK
        ld      c, #7                   ; "usually 7, but 0 for some calls"
        rst     #0x8
        .db     #0x94                   ; +3dos call
    	jr      nc, allocfail
        ld      (_pagehandle4-0x2000),de

    	ld	    a,      #0x54 ; nextreg 
        ld      bc,     #0x243B   ; nextreg select
        out     (c),    a
        inc     b                 ; nextreg i/o
        in      a,      (c)
        ld      (_mmu4-0x2000), a
        ld      a, (_pagehandle4-0x2000)
        out     (c),     a

    	ld	    a,      #0x52 ; nextreg 
        ld      bc,     #0x243B   ; nextreg select
        out     (c),    a
        inc     b                 ; nextreg i/o
        in      a,      (c)
        ld      (_mmu2-0x2000), a
        ld      a, (_pagehandle2-0x2000)
        out     (c),     a

		ld sp, #0x9fff ; for mmu4

    ; set up dot command error handler - if we get an error,
    ; do the crt0 cleanup (this is pretty insufficient in this case,
    ; should actually go to main()::cleanup label..
    ld      hl, #shutdown
	rst     #0x8
	.db     #0x95

        ld de, #0x4000 ; destination
        ld hl, #0x2000 ; source
        ld bc, #0x2000 ; count
        ldir ; copy

		;; start the os
		call _main

shutdown:
    	ld	    a,      #0x54 ; nextreg
        ld      bc,     #0x243B   ; nextreg select
        out     (c),    a
        inc     b                 ; nextreg i/o
        ld      a, (_mmu4-0x2000)
        out     (c),     a

    	ld	    a,      #0x52 ; nextreg
        ld      bc,     #0x243B   ; nextreg select
        out     (c),    a
        inc     b                 ; nextreg i/o
        ld      a, (_mmu2-0x2000)
        out     (c),     a

    	ld	    de, (_pagehandle2-0x2000)      ; page
        ld      hl, #0x0003             ; free zx memory
        exx                             ; place parameters in alternates
        ld      de, #0x01bd             ; IDE_BANK
        ld      c, #7                   ; "usually 7, but 0 for some calls"
        rst     #0x8
        .db     #0x94                   ; +3dos call

    	ld	    de, (_pagehandle4-0x2000)      ; page
        ld      hl, #0x0003             ; free zx memory
        exx                             ; place parameters in alternates
        ld      de, #0x01bd             ; IDE_BANK
        ld      c, #7                   ; "usually 7, but 0 for some calls"
        rst     #0x8
        .db     #0x94                   ; +3dos call

allocfail:	
		ld sp,(#store_sp-0x2000)		; restore original SP
		;; restore all regs
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
		ret	
store_sp:	.word 252
_cmdline:   .word 0
_pagehandle4: .word 0
_pagehandle2: .word 0
_osiy: .word 0
_mmu4: .db 0
_mmu2: .db 0
_scr_x: .db 0
_scr_y: .db 0
_dbg: .db 0

_endof_crt0:
		;;	(linker documentation:) where specific ordering is desired - 
		;;	the first linker input file should have the area definitions 
		;;	in the desired order
		.area _HOME
		.area _CODE
	        .area _GSINIT
	        .area _GSFINAL	
		.area _DATA
	        .area _BSS
	        .area _HEAP

		;;	this area contains data initialization code -
		;;	unlike gnu toolchain which generates data, sdcc generates 
		;;	initialization code for every initialized global 
		;;	variable. and it puts this code into _GSINIT area
        	.area _GSINIT        	
gsinit:	
        	.area _GSFINAL
        	ret

		.area _DATA

		.area _BSS

		.area _HEAP
_heap::
