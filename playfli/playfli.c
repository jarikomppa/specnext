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


// mmu4 = stack
// mmu3 = 8k framebuffer
// mmu6/7 = scratch

__at 0xc000 unsigned char scratch[16384];
__at 0x6000 unsigned char fb[8192];


void main()
{   
    FLIHEADER hdr; 
    FLIFRAMEHEADER framehdr;
    FLICHUNKHEADER chunkhdr;
    char nextreg7, mmu3, mmu6, mmu7, mmu6p, mmu7p;
    char f;
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz
    mmu3 = readnextreg(NEXTREG_MMU3);
    mmu6 = readnextreg(NEXTREG_MMU6);
    mmu7 = readnextreg(NEXTREG_MMU7);
    mmu6p = allocpage();
    mmu7p = allocpage();
    writenextreg(NEXTREG_MMU6, mmu6p);
    writenextreg(NEXTREG_MMU7, mmu7p);

    f = fopen("/testfli/ba.flc", 1); // open existing for read
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
    
//    ei(); // let keyboard work
  
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
    
    while(hdr.mFliFrames)
    {
        hdr.mFliFrames--;
        fread(f, (unsigned char*)&framehdr, sizeof(framehdr));
        if (framehdr.mMagic == 0xf1fa)
        {
//            conprint("frame magic ok\r");
            for (unsigned short numchunks = 1; numchunks <= framehdr.mChunks; numchunks++)
            {
                fread(f, (unsigned char*)&chunkhdr, sizeof(chunkhdr));
                chunkhdr.mSize -= sizeof(chunkhdr);
                if (chunkhdr.mSize < 16384) // skip huge chunks for now
                switch (chunkhdr.mType) 
                {                    
                case  4: // 256-level palette (FLC only)
                    //conprint("8 bit pal\r");
                    //ifli_colour256(aFli, chunkdata); // TODO
                    {
                        unsigned short numberpk;
                        unsigned char *data;
                        unsigned char set, skip;
                        fread(f, scratch, chunkhdr.mSize);
                        chunkhdr.mSize = 0;
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
//                    conprint("6b palette\r");
                    //ifli_color(aFli, chunkdata);
                    {
                        unsigned short numberpk;
                        unsigned char *data;
                        unsigned char set, skip;
                        fread(f, scratch, chunkhdr.mSize);
                        chunkhdr.mSize = 0;
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
                    //conprint("full RLE frame\r"); // TODO
                    //ifli_brun(aFli, chunkdata);
                    {
                        
                        short int numlines;
                        unsigned char *vbuffptr;
                        unsigned char pktcount, numpkt;
                        signed short sizecount;
                        signed char size;
                        unsigned char *data;
                        fread(f, scratch, chunkhdr.mSize);
                        chunkhdr.mSize = 0;
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
                                    for (sizecount = 0; sizecount < size; sizecount++) 
                                    {
                                        *vbuffptr = *data;
                                        vbuffptr++;
                                    }
                                    data++;
                                } else {
                                    size = -size;
                                    for (sizecount = 0; sizecount < size; sizecount++)
                                    {
                                        *vbuffptr = *data;
                                        vbuffptr++;
                                        data++;
                                    }
                                }
                            }
                        }
                        
                    }
                    break;

                case 12: // 8b-based delta
                    //conprint("8b delta\r"); 
                    //ifli_rc(aFli, chunkdata); // TODO
                    {
                        short int *addlines;
                        short int maxline;
                        short int startline;
                        unsigned char *vbuffptr;
                        short int linecount;
                        unsigned char pktcount, skip, numpkt, databyte;
                        signed short sizecount;
                        signed char size;
                        unsigned char *data;
                        fread(f, scratch, chunkhdr.mSize);
                        chunkhdr.mSize = 0;
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
                                    for (sizecount = 0; sizecount < size; sizecount++) 
                                    {
                                        *vbuffptr = *data;
                                        vbuffptr++;
                                        data++;
                                    }
                                } else {
                                    size = -size;
                                    databyte = *data;
                                    data++;
                                    for (sizecount = 0; sizecount < size; sizecount++)
                                    {
                                        *vbuffptr = databyte;
                                        vbuffptr++;
                                    }
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
                while (chunkhdr.mSize > 16384)
                {
                    fread(f, scratch, 16384);
                }
                if (chunkhdr.mSize)
                {
                    fread(f, scratch, chunkhdr.mSize);
                }
            }
        }
    }

    conprint("That's all\r");
cleanup:    
    fclose(f);
    writenextreg(0x07, nextreg7); // restore cpu speed
    writenextreg(NEXTREG_MMU3, mmu3);
    writenextreg(NEXTREG_MMU6, mmu6);
    writenextreg(NEXTREG_MMU7, mmu7);
    freepage(mmu6p);
    freepage(mmu7p);
}