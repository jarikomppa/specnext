/*
 * Part of Jari Komppa's zx specnext suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

// xxxsmbbb
// where b = border color, m is mic, s is speaker
__sfr __at 0xfe PORT_254;
__sfr __at 0xfe PORT_ULA_CONTROL;

__sfr __at 0x1f PORT_KEMPSTON1;
__sfr __at 0x37 PORT_KEMPSTON2;

__sfr __banked __at 0x243B PORT_NEXTREG_SELECT;
__sfr __banked __at 0x253B PORT_NEXTREG_IO;

__sfr __banked __at 0xfefe PORT_KEYROW1;
__sfr __banked __at 0xfdfe PORT_KEYROW2;
__sfr __banked __at 0xfbfe PORT_KEYROW3;
__sfr __banked __at 0xf7fe PORT_KEYROW4;
__sfr __banked __at 0xeffe PORT_KEYROW5;
__sfr __banked __at 0xdffe PORT_KEYROW6;
__sfr __banked __at 0xbffe PORT_KEYROW7;
__sfr __banked __at 0x7ffe PORT_KEYROW8;

__sfr __banked __at 0x103b PORT_I2C_CLOCK;
__sfr __banked __at 0x113b PORT_I2C_DATA;

__sfr __banked __at 0x123b PORT_LAYER2_ACCESS;

__sfr __banked __at 0x133b PORT_UART_TX;
__sfr __banked __at 0x143b PORT_UART_RX;
__sfr __banked __at 0x153b PORT_UART_CONTROL;

__sfr __banked __at 0x1ffd PORT_PLUS3_MEMORY_PAGING;

__sfr __banked __at 0x303b PORT_SPRITE_STATUS;
__sfr __banked __at 0x303b PORT_SPRITE_SLOT_SELECT;

__sfr __banked __at 0x7ffd PORT_MEMORY_PAGING_CONTROL;
__sfr __banked __at 0xdffd PORT_MEMORY_BANK_SELECT;

__sfr __banked __at 0xfadf PORT_KEMPSTON_MOUSE_BUTTONS;
__sfr __banked __at 0xfbdf PORT_KEMPSTON_MOUSE_X;
__sfr __banked __at 0xffdf PORT_KEMPSTON_MOUSE_Y;

__sfr __banked __at 0xbffd PORT_SOUND_CHIP_REGWRITE;
__sfr __banked __at 0xfffd PORT_TURBOSOUND_NEXT_CONTROL;

__sfr __at 0x0b PORT_MB02_DMA;
__sfr __at 0x57 PORT_SPRITE_ATTRIBUTE_UPLOAD;
__sfr __at 0x5b PORT_SPRITE_PATTERN_UPLOAD;
__sfr __at 0x6b PORT_DATAGEAR_DMA;
__sfr __at 0xdf PORT_SPECDRUM_DAC;
__sfr __at 0xff PORT_TIMEX_VIDEO_MODE_CONTROL;

enum NEXTREGS
{
    NEXTREG_MACHINE_ID = 0x00,
    NEXTREG_CORE_VERSION = 0x01,
    NEXTREG_NEXT_RESET = 0x02,
    NEXTREG_MACHINE_TYPE = 0x03,
    NEXTREG_CONFIG_MAP = 0x04,
    NEXTREG_PERIPHERAL1 = 0x05,
    NEXTREG_PERIPHERAL2 = 0x06,
    NEXTREG_CPU_SPEED = 0x07,
    NEXTREG_PERIPHERAL3 = 0x08,
    NEXTREG_PERIPHERAL4 = 0x09,
    NEXTREG_PERIPHERAL5 = 0x0a,
    NEXTREG_CORE_VERSION_MINOR = 0x0e,
    NEXTREG_ANTIBRICK = 0x10,
    NEXTREG_VIDEO_TIMING = 0x11,
    NEXTREG_LAYER2_RAMPAGE = 0x12,
    NEXTREG_LAYER2_RAMSHADOWPAGE = 0x13,
    NEXTREG_GENERAL_TRANSPARENCY = 0x14,
    NEXTREG_SPRITE_AND_LAYERS = 0x15,
    NEXTREG_LAYER2_X = 0x16,
    NEXTREG_LAYER2_Y = 0x17,
    NEXTREG_CLIP_LAYER2 = 0x18,
    NEXTREG_CLIP_SPRITES = 0x19,
    NEXTREG_CLIP_ULA = 0x1a,
    NEXTREG_CLIP_TILEMAP = 0x1b,
    NEXTREG_CLIP_CONTROL = 0x1c,
    NEXTREG_VIDEOLINE_MSB = 0x1e,
    NEXTREG_VIDEOLINE_LSB = 0x1f,
    NEXTREG_VIDEOLINE_INTERRUPT_CONTROL = 0x22,
    NEXTREG_VIDEOLINE_INTERRUPT_VALUE = 0x23,
    NEXTREG_ULA_X_OFFSET = 0x26,
    NEXTREG_ULA_Y_OFFSET = 0x27,
    NEXTREG_KEYMAP_HIGH_ADDRESS = 0x28,
    NEXTREG_KEYMAP_LOW_ADDRESS = 0x29,
    NEXTREG_KEYMAP_HIGH_DATA = 0x2a,
    NEXTREG_KEYMAP_LOW_DATA = 0x2b,
    NEXTREG_DAC_B_MIRROR = 0x2c,
    NEXTREG_DAC_AD_MIRROR = 0x2d,
    NEXTREG_DAC_C_MIRROR = 0x2e,
    NEXTREG_TILEMAP_OFFSET_X_MSB = 0x2f,
    NEXTREG_TILEMAP_OFFSET_X_LSB = 0x30,
    NEXTREG_TILEMAP_OFFSET_Y = 0x31,
    NEXTREG_LORES_X_OFFSET = 0x32,
    NEXTREG_LORES_Y_OFFSET = 0x33,
    NEXTREG_SPRITE_PORT_MIRROR_INDEX = 0x34,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_0 = 0x35,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_1 = 0x36,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_2 = 0x37,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_3 = 0x38,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_4 = 0x39,
    NEXTREG_PALETTE_INDEX = 0x40,
    NEXTREG_PALETTE_VALUE = 0x41,
    NEXTREG_ENHANCED_ULA_INK_COLOR_MASK = 0x42,
    NEXTREG_ENHANCED_ULA_CONTROL = 0x43,
    NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION = 0x44,
    NEXTREG_TRANSPARENCY_COLOR_FALLBACK = 0x4a,
    NEXTREG_SPRITES_TRANSPARENCY_INDEX = 0x4b,
    NEXTREG_TILEMAP_TRANSPARENCY_INDEX = 0x4c,
    NEXTREG_MMU0 = 0x50,
    NEXTREG_MMU1 = 0x51,
    NEXTREG_MMU2 = 0x52,
    NEXTREG_MMU3 = 0x53,
    NEXTREG_MMU4 = 0x54,
    NEXTREG_MMU5 = 0x55,
    NEXTREG_MMU6 = 0x56,
    NEXTREG_MMU7 = 0x57,
    NEXTREG_COPPER_DATA = 0x60,
    NEXTREG_COPPER_CONTROL_LOW = 0x61,
    NEXTREG_COPPER_CONTROL_HIGH = 0x62,
    NEXTREG_COPPER_DATA_16BIT_WRITE = 0x63,
    NEXTREG_VERTICAL_CIDEO_LINE_OFFSET = 0x64,
    NEXTREG_ULA_CONTROL = 0x68,
    NEXTREG_DISPLAY_CONTROL_1 = 0x69,
    NEXTREG_LORES_CONTROL = 0x6a,
    NEXTREG_TILEMAP_CONTROL = 0x6b,
    NEXTREG_DEFAULT_TILEMAP_ATTRIBUTE = 0x6c,
    NEXTREG_TILEMAP_BASE_ADDRESS = 0x6e,
    NEXTREG_TILE_DEFINITIONS_BASE_ADDRESS = 0x6f,
    NEXTREG_LAYER2_CONTROL = 0x70,
    NEXTREG_LAYER2_X_OFFSET_MSB = 0x71,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_0_INC = 0x75,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_1_INC = 0x76,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_2_INC = 0x77,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_3_INC = 0x78,
    NEXTREG_SPRITE_PORT_MIRROR_ATTRIBUTE_4_INC = 0x79,
    NEXTREG_USER_STORAGE_0 = 0x7f,
    NEXTREG_EXPANSION_BUS_ENABLE = 0x80,
    NEXTREG_EXPANSION_BUS_CONTROL = 0x81,
    NEXTREG_INTERNAL_PORT_DECODING_B0 = 0x82,
    NEXTREG_INTERNAL_PORT_DECODING_B8 = 0x83,
    NEXTREG_INTERNAL_PORT_DECODING_B16 = 0x84,
    NEXTREG_INTERNAL_PORT_DECODING_B24 = 0x85,
    NEXTREG_EXPANSION_PORT_DECODING_B0 = 0x86,
    NEXTREG_EXPANSION_PORT_DECODING_B8 = 0x87,
    NEXTREG_EXPANSION_PORT_DECODING_B16 = 0x88,
    NEXTREG_EXPANSION_PORT_DECODING_B24 = 0x89,
    NEXTREG_EXPANSION_PORT_BUS_IO_PROPAGETE = 0x8a,
    NEXTREG_ALTERNATE_ROM = 0x8c,
    NEXTREG_MEMORY_MAPPING = 0x8e,
    NEXTREG_PI_GPIO_OUTPUT_ENABLE_0 = 0x90,
    NEXTREG_PI_GPIO_OUTPUT_ENABLE_1 = 0x91,
    NEXTREG_PI_GPIO_OUTPUT_ENABLE_2 = 0x92,
    NEXTREG_PI_GPIO_OUTPUT_ENABLE_3 = 0x93,
    NEXTREG_PI_GPIO_0 = 0x98,
    NEXTREG_PI_GPIO_1 = 0x99,
    NEXTREG_PI_GPIO_2 = 0x9a,
    NEXTREG_PI_GPIO_3 = 0x9b,
    NEXTREG_PI_PERIPHERAL_ENABLE = 0xa0,
    NEXTREG_PI_I2S_AUDIO_CONTROL = 0xa2,    
    NEXTREG_PI_I2S_CLOCK_DIVIDE = 0xa3,
    NEXTREG_ESP_WIFI_GPIO_OUTPUT = 0xa8,
    NEXTREG_ESP_WIFI_GPIO = 0xa9,
    NEXTREG_EXTENDED_KEYS_0 = 0xb0,
    NEXTREG_EXTENDED_KEYS_1 = 0xb1,
    NEXTREG_DIVMMC_TRAP_ENABLE_1 = 0xb2,
    NEXTREG_DIVMMC_TRAP_ENABLE_2 = 0xb4    
};

// Use HWIF_IMPLEMENTATION once in project

#define MKEYBYTE(x) KEYBYTE_ ## x
#define MKEYBIT(x) KEYBIT_ ## x
#define KEYUP(x) (gKeydata[MKEYBYTE(x)] & MKEYBIT(x))
#define KEYDOWN(x) (!KEYUP(x))
#define ANYKEY() (((gKeydata[0] & gKeydata[1] & gKeydata[2] & gKeydata[3] & gKeydata[4] & gKeydata[5] & gKeydata[6] & gKeydata[7]) & 0x1f) != 0x1f)

enum KEYS
{
    KEYBIT_SHIFT = (1 << 0),
    KEYBIT_Z     = (1 << 1),
    KEYBIT_X     = (1 << 2),
    KEYBIT_C     = (1 << 3),
    KEYBIT_V     = (1 << 4),

    KEYBIT_A     = (1 << 0),
    KEYBIT_S     = (1 << 1),
    KEYBIT_D     = (1 << 2),
    KEYBIT_F     = (1 << 3),
    KEYBIT_G     = (1 << 4),

    KEYBIT_Q     = (1 << 0),
    KEYBIT_W     = (1 << 1),
    KEYBIT_E     = (1 << 2),
    KEYBIT_R     = (1 << 3),
    KEYBIT_T     = (1 << 4),

    KEYBIT_1     = (1 << 0),
    KEYBIT_2     = (1 << 1),
    KEYBIT_3     = (1 << 2),
    KEYBIT_4     = (1 << 3),
    KEYBIT_5     = (1 << 4),

    KEYBIT_0     = (1 << 0),
    KEYBIT_9     = (1 << 1),
    KEYBIT_8     = (1 << 2),
    KEYBIT_7     = (1 << 3),
    KEYBIT_6     = (1 << 4),

    KEYBIT_P     = (1 << 0),
    KEYBIT_O     = (1 << 1),
    KEYBIT_I     = (1 << 2),
    KEYBIT_U     = (1 << 3),
    KEYBIT_Y     = (1 << 4),

    KEYBIT_ENTER = (1 << 0),
    KEYBIT_L     = (1 << 1),
    KEYBIT_K     = (1 << 2),
    KEYBIT_J     = (1 << 3),
    KEYBIT_H     = (1 << 4),

    KEYBIT_SPACE = (1 << 0),
    KEYBIT_SYM   = (1 << 1),
    KEYBIT_M     = (1 << 2),
    KEYBIT_N     = (1 << 3),
    KEYBIT_B     = (1 << 4),

    KEYBIT_KEMP1L = (1 << 0),
    KEYBIT_KEMP1R = (1 << 1),
    KEYBIT_KEMP1U = (1 << 2),
    KEYBIT_KEMP1D = (1 << 3),
    KEYBIT_KEMP1A = (1 << 4),
    KEYBIT_KEMP1B = (1 << 5),

    KEYBIT_KEMP2L = (1 << 0),
    KEYBIT_KEMP2R = (1 << 1),
    KEYBIT_KEMP2U = (1 << 2),
    KEYBIT_KEMP2D = (1 << 3),
    KEYBIT_KEMP2A = (1 << 4),
    KEYBIT_KEMP2B = (1 << 5),

    KEYBYTE_SHIFT = 0,
    KEYBYTE_Z     = 0,
    KEYBYTE_X     = 0,
    KEYBYTE_C     = 0,
    KEYBYTE_V     = 0,

    KEYBYTE_A     = 1,
    KEYBYTE_S     = 1,
    KEYBYTE_D     = 1,
    KEYBYTE_F     = 1,
    KEYBYTE_G     = 1,

    KEYBYTE_Q     = 2,
    KEYBYTE_W     = 2,
    KEYBYTE_E     = 2,
    KEYBYTE_R     = 2,
    KEYBYTE_T     = 2,

    KEYBYTE_1     = 3,
    KEYBYTE_2     = 3,
    KEYBYTE_3     = 3,
    KEYBYTE_4     = 3,
    KEYBYTE_5     = 3,

    KEYBYTE_0     = 4,
    KEYBYTE_9     = 4,
    KEYBYTE_8     = 4,
    KEYBYTE_7     = 4,
    KEYBYTE_6     = 4,

    KEYBYTE_P     = 5,
    KEYBYTE_O     = 5,
    KEYBYTE_I     = 5,
    KEYBYTE_U     = 5,
    KEYBYTE_Y     = 5,

    KEYBYTE_ENTER = 6,
    KEYBYTE_L     = 6,
    KEYBYTE_K     = 6,
    KEYBYTE_J     = 6,
    KEYBYTE_H     = 6,

    KEYBYTE_SPACE = 7,
    KEYBYTE_SYM   = 7,
    KEYBYTE_M     = 7,
    KEYBYTE_N     = 7,
    KEYBYTE_B     = 7,
    
    KEYBYTE_KEMP1L = 8,
    KEYBYTE_KEMP1R = 8,
    KEYBYTE_KEMP1U = 8,
    KEYBYTE_KEMP1D = 8,
    KEYBYTE_KEMP1A = 8,
    KEYBYTE_KEMP1B = 8,

    KEYBYTE_KEMP2L = 9,
    KEYBYTE_KEMP2R = 9,
    KEYBYTE_KEMP2U = 9,
    KEYBYTE_KEMP2D = 9,
    KEYBYTE_KEMP2A = 9,
    KEYBYTE_KEMP2B = 9,
};

#ifndef HWIF_IMPLEMENTATION

extern unsigned char keydata[];
extern void do_halt();
extern void port254(const unsigned char color) __z88dk_fastcall;
extern void readkeyboard();

#else

const unsigned char gKeydata[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


void readkeyboard()
{
    *((unsigned char*)&gKeydata[0]) = PORT_KEYROW1;
    *((unsigned char*)&gKeydata[1]) = PORT_KEYROW2;
    *((unsigned char*)&gKeydata[2]) = PORT_KEYROW3;
    *((unsigned char*)&gKeydata[3]) = PORT_KEYROW4;
    *((unsigned char*)&gKeydata[4]) = PORT_KEYROW5;
    *((unsigned char*)&gKeydata[5]) = PORT_KEYROW6;
    *((unsigned char*)&gKeydata[6]) = PORT_KEYROW7;
    *((unsigned char*)&gKeydata[7]) = PORT_KEYROW8;
    *((unsigned char*)&gKeydata[8]) = PORT_KEMPSTON1 ^ 0x3f;
    *((unsigned char*)&gKeydata[9]) = PORT_KEMPSTON2 ^ 0x3f;
}

// xxxsmbbb
// where b = border color, m is mic, s is speaker
void do_port254(const unsigned char color) __z88dk_fastcall
{
    PORT_254 = color;   
}

void port254(const unsigned char color) __z88dk_fastcall
{
    do_port254(color);
}

// practically waits for retrace
void do_halt()
{
    __asm
        ei
        halt
        di
    __endasm;
}

void do_freeze()
{
    __asm
        di
        halt
    __endasm;
}

#endif