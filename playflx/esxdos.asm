;extern unsigned char allocpage()
; output e = page, nc = fail
allocpage:
    ld      hl, 0x0001 ; alloc zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
	ret

;extern unsigned char reservepage(unsigned char page)
; e = page, output e = page, nc = fail
reservepage:
    ld      hl, 0x0002 ; reserve zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
	ret

;extern void freepage(unsigned char page)
; e = page
freepage:
    ld      hl, 0x0003 ; free zx memory
    exx                             ; place parameters in alternates
    ld      de, 0x01bd             ; IDE_BANK
    ld      c, 7                   ; "usually 7, but 0 for some calls"
    rst     0x8
    .db     0x94                   ; +3dos call
	ret

; hl = filename
; b = mode
;       esx_mode_read           $01    request read access
;       esx_mode_write          $02    request write access
;       esx_mode_use_header     $40    read/write +3DOS header
;                plus one of:
;       esx_mode_open_exist     $00    only open existing file
;       esx_mode_open_creat     $08    open existing or create file
;       esx_mode_creat_noexist  $04    create new file, error if exists
;       esx_mode_creat_trunc    $0c    create new file, delete existing
; output: a = handle, carry = failure
fopen:
	ld  a,  '*'
	rst     0x8
	.db     0x9a
    ret

;extern void fclose(unsigned char handle);
; a = handle
fclose:
    rst     0x8
    .db     0x9b
    ret

;extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
; a = handle
; hl = buf
; bc = bytes
; output: bc = bytes
fread:
    rst     0x8
    .db     0x9d
	ret

;extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
; a = handle
; bc = bytes
; hl = buf
fwrite:
    rst     0x8
    .db     0x9e
	ret

;extern void fseek(unsigned char handle, unsigned long ofs);
; a = handle
; offset = BCDE
; mode = l (0 = set, 1 = fwd, 2 = bwd)
fseek:
    rst     0x8
    .db     0x9f
	ret
