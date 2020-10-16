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
// mmu6/7 = scratch

__at 0xc000 unsigned char scratch[16384];

void main()
{   
    FLIHEADER hdr; 
    FLIFRAMEHEADER framehdr;
    FLICHUNKHEADER chunkhdr;
    char nextreg7, mmu6, mmu7, mmu6p, mmu7p;
    char f;
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz
    mmu6 = readnextreg(NEXTREG_MMU6);
    mmu7 = readnextreg(NEXTREG_MMU7);
    mmu6p = allocpage();
    mmu7p = allocpage();
    writenextreg(NEXTREG_MMU6, mmu6p);
    writenextreg(NEXTREG_MMU7, mmu7p);

    f = fopen("/testfli/ba.flc", 1); // open existing for read
    fread(f, (unsigned char*)&hdr, sizeof(hdr));
    if (hdr.mFliMagic != 0xAF12)
    {
        conprint("Magic not ok, this isn't a fli file?\r");
        fclose(f);
        return;
    }
    conprint("Magic ok\r");
    
    ei(); // let keyboard work
    
    while(hdr.mFliFrames)
    {
        hdr.mFliFrames--;
        fread(f, (unsigned char*)&framehdr, sizeof(framehdr));
        if (framehdr.mMagic == 0xf1fa)
        {
            conprint("frame magic ok\r");
            for (unsigned short numchunks = 1; numchunks <= framehdr.mChunks; numchunks++)
            {
                fread(f, (unsigned char*)&chunkhdr, sizeof(chunkhdr));
                switch (chunkhdr.mType) 
                {
                case  4: // 256-level palette (FLC only)
                    conprint("8 bit pal\r");
                    //ifli_colour256(aFli, chunkdata); // TODO
                    break;
                case  7: // 16b-based delta (FLC only)
                    conprint("16b delta\r");
                    //ifli_ss2(aFli, chunkdata);
                    break;
                case 11: // 64-level palette
                    conprint("6b palette\r");
                    //ifli_color(aFli, chunkdata);
                    break;
                case 12: // 8b-based delta
                    conprint("8b delta\r"); 
                    //ifli_rc(aFli, chunkdata); // TODO
                    break;
                case 13: // full black frame
                    conprint("black frame\r");
                    //ifli_black(aFli);
                    break;
                case 15: // RLE full frame
                    conprint("full RLE frame\r"); // TODO
                    //ifli_brun(aFli, chunkdata);
                    break;
                case 16: // full frame, no compression
                    conprint("uncompressed frame\r");
                    //ifli_copy(aFli, chunkdata);
                    break;
                case 18: // postage stamp sized image - out of this routine's scope
                    break;
                default: // unknown/irrelevant
                    conprint("unknown\r");
                    break;
                }
                chunkhdr.mSize -= sizeof(chunkhdr);
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
    writenextreg(NEXTREG_MMU6, mmu6);
    writenextreg(NEXTREG_MMU7, mmu7);
    freepage(mmu6p);
    freepage(mmu7p);
}