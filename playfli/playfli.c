/*
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

#define HWIF_IMPLEMENTATION
#include "../common/hwif.c"

extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fseek(unsigned char handle, unsigned long ofs);
extern void makepath(char *pathspec); // must be 0xff terminated!
extern void conprint(char *txt) __z88dk_fastcall;

extern unsigned char allocpage();
extern void freepage(unsigned char page);

extern void memcpy(char *dest, const char *source, unsigned short count);
extern void memset(char *dest, const short value, unsigned short count);

extern volatile short framecounter;
extern void setupisr7();
extern void closeisr7();
extern void setupisr0();
extern void di();
extern void ei();


void dma_memcpy(char *dest, const char *source, unsigned short count)
{   
    if (!count) return;
    if (count < 20)
    {
        memcpy(dest, source, count);
        return;
    }
    PORT_DATAGEAR_DMA = 0x83; // DMA_DISABLE
    PORT_DATAGEAR_DMA = 0b01111101; // R0-Transfer mode, A -> B, write adress + block length
    PORT_DATAGEAR_DMA = (unsigned short)source & 0xff;
    PORT_DATAGEAR_DMA = ((unsigned short)source >> 8) & 0xff;
    PORT_DATAGEAR_DMA = count & 0xff;
    PORT_DATAGEAR_DMA = (count >> 8) & 0xff;
    PORT_DATAGEAR_DMA = 0b01010100; // R1-write A time byte, increment, to memory, bitmask
    PORT_DATAGEAR_DMA = 0b00000010; // 2t
    PORT_DATAGEAR_DMA = 0b01010000; // R2-write B time byte, increment, to memory, bitmask
    PORT_DATAGEAR_DMA = 0b00000010; // R2-Cycle length port B
    PORT_DATAGEAR_DMA = 0b10101101; // R4-Continuous mode (use this for block transfer), write dest adress
    PORT_DATAGEAR_DMA = (unsigned short)dest & 0xff;
    PORT_DATAGEAR_DMA = ((unsigned short)dest >> 8) & 0xff;
    PORT_DATAGEAR_DMA = 0b10000010; // R5-Restart on end of block, RDY active LOW
    PORT_DATAGEAR_DMA = 0b11001111; // R6-Load
    PORT_DATAGEAR_DMA = 0x87;       // R6-Enable DMA
}

void dma_memset(char *dest, const short value, unsigned short count)
{
    if (!count) return;
    *dest = value;
    dma_memcpy(dest+1, dest, count-1);
}

extern char *cmdline;

unsigned char readnextreg(char reg)
{
    PORT_NEXTREG_SELECT = reg;
    return PORT_NEXTREG_IO;
}

#define writenextreg(REG, VAL) { PORT_NEXTREG_SELECT = (REG); PORT_NEXTREG_IO = (VAL); }


typedef struct FLIHEADER_ 
{ 
    unsigned long mFliSize; // Should be same as file size.
    short int mFliMagic; // 0AF12h
    short int mFliFrames; // Frames in flic, 4000 max
    short int mWidth; // x size
    short int mHeight;// y size
    short int mDepth; // bits per pixel, always 8 in fli/flc
    short int mFlags; // Bitmapped flags. 0=ring frame, 1=header updated
    short int mSpeed; // Delay between frames in ms (or retraces :))
    short int mReserved1;
    unsigned long        mCreated; // MS-dos date of creation
    unsigned long        mCreator; // SerNo of animator, 0464c4942h for FlicLib
    unsigned long        mUpdated; // MS-dos date of last modify
    unsigned long        mUpdater;
    short int mAspectx; // x-axis aspect ratio (320x200: 6)
    short int mAspecty; // y-axis aspect ratio (320x200: 5)
    char      mReserved2[38];
    unsigned long        mOframe1; // offset to frame 1 
    unsigned long        mOframe2; // offset to frame 2 - for looping, jump here
    char      mReserved3[42];
} FLIHEADER;

// Frame header
typedef struct  FLIFRAMEHEADER_
{
    unsigned long int       mFramesize;
    unsigned short int mMagic;
    short int mChunks;
    unsigned char mReserved[8];
} FLIFRAMEHEADER;

// Chunk header
typedef struct FLICHUNKHEADER_ 
{
    unsigned long int       mSize;
    short int mType;
} FLICHUNKHEADER;

// 0x0000 mmu0 = rom
// 0x2000 mmu1 = dot program
// 0x4000 mmu2 = 
// 0x6000 mmu3 = 8k framebuffer
// 0x8000 mmu4 = stack, nextreg store & plenty of free space
// 0xa000 mmu5 = scratch
// 0xc000 mmu6 = scratch
// 0xe000 mmu7 = isr trampoline + empty space up to 0xfe00

// isr needs trampoline in mmu7. Can't put stack there, 
// because ROM calls want data in the middle 32k. So, no idea
// what I'm going to use the rest of the space for, for now..
// I guess I could edit my isr code to use mmu4 and drop the 
// stack a bit. I'll cross that bridge if I need to.

__at 0x6000 unsigned char fb[8192];
__at 0xa000 unsigned char scratch[16384];
__at 0x8000 unsigned char regstate[256];

void isr()
{
}

void main()
{   
    FLIHEADER hdr; 
    FLIFRAMEHEADER framehdr;
    FLICHUNKHEADER chunkhdr;
    unsigned short frames;
    char mmu5p, mmu6p, mmu7p;
    char f;

#define SAVEREG(x) regstate[x] = readnextreg(x)
#define RESTOREREG(x) writenextreg(x, regstate[x])
       
    SAVEREG(NEXTREG_CPU_SPEED);
    SAVEREG(NEXTREG_MMU3);
    SAVEREG(NEXTREG_MMU5);
    SAVEREG(NEXTREG_MMU6);
    SAVEREG(NEXTREG_MMU7);
    SAVEREG(NEXTREG_DISPLAY_CONTROL_1);
    SAVEREG(NEXTREG_LAYER2_CONTROL);
    SAVEREG(NEXTREG_GENERAL_TRANSPARENCY);
    SAVEREG(NEXTREG_TRANSPARENCY_COLOR_FALLBACK);
    SAVEREG(NEXTREG_ENHANCED_ULA_CONTROL);
    SAVEREG(NEXTREG_ENHANCED_ULA_INK_COLOR_MASK);
    SAVEREG(NEXTREG_ULA_CONTROL);
    
    writenextreg(NEXTREG_CPU_SPEED, 3); // 28MHz
    mmu5p = allocpage();
    mmu6p = allocpage();
    mmu7p = allocpage();
    writenextreg(NEXTREG_MMU5, mmu5p);
    writenextreg(NEXTREG_MMU6, mmu6p);
    writenextreg(NEXTREG_MMU7, mmu7p);

    //f = fopen("/testfli/ba_hvy.flc", 1); // open existing for read
    //f = fopen("/testfli/ba.flc", 1); // open existing for read
    f = fopen("/testfli/ba_small.flc", 1); // open existing for read
    //f = fopen("/testfli/cube.flc", 1); // open existing for read
    if (f == 0)
    {
        conprint("Can't open file\r");
        goto cleanup;
    }
    fread(f, (unsigned char*)&hdr, sizeof(hdr));
    if (hdr.mFliMagic != 0xAF12)
    {
        conprint("Magic not ok, this isn't a fli file?\r");
        fclose(f);
        return;
    }
//    conprint("Magic ok\r");
      
    PORT_NEXTREG_SELECT = NEXTREG_DISPLAY_CONTROL_1;
    PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // enable layer 2
    
    PORT_NEXTREG_SELECT = NEXTREG_LAYER2_CONTROL;
    PORT_NEXTREG_IO = 0; // 256x192 resolution, palette offset 0

    // Set transparent color to 0   
    PORT_NEXTREG_SELECT = NEXTREG_GENERAL_TRANSPARENCY;
    PORT_NEXTREG_IO = 0;
    // Set fallback color to 0
    PORT_NEXTREG_SELECT = NEXTREG_TRANSPARENCY_COLOR_FALLBACK;
    PORT_NEXTREG_IO = 0;

    // select layer2 palette 0    
    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_CONTROL;
    PORT_NEXTREG_IO = 0x10 | 1; // enable ulanext
    
    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_INK_COLOR_MASK;
    PORT_NEXTREG_IO = 0xff; // ulanext color mask
    
    writenextreg(NEXTREG_ULA_CONTROL, 0x80); // disable ULA

    { // cls
        short int numlines;
        unsigned char *vbuffptr;
                
        for (numlines = 0; numlines < 192; numlines++)
        {
            writenextreg(NEXTREG_MMU3, 18 + (numlines >> 5)); // one 8k bank eats 32 scanlines
            vbuffptr = fb + ((numlines & 31) << 8);
            memset(vbuffptr, 0, 256);
        }
    }
    
    setupisr7(); // Write trampoline to mmu7
    //ei();

loopanim:    
    frames = hdr.mFliFrames;
    while (frames)
    {
        writenextreg(NEXTREG_MMU3, 18);
        *((unsigned short*)fb) = frames & 0x0707;
        
        while (framecounter < 2) {};
        framecounter-=2;
        
        frames--;
        fread(f, (unsigned char*)&framehdr, sizeof(framehdr));
        if (framehdr.mMagic == 0xf1fa)
        {
//            conprint("frame magic ok\r");
            for (unsigned short numchunks = 1; numchunks <= framehdr.mChunks; numchunks++)
            {
                fread(f, (unsigned char*)&chunkhdr, sizeof(chunkhdr));
                chunkhdr.mSize -= sizeof(chunkhdr);
                if (chunkhdr.mSize < 16384) // skip huge chunks for now
                {
                    fread(f, scratch, chunkhdr.mSize);
                    chunkhdr.mSize = 0;
                    switch (chunkhdr.mType) 
                    {                    
                    case  4: // 256-level palette (FLC only)
                        {
                            unsigned short numberpk;
                            unsigned char *data;
                            unsigned char set, skip;
                            numberpk = *(unsigned short*)&scratch[0];
                            data = (unsigned char*)&scratch[2];
                            for (unsigned short packetcount = 0; packetcount < numberpk; packetcount++)
                            {
                                skip = *data;
                                data++;
                                set = *data;
                                data++;
                                if (set == 0) 
                                {
                                    PORT_NEXTREG_SELECT = NEXTREG_PALETTE_INDEX;
                                    PORT_NEXTREG_IO = 0;    
                                    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION;    
                                    for (unsigned short i = 0; i < 768; i += 3)
                                    {
                                        unsigned short c;
                                        c = (*data & 0xe0) << 1;
                                        data++;
                                        c |= (*data & 0xe0) >> 2;
                                        data++;
                                        c |= (*data & 0xe0) >> 5;
                                        data++;
                                        PORT_NEXTREG_IO = c >> 1;
                                        PORT_NEXTREG_IO = c & 1;
                                    }                                
                                } 
                                else 
                                {
                                    PORT_NEXTREG_SELECT = NEXTREG_PALETTE_INDEX;
                                    PORT_NEXTREG_IO = skip;
                                    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION;
                                    set += set + set; // set *= 3
                                    for (unsigned short i = 0; i < set; i += 3)
                                    {
                                        unsigned short c;
                                        c = (*data & 0xe0) << 1;
                                        data++;
                                        c |= (*data & 0xe0) >> 2;
                                        data++;
                                        c |= (*data & 0xe0) >> 5;
                                        data++;
                                        PORT_NEXTREG_IO = c >> 1;
                                        PORT_NEXTREG_IO = c & 1;
                                    }                                
                                }
                            }                        
                        }
                        //conprint("8 bit pal done\r");
                        break;
                    case 11: // 64-level palette
                        {
                            unsigned short numberpk;
                            unsigned char *data;
                            unsigned char set, skip;
                            numberpk = *(unsigned short*)&scratch[0];
                            data = (unsigned char*)&scratch[2];
                            for (unsigned short packetcount = 0; packetcount < numberpk; packetcount++)
                            {
                                skip = *data;
                                data++;
                                set = *data;
                                data++;
                                if (set == 0) 
                                {
                                    PORT_NEXTREG_SELECT = NEXTREG_PALETTE_INDEX;
                                    PORT_NEXTREG_IO = 0;    
                                    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION;    
                                    for (unsigned short i = 0; i < 768; i += 3)
                                    {
                                        unsigned short c;
                                        c = (*data & 0x38) << 3;
                                        data++;
                                        c |= (*data & 0x38) >> 0;
                                        data++;
                                        c |= (*data & 0x38) >> 3;
                                        data++;
                                        PORT_NEXTREG_IO = c >> 1;
                                        PORT_NEXTREG_IO = c & 1;
                                    }                                
                                } 
                                else 
                                {
                                    PORT_NEXTREG_SELECT = NEXTREG_PALETTE_INDEX;
                                    PORT_NEXTREG_IO = skip;
                                    PORT_NEXTREG_SELECT = NEXTREG_ENHANCED_ULA_PALETTE_EXTENSION;
                                    set += set + set; // set *= 3
                                    for (unsigned short i = 0; i < set; i += 3)
                                    {
                                        unsigned short c;
                                        c = (*data & 0x38) << 3;
                                        data++;
                                        c |= (*data & 0x38) >> 0;
                                        data++;
                                        c |= (*data & 0x38) >> 3;
                                        data++;
                                        PORT_NEXTREG_IO = c >> 1;
                                        PORT_NEXTREG_IO = c & 1;
                                    }                                
                                }
                            }                        
                        }
                        break;
                    case 15: // RLE full frame
                        {
                            
                            short int numlines;
                            unsigned char *vbuffptr;
                            unsigned char pktcount, numpkt;
                            //signed short sizecount;
                            signed char size;
                            unsigned char *data;
                            data = (unsigned char*)&scratch[0];
                            
                            for (numlines = 0; numlines < hdr.mHeight; numlines++)
                            {
                                writenextreg(NEXTREG_MMU3, 18 + (numlines >> 5)); // one 8k bank eats 32 scanlines
                                vbuffptr = fb + ((numlines & 31) << 8);
                                numpkt = *data;
                                data++;
                                for (pktcount = 0; pktcount < numpkt; pktcount++)
                                {
                                    size = (signed char)*data;
                                    data++;
                                    if (size >= 0)
                                    {
                                        dma_memset(vbuffptr, *data, size);
                                        vbuffptr += size;
                                        data++;
                                    } else {
                                        size = -size;
                                        dma_memcpy(vbuffptr, data, size);
                                        data += size;
                                        vbuffptr += size;
                                    }
                                }
                            }
                            
                        }
                        break;

                    case 12: // 8b-based delta
                        {
                            short int *addlines;
                            short int maxline;
                            short int startline;
                            unsigned char *vbuffptr;
                            short int linecount;
                            unsigned char pktcount, skip, numpkt, databyte;
                            //signed short sizecount;
                            signed char size;
                            unsigned char *data;
                            data = (unsigned char*)&scratch[0];
                            
                            addlines = (short int *)data;
                            startline = *addlines;
                            data += 4;
                            addlines += 1;
                            maxline = *addlines + startline;
                            for (linecount = startline; linecount < maxline; linecount++)
                            {
                                writenextreg(NEXTREG_MMU3, 18 + (linecount >> 5)); // one 8k bank eats 32 scanlines
                                vbuffptr = fb + ((linecount & 31) << 8);
                                numpkt = *data;
                                data++;
                                for (pktcount = 0; pktcount < numpkt; pktcount++)
                                {
                                    skip = *data;
                                    data++;
                                    vbuffptr += skip;
                                    size = (signed char)*data;
                                    data++;
                                    if (size >= 0) 
                                    {
                                        dma_memcpy(vbuffptr, data, size);
                                        vbuffptr += size;
                                        data += size;
                                    } else {
                                        size = -size;
                                        databyte = *data;
                                        data++;
                                        dma_memset(vbuffptr, databyte, size);
                                        vbuffptr += size;
                                    }
                                }
                            }
                        }
                        break;
    /*
                    case  7: // 16b-based delta (FLC only)
    //                    conprint("16b delta\r");
                        //ifli_ss2(aFli, chunkdata);
                        break;
                    case 13: // full black frame
    //                    conprint("black frame\r");
                        //ifli_black(aFli);
                        break;
                    case 16: // full frame, no compression
    //                    conprint("uncompressed frame\r");
                        //ifli_copy(aFli, chunkdata);
                        break;
                    case 18: // postage stamp sized image - out of this routine's scope
                        break;
                    default: // unknown/irrelevant
    //                    conprint("unknown\r");
                        break;
    */                    
                    }
                }
                while (chunkhdr.mSize > 8192)
                {
                    fread(f, scratch, 8192);
                }
                if (chunkhdr.mSize)
                {
                    fread(f, scratch, chunkhdr.mSize);
                }
            }
        }
    }
//    fseek(f, hdr.mOframe2);
//    if (hdr.mOframe2)
//        goto loopanim;

//    conprint("That's all\r");
cleanup:    
    di();
    closeisr7();
    fclose(f);
 
    RESTOREREG(NEXTREG_CPU_SPEED);
    RESTOREREG(NEXTREG_MMU3);
    RESTOREREG(NEXTREG_MMU5);
    RESTOREREG(NEXTREG_MMU6);
    RESTOREREG(NEXTREG_MMU7);
    RESTOREREG(NEXTREG_DISPLAY_CONTROL_1);
    RESTOREREG(NEXTREG_LAYER2_CONTROL);
    RESTOREREG(NEXTREG_GENERAL_TRANSPARENCY);
    RESTOREREG(NEXTREG_TRANSPARENCY_COLOR_FALLBACK);
    RESTOREREG(NEXTREG_ENHANCED_ULA_CONTROL);
    RESTOREREG(NEXTREG_ENHANCED_ULA_INK_COLOR_MASK);
    RESTOREREG(NEXTREG_ULA_CONTROL); 
 
    freepage(mmu5p);
    freepage(mmu6p);
    freepage(mmu7p);
}