/*
FliFlc (c) 1998-2020 Jari Komppa http://iki.fi/sol/
Released under Unlicense. Google it.

This is a stb-like single-header library for playing FLI / FLC animation files.

In one source file, do this:

	#define SOL_FLIFLC_IMPLEMENTATION
	#include "sol_fliflc.h"

Usage example:

	FLI *fli;
    fli = fli_open("myanim.fli");
    
    while (fli->mFrame < fli->mMaxframe)
    {
        fli_render(fli);
        ...
        output fli->mFramebuffer with fli->mPalette, fli->mXSize, fli->mYSize 
        ...
    }
    
    fli_free(fli);
	*/

#ifndef SOL_FLIFLC_H
#define SOL_FLIFLC_H
#ifdef __cplusplus
extern "C" {
#endif

// FLI player interface structure
typedef struct FLIDATA
{ 
    unsigned char * mFramebuffer; // Picture buffer.
    unsigned char * mFlicdata;    // Pointer to the (raw) fli data.
    unsigned char * mPalette;     // Pointer to 768 byte palette buffer
    int    mPaletteChange; // changes to 1 if palette changes
    int    mXSize;      // x-size of framebuffer
    int    mYSize;      // y-size of framebuffer
    int    mFrame;    // Current frame
    int    mMaxframe;    // max. frame
    int    mLooped;      // changes to 1 if we're looped.
    unsigned char * mNextframe;   // Pointer to next frame in flicdata
    unsigned char * mLoopframe;   // Pointer to loop frame in flicdata
} FLI;

// Opens a FLI or FLC file, and loads it to memory.
extern FLI * fli_open(char * aFilename);

// Deallocates all data behind aFlidata
extern void fli_free(FLI * aFlidata);

// Renders one frame.
extern void fli_render(FLI * flidata);

#ifdef SOL_FLIFLC_IMPLEMENTATION

#include <string.h>

// FLI header
typedef struct FLIHEADER_ 
{ 
    int       mFliSize; // Should be same as file size.
    short int mFliMagic; // 0AF12h
    short int mFliFrames; // Frames in flic, 4000 max
    short int mWidth; // x size
    short int mHeight;// y size
    short int mDepth; // bits per pixel, always 8 in fli/flc
    short int mFlags; // Bitmapped flags. 0=ring frame, 1=header updated
    short int mSpeed; // Delay between frames in ms (or retraces :))
    short int mReserved1;
    int       mCreated; // MS-dos date of creation
    int       mCreator; // SerNo of animator, 0464c4942h for FlicLib
    int       mUpdated; // MS-dos date of last modify
    int       mUpdater;
    short int mAspectx; // x-axis aspect ratio (320x200: 6)
    short int mAspecty; // y-axis aspect ratio (320x200: 5)
    char      mReserved2[38];
    int       mOframe1; // offset to frame 1 
    int       mOframe2; // offset to frame 2 - for looping, jump here
    char      mReserved3[42];
} FLIHEADER;

// Frame header
typedef struct  FLIFRAMEHEADER_
{
    int       mFramesize;
    unsigned short int mMagic;
    short int mChunks;
    unsigned char mReserved[8];
} FLIFRAMEHEADER;

// Chunk header
typedef struct FLICHUNKHEADER_ 
{
    int       mSize;
    short int mType;
} FLICHUNKHEADER;


/************************************************************************
**  FLI/FLC decoder functions. Part 1: chunk decoders                  **
*************************************************************************/

// "Set whole frame to index 0" -chunk
void ifli_black(FLI * aFli) 
{
    memset(aFli->mFramebuffer, 0, aFli->mXSize * aFli->mYSize);
}

// Uncompressed frame (extremely rare)
void ifli_copy(FLI * aFli, unsigned char *aData) 
{
    memcpy(aFli->mFramebuffer, aData, aFli->mXSize * aFli->mYSize);
}

// 64 - level palette chunk
void ifli_color(FLI * aFli, unsigned char *aData) 
{
    short int *pktaddress;
    unsigned char skip;
    unsigned char set;
    short int numberpk;
    short int packetcount;
    int a;
    pktaddress = (short int *)aData;
    aData += 2;
    numberpk = *pktaddress;
    for (packetcount = 0; packetcount < numberpk; packetcount++) 
    {
        skip = *aData;
        aData++;
        set = *aData;
        aData++;
        if (set == 0) 
        {
            for (a = 0; a < 768; a++)
                *(aFli->mPalette + a) = *(aData + a) * 4;
            aFli->mPaletteChange = 1;
        } 
        else 
        {
            for (a = 0; a < set * 3; a++)
                *(aFli->mPalette + skip * 3 + a) = *(aData + a);
            aFli->mPaletteChange = 1;
            aData += set * 3;
        }
    }
}

// 256 - level palette chunk
void ifli_colour256(FLI * aFli, unsigned char *aData) 
{
    short int *pktaddress;
    unsigned char skip;
    int set;
    short int numberpk;
    short int packetcount;
    int a;
    pktaddress = (short int *)aData;
    aData += 2;
    numberpk = *pktaddress;
    for (packetcount = 0; packetcount < numberpk; packetcount++)
    {
        skip = *aData;
        aData++;
        set = *aData;
        aData++;
        if (set == 0) 
        {
            for (a = 0; a < 768; a++)
                *(aFli->mPalette + a) = *(aData + a);
            aFli->mPaletteChange = 1;
        } 
        else 
        {
            for (a = 0; a < (set*3); a++)
                *(aFli->mPalette + a + skip * 3) = *(aData + a);
            aFli->mPaletteChange = 1;
            aData += (set * 3);
        }
    }
}

// 8b-based delta
void ifli_rc(FLI *aFli, unsigned char *aData)
{
    short int *addlines;
    short int numlines;
    unsigned char *vbuffptr;
    short int linecount;
    unsigned char pktcount, skip, numpkt, sizecount, databyte;
    signed char size;
    unsigned char *linestart;
    vbuffptr = aFli->mFramebuffer;
    addlines = (short int *)aData;
    numlines = *addlines;
    aData += 4;
    addlines += 1;
    vbuffptr += numlines * aFli->mXSize;
    numlines = *addlines;
    linestart = vbuffptr;
    for (linecount = 0; linecount < numlines; linecount++)
    {
        vbuffptr = linestart;
        numpkt = *aData;
        aData++;
        for (pktcount = 0; pktcount < numpkt; pktcount++)
        {
            skip = *aData;
            aData++;
            vbuffptr += skip;
            size = (signed char)*aData;
            aData++;
            if (size >= 0) 
            {
                for (sizecount = 0; sizecount < size; sizecount++) 
                {
                    *vbuffptr = *aData;
                    vbuffptr++;
                    aData++;
                }
            } else {
                size = -size;
                databyte = *aData;
                aData++;
                for (sizecount = 0; sizecount < size; sizecount++)
                {
                    *vbuffptr = databyte;
                    vbuffptr++;
                }
            }
        }
        linestart += aFli->mXSize;
    }
}

// 16b-based delta (FLC only)
void ifli_ss2(FLI *aFli, unsigned char *aData)
{
    short int numlines;
    unsigned char *vbuffptr;
    short int linecount;
    char skip;
    int pktcount, sizecount, databyte;
    short int numpkt;
    signed char size;
    unsigned char *linestart;
    vbuffptr = aFli->mFramebuffer;
    numlines = *(short int *)aData;
    aData += 2;
    linestart = vbuffptr;
    for (linecount = 0; linecount < numlines; linecount++)
    {
        vbuffptr = linestart;
        numpkt = *(short int *)aData;
        aData += 2;
        if (numpkt <= 0) 
        {
            numpkt = -numpkt;
            linecount--;
            linestart += (numpkt - 1) * aFli->mXSize;
        } 
        else
        {
            for (pktcount = 0; pktcount < numpkt; pktcount++)
            {
                skip = *aData;
                aData++;
                vbuffptr += skip;
                size = *aData;
                aData++;
                if (size >= 0)
                {
                    for (sizecount = 0; sizecount < size; sizecount++)
                    {
                        *vbuffptr = *aData;
                        vbuffptr++;
                        aData++;
                        *vbuffptr = *aData;
                        vbuffptr++;
                        aData++;
                    }
                } 
                else 
                {
                    size = -size;
                    databyte = *aData;
                    aData++;
                    for (sizecount = 0; sizecount < size; sizecount++)
                    {
                        *vbuffptr = databyte;
                        vbuffptr++;
                        *vbuffptr = *aData;
                        vbuffptr++;
                    }
                    aData++;
                }
            }
        }
        linestart += aFli->mXSize;
    }
}

// RLE full frame
void ifli_brun(FLI * aFli, unsigned char* aData)
{
    short int numlines;
    unsigned char *vbuffptr;
    unsigned char pktcount, numpkt, sizecount;
    signed char size;
    vbuffptr = aFli->mFramebuffer;
    
    for (numlines = 0; numlines < aFli->mYSize; numlines++)
    {
        numpkt = *aData;
        aData++;
        for (pktcount = 0; pktcount < numpkt; pktcount++)
        {
            size = (signed char)*aData;
            aData++;
            if (size >= 0)
            {
                for (sizecount = 0; sizecount < size; sizecount++) 
                {
                    *vbuffptr = *aData;
                    vbuffptr++;
                }
                aData++;
            } else {
                size = -size;
                for (sizecount = 0; sizecount < size; sizecount++)
                {
                    *vbuffptr = *aData;
                    vbuffptr++;
                    aData++;
                }
            }
        }
    }
}

/************************************************************************
**  FLI/FLC decoder functions. Part 2: frame decoder                   **
*************************************************************************/

unsigned char * ifli_decode_chunk(FLI * aFli, unsigned char *aThischunk)
{
    FLICHUNKHEADER *chunkhead;
    unsigned char *nextchunk;
    unsigned char *chunkdata;
    chunkhead = (FLICHUNKHEADER *)aThischunk;
    nextchunk = aThischunk + chunkhead->mSize;
    chunkdata = aThischunk + 6;
    switch (chunkhead->mType) 
    {
    case  4: // 256-level palette (FLC only)
        ifli_colour256(aFli, chunkdata);
        break;
    case  7: // 16b-based delta (FLC only)
        ifli_ss2(aFli, chunkdata);
        break;
    case 11: // 64-level palette
        ifli_color(aFli, chunkdata);
        break;
    case 12: // 8b-based delta
        ifli_rc(aFli, chunkdata);
        break;
    case 13: // full black frame
        ifli_black(aFli);
        break;
    case 15: // RLE full frame
        ifli_brun(aFli, chunkdata);
        break;
    case 16: // full frame, no compression
        ifli_copy(aFli, chunkdata);
        break;
    case 18: // postage stamp sized image - out of this routine's scope
        break;
    default: // unknown/irrelevant
        break;
    }
    return nextchunk;
}

unsigned char *ifli_frame(FLI * aFli)
{
    FLIFRAMEHEADER *thisframe;
    unsigned char *nextframe;
    short int numchunks;
    unsigned char *thischunk;
    thisframe = (FLIFRAMEHEADER *)aFli->mNextframe;
    nextframe = aFli->mNextframe;
    nextframe += thisframe->mFramesize;
    thischunk = (aFli->mNextframe + sizeof(FLIFRAMEHEADER));
    if (thisframe->mMagic == 0xf1fa)
    {
        for (numchunks = 1; numchunks <= thisframe->mChunks; numchunks++)
        {
            thischunk = ifli_decode_chunk(aFli, thischunk);
        }
    }
    return nextframe;
}

/************************************************************************
**  FLI/FLC decoder functions. Part 3: "high level" functions          **
*************************************************************************/

FLI * fli_open(char * filename)
{
    FILE * f;
    FLIHEADER header;
    FLIDATA * fli;
    f = fopen(filename, "rb");
    if (f == 0) 
    {
        return 0;
    }
    fli = (FLIDATA *)malloc(sizeof(FLIDATA));
    fread(&header, 1, sizeof(header), f);
    if (header.mFliMagic != 0xAF12)
    {
        fclose(f);
        return 0;
    }
    fli->mFramebuffer = (unsigned char*)malloc(header.mWidth * header.mHeight);
    fli->mXSize = header.mWidth;
    fli->mYSize = header.mHeight;
    fli->mPalette = (unsigned char*)malloc(768);
    fli->mPaletteChange = 0;
    fli->mFrame = 0;
    fli->mLooped = 0;
    fli->mMaxframe = header.mFliFrames;
    fli->mFlicdata = (unsigned char *)malloc(header.mFliSize - sizeof(header));
    fread(fli->mFlicdata, 1, header.mFliSize - sizeof(header), f);
    fli->mNextframe = fli->mFlicdata;
    fli->mLoopframe = fli->mNextframe;
    fclose(f);
    memset(fli->mFramebuffer, 0, header.mWidth * header.mHeight);
    return fli;
}

void fli_free(FLI * aFli)
{
  free(aFli->mFlicdata);
  free(aFli->mPalette);
  free(aFli->mFramebuffer);
  free(aFli);
}

void fli_render(FLI * aFli)
{
    aFli->mNextframe = ifli_frame(aFli);
    aFli->mFrame++;
    if (aFli->mFrame == aFli->mMaxframe)
    {
        aFli->mFrame = 0;
        aFli->mNextframe = aFli->mLoopframe;
        aFli->mLooped = 1;
    }
}

#endif // SOL_FLIFLC_IMPLEMENTATION
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SOL_FLIFLC_H
