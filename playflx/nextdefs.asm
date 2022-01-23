    MACRO PRINT msg
        PUSHALL
        ld hl, .message
        call printmsg
        jr .done
.message:
            db msg,0
.done:
        POPALL
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

PORT_ULA_CONTROL EQU 0xfe
PORT_KEMPSTON1 EQU 0x1f
PORT_KEMPSTON2 EQU 0x37
PORT_NEXTREG_SELECT EQU 0x243B
PORT_NEXTREG_IO EQU 0x253B
PORT_KEYROW1 EQU 0xfefe
PORT_KEYROW2 EQU 0xfdfe
PORT_KEYROW3 EQU 0xfbfe
PORT_KEYROW4 EQU 0xf7fe
PORT_KEYROW5 EQU 0xeffe
PORT_KEYROW6 EQU 0xdffe
PORT_KEYROW7 EQU 0xbffe
PORT_KEYROW8 EQU 0x7ffe
PORT_I2C_CLOCK EQU 0x103b
PORT_I2C_DATA EQU 0x113b
PORT_LAYER2_ACCESS EQU 0x123b
PORT_UART_TX EQU 0x133b
PORT_UART_RX EQU 0x143b
PORT_UART_CONTROL EQU 0x153b
PORT_PLUS3_MEMORY_PAGING EQU 0x1ffd
PORT_SPRITE_STATUS EQU 0x303b
PORT_SPRITE_SLOT_SELECT EQU 0x303b
PORT_MEMORY_PAGING_CONTROL EQU 0x7ffd
PORT_MEMORY_BANK_SELECT EQU 0xdffd
PORT_KEMPSTON_MOUSE_BUTTONS EQU 0xfadf
PORT_KEMPSTON_MOUSE_X EQU 0xfbdf
PORT_KEMPSTON_MOUSE_Y EQU 0xffdf
PORT_SOUND_CHIP_REGWRITE EQU 0xbffd
PORT_TURBOSOUND_NEXT_CONTROL EQU 0xfffd
PORT_MB02_DMA EQU 0x0b 
PORT_SPRITE_ATTRIBUTE_UPLOAD EQU 0x57 
PORT_SPRITE_PATTERN_UPLOAD EQU 0x5b 
PORT_DATAGEAR_DMA EQU 0x6b 
PORT_SPECDRUM_DAC EQU 0xdf 
PORT_TIMEX_VIDEO_MODE_CONTROL EQU 0xff 
NEXTREG_MACHINE_ID EQU 0x00
NEXTREG_CORE_VERSION EQU 0x01
NEXTREG_NEXT_RESET EQU 0x02
NEXTREG_MACHINE_TYPE EQU 0x03
NEXTREG_CONFIG_MAP EQU 0x04
NEXTREG_PERIPHERAL1 EQU 0x05
NEXTREG_PERIPHERAL2 EQU 0x06
NEXTREG_CPU_SPEED EQU 0x07
NEXTREG_PERIPHERAL3 EQU 0x08
NEXTREG_PERIPHERAL4 EQU 0x09
NEXTREG_PERIPHERAL5 EQU 0x0a
NEXTREG_CORE_VERSION_MINOR EQU 0x0e
NEXTREG_ANTIBRICK EQU 0x10
NEXTREG_VIDEO_TIMING EQU 0x11
NEXTREG_LAYER2_RAMPAGE EQU 0x12
NEXTREG_LAYER2_RAMSHADOWPAGE EQU 0x13
NEXTREG_GENERAL_TRANSPARENCY EQU 0x14
NEXTREG_SPRITE_AND_LAYERS EQU 0x15
NEXTREG_LAYER2_X EQU 0x16
NEXTREG_LAYER2_Y EQU 0x17
NEXTREG_CLIP_LAYER2 EQU 0x18
NEXTREG_CLIP_SPRITES EQU 0x19
NEXTREG_CLIP_ULA EQU 0x1a
NEXTREG_CLIP_TILEMAP EQU 0x1b
NEXTREG_CLIP_CONTROL EQU 0x1c
NEXTREG_VIDEOLINE_MSB EQU 0x1e
NEXTREG_VIDEOLINE_LSB EQU 0x1f
NEXTREG_VIDEOLINE_INTERRUPT_CONTROL EQU 0x22
NEXTREG_VIDEOLINE_INTERRUPT_VALUE EQU 0x23
NEXTREG_ULA_X_OFFSET EQU 0x26
NEXTREG_ULA_Y_OFFSET EQU 0x27
NEXTREG_KEYMAP_HIGH_ADDRESS EQU 0x28
NEXTREG_KEYMAP_LOW_ADDRESS EQU 0x29
NEXTREG_KEYMAP_HIGH_DATA EQU 0x2a
NEXTREG_KEYMAP_LOW_DATA EQU 0x2b
NEXTREG_DAC_B_MIRROR EQU 0x2c
NEXTREG_DAC_AD_MIRROR EQU 0x2d
NEXTREG_DAC_C_MIRROR EQU 0x2e
NEXTREG_TILEMAP_OFFSET_X_MSB EQU 0x2f
NEXTREG_TILEMAP_OFFSET_X_LSB EQU 0x30
NEXTREG_TILEMAP_OFFSET_Y EQU 0x31
NEXTREG_LORES_X_OFFSET EQU 0x32
NEXTREG_LORES_Y_OFFSET EQU 0x33
NEXTREG_SPRITE_PORT_MIRROR_INDEX EQU 0x34
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_0 EQU 0x35
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_1 EQU 0x36
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_2 EQU 0x37
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_3 EQU 0x38
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_4 EQU 0x39
NEXTREG_PALETTE_INDEX EQU 0x40
NEXTREG_PALETTE_VALUE EQU 0x41
NEXTREG_ENHANCED_ULA_INK_COLOR_MASK EQU 0x42
NEXTREG_ENHANCED_ULA_CONTROL EQU 0x43
NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION EQU 0x44
NEXTREG_TRANSPARENCY_COLOR_FALLBACK EQU 0x4a
NEXTREG_SPRITES_TRANSPARENCY_INDEX EQU 0x4b
NEXTREG_TILEMAP_TRANSPARENCY_INDEX EQU 0x4c
NEXTREG_MMU0 EQU 0x50
NEXTREG_MMU1 EQU 0x51
NEXTREG_MMU2 EQU 0x52
NEXTREG_MMU3 EQU 0x53
NEXTREG_MMU4 EQU 0x54
NEXTREG_MMU5 EQU 0x55
NEXTREG_MMU6 EQU 0x56
NEXTREG_MMU7 EQU 0x57
NEXTREG_COPPER_DATA EQU 0x60
NEXTREG_COPPER_CONTROL_LOW EQU 0x61
NEXTREG_COPPER_CONTROL_HIGH EQU 0x62
NEXTREG_COPPER_DATA_16BIT_WRITE EQU 0x63
NEXTREG_VERTICAL_CIDEO_LINE_OFFSET EQU 0x64
NEXTREG_ULA_CONTROL EQU 0x68
NEXTREG_DISPLAY_CONTROL_1 EQU 0x69
NEXTREG_LORES_CONTROL EQU 0x6a
NEXTREG_TILEMAP_CONTROL EQU 0x6b
NEXTREG_DEFAULT_TILEMAP_ATTRIBUTE EQU 0x6c
NEXTREG_TILEMAP_BASE_ADDRESS EQU 0x6e
NEXTREG_TILE_DEFINITIONS_BASE_ADDRESS EQU 0x6f
NEXTREG_LAYER2_CONTROL EQU 0x70
NEXTREG_LAYER2_X_OFFSET_MSB EQU 0x71
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_0_INC EQU 0x75
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_1_INC EQU 0x76
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_2_INC EQU 0x77
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_3_INC EQU 0x78
NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_4_INC EQU 0x79
NEXTREG_USER_STORAGE_0 EQU 0x7f
NEXTREG_EXPANSION_BUS_ENABLE EQU 0x80
NEXTREG_EXPANSION_BUS_CONTROL EQU 0x81
NEXTREG_INTERNAL_PORT_DECODING_B0 EQU 0x82
NEXTREG_INTERNAL_PORT_DECODING_B8 EQU 0x83
NEXTREG_INTERNAL_PORT_DECODING_B16 EQU 0x84
NEXTREG_INTERNAL_PORT_DECODING_B24 EQU 0x85
NEXTREG_EXPANSION_PORT_DECODING_B0 EQU 0x86
NEXTREG_EXPANSION_PORT_DECODING_B8 EQU 0x87
NEXTREG_EXPANSION_PORT_DECODING_B16 EQU 0x88
NEXTREG_EXPANSION_PORT_DECODING_B24 EQU 0x89
NEXTREG_EXPANSION_PORT_BUS_IO_PROPAGETE EQU 0x8a
NEXTREG_ALTERNATE_ROM EQU 0x8c
NEXTREG_MEMORY_MAPPING EQU 0x8e
NEXTREG_PI_GPIO_OUTPUT_ENABLE_0 EQU 0x90
NEXTREG_PI_GPIO_OUTPUT_ENABLE_1 EQU 0x91
NEXTREG_PI_GPIO_OUTPUT_ENABLE_2 EQU 0x92
NEXTREG_PI_GPIO_OUTPUT_ENABLE_3 EQU 0x93
NEXTREG_PI_GPIO_0 EQU 0x98
NEXTREG_PI_GPIO_1 EQU 0x99
NEXTREG_PI_GPIO_2 EQU 0x9a
NEXTREG_PI_GPIO_3 EQU 0x9b
NEXTREG_PI_PERIPHERAL_ENABLE EQU 0xa0
NEXTREG_PI_I2S_AUDIO_CONTROL EQU 0xa2
NEXTREG_PI_I2S_CLOCK_DIVIDE EQU 0xa3
NEXTREG_ESP_WIFI_GPIO_OUTPUT EQU 0xa8
NEXTREG_ESP_WIFI_GPIO EQU 0xa9
NEXTREG_EXTENDED_KEYS_0 EQU 0xb0
NEXTREG_EXTENDED_KEYS_1 EQU 0xb1
NEXTREG_DIVMMC_TRAP_ENABLE_1 EQU 0xb2
NEXTREG_DIVMMC_TRAP_ENABLE_2 EQU 0xb4    
