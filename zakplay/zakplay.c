/*
 * Part of Jari Komppa's zx spectrum next suite
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
 */
 
 
 /*
 State machine:
 0 - ISR playing from buffer A, app should just wait. ISR 0->1
 0 - ISR notices the song ends                        ISR 0->4
 0 - ISR notices the song ends and wants to loop      ISR 0->5
 1 - ISR moved to buffer B, app should copy B to A.   App 1->2
 2 - App copied B to A, ISR should move to A.         ISR 2->3
 3 - ISR moved to A, app should fill buffer B.        App 3->0
 4 - Shutdown and quit                                n/a
 5 - App copies loop point to buffers a and b         App 5->6
 6 - ISR moved to A/B                                 ISR 6->0 or 6->1 
 */



#define HWIF_IMPLEMENTATION
#include "hwif.c"

#include "yofstab.h"
const unsigned char propfont[] = {
#include "font_elegante_pixel.h"
};

const unsigned char zaklogo[] = {
#include "zaklogo.h"
};

extern void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY);
extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);

extern void writenextreg(unsigned char reg, unsigned char val);
extern unsigned char readnextreg(unsigned char reg);
extern unsigned short findmaxscan();
extern unsigned char allocpage();
extern void freepage(unsigned char page);

extern void setaychip(unsigned char val);
extern void aywrite(unsigned char reg, unsigned char val);

extern unsigned short framecounter;
extern char *cmdline;
extern void setupisr7();
extern void closeisr7();
extern void setupisr0();
extern void di();
extern void ei();
extern unsigned char* dzx7_mega(unsigned char *src)  __z88dk_fastcall;         

void printnum(unsigned char v, unsigned char x, unsigned char y)
{
    char temp[4];
    char p = 0;
    char ov = v;
    temp[0] = '0';
    temp[1] = '0';
    temp[2] = '0';
    if (ov >= 100) { while (v >= 100) { temp[p]++; v -= 100; } p++; }
    if (ov >= 10) { while (v >= 10) { temp[p]++; v -= 10; } p++; }
    while (v >= 1) { temp[p]++; v -= 1; } p++;  
    temp[p] = 0;
    
    drawstringz(temp, x, y);    
}

/*
our rather wasteful memory layout
0xe000 - nextreg original values
0xe100 - allocated page handles
0xe200 - state variables
0xe300 - ay register state for visualization
0xe400 - buffer A
0xe800 - buffer B
0xf000 - zak header
0xfdfd - isr hop table etc
*/
__at (0xe000) unsigned char nextregbackup[256];
__at (0xe100) unsigned char pages[100];

__at (0xe200) unsigned char activepage;
__at (0xe201) unsigned short ofs;
__at (0xe203) unsigned char framedelay;
__at (0xe204) unsigned char state;
__at (0xe205) unsigned short srcofs;
__at (0xe207) unsigned short kblock;
__at (0xe209) unsigned short maxscan;
__at (0xe20b) unsigned short scanlineskip;
__at (0xe20d) unsigned char frameskip;
__at (0xe20e) unsigned short nextscanline;
__at (0xe250) unsigned char debugvalue;

__at (0xe300) unsigned char ayregs[48];
__at (0xe400) unsigned char buffer_a[1024];
__at (0xe800) unsigned char buffer_b[1024];
__at (0xf000) unsigned char zakheader[100];
__at (0xf000+10) unsigned char zak_chiptype;
__at (0xf000+11) unsigned char zak_flags;
__at (0xf000+12) unsigned short zak_kblocks;
__at (0xf000+14) unsigned short zak_lastblock;
__at (0xf000+16) unsigned short zak_loopblock;
__at (0xf000+18) unsigned short zak_loopbyte;



char memcmp(char *a, char *b, unsigned short l)
{
    unsigned short i = 0;
    while (i < l)
    {
        char v = a[i] - b[i];
        if (v != 0) return v;            
        i++;
    }
    return 0;
}

void memset(char *a, char b, unsigned short l)
{
    unsigned short i = 0;
    while (i < l)
    {
        a[i] = b;
        i++;
    }
}

void memcpy(char *a, char * b, unsigned short l)
{
    unsigned short i = 0;
    while (i < l)
    {
        a[i] = b[i];
        i++;
    }
}

void bytetohex(unsigned char v, char *p)
{
    char hex[17] = "0123456789ABCDEF";
    *p = hex[v>>4];
    p++;
    *p = hex[v&0xf];
}

void shorttohex(unsigned short v, char *p)
{
    bytetohex(v >> 8, p);
    bytetohex(v, p+2);
}

void printshort(unsigned short v, unsigned char x, unsigned char y)
{
    char temp[5];
    shorttohex(v, temp);
    temp[4] = 0;
    drawstringz(temp, x, y);    
}

// really good candidate for optimization (both size and speed wise)
void copybufbtoa()
{
    memcpy(buffer_a, buffer_b, 1024);
}

void fillbufb()
{
    if (kblock == zak_kblocks)
    {
        memset(buffer_b, 0, 1024);
        return;
    }
    kblock++;
    writenextreg(0x55, pages[2 + activepage]);
    writenextreg(0x56, pages[2 + activepage + 1]);
    //memcpy(buffer_b, (unsigned char*)srcofs, 1024);    
    srcofs = (unsigned short)dzx7_mega((unsigned char*)srcofs) - 2;
    //printshort(debugvalue, 0, 7);
    //printshort(srcofs, 0, 8 + debugvalue);
    debugvalue++;
    if (srcofs >= 0xa000 + 8192)
    {
        srcofs -= 8192;
        activepage++;
    }
}

void zeroay()
{
    unsigned char r, c;
    for (c = 0; c < 3; c++)
    {
        setaychip(c); // AY 0, 1 or 2
        for (r = 0; r < 14; r++)
        {
            aywrite(r, 0);
        }
    }
}

void isr()
{
    unsigned char reg, val;
    
    //gPort254 = 1;
    
    // I have a funny feeling I'll be writing this in assembler:
    nextscanline += scanlineskip;
    while (nextscanline > maxscan) nextscanline -= maxscan;
    writenextreg(0x22, 2 + 4 + (nextscanline >> 8));
    writenextreg(0x23, nextscanline & 0xff);
    
    if (state == 2)
    {
        ofs -= 1024;
        state = 3;
    }
    //gPort254 = 0;

    if (framedelay)
    {
        framedelay--;
        return;
    }

    //gPort254 = 2;
    do
    {
        val = buffer_a[ofs]; ofs++;
        reg = buffer_a[ofs]; ofs++;
        if (kblock == zak_kblocks)
        {
            if (ofs >= 1024 + zak_lastblock)
            {
                // todo: handle looping
                state = 4;
            }
        }
        else
        {
            if (ofs == 1024)
            {
                state = 1;
            }
        }
        if (reg < 48)
        {
            ayregs[reg] = val;
            setaychip(reg >> 4); // AY 0, 1 or 2
            aywrite(reg & 15, val); // low 4 bits is reg number
            //port254(val & 7);
        }

    } while ((reg & 0x80) == 0);    
    framedelay = val - 1;
    for (reg = 0; reg < frameskip; reg++)
        framedelay += val;
    //gPort254 = 0;
}

char test(char a, char b)
{
    return a > b;
}

char readstring(char f, char ofs)
{
    char i;
    char len;
    char temp[256];
    fread(f, &len, 1); // size byte
    if (!len)
    {
        return ofs;
    }
    fread(f, temp, len); 
    // sanitize
    for (i = 0; i < len; i++)
    {
        if (temp[i] < 32 || temp[i] > 126)
            temp[i] = '?';
    }
    temp[len] = 0;
    // todo: wrap long lines, handle newlines, ??
    drawstringz(temp, 0, ofs);
    return ofs + 2;
}

char checkhdr(char f)
{
    char ofs;
    if (f == 0) return 4;
    fread(f, zakheader, 28); // zak header length
    // is signature ok?
    if (memcmp(zakheader, "CHIPTUNE", 8) != 0)
    {
        return 1;
    }
    // is this an AY/YM file?
    if (!(zak_chiptype == 1 ||
          zak_chiptype == 2 ||
          zak_chiptype == 3) ||
          (zak_flags & 2) == 2)
    {
        return 2;
    }
    
    // Is this a sub-255hz file?    
    if (!(//zakheader[20] == 50 &&
          zakheader[21] == 0 &&
          zakheader[22] == 0 && 
          zakheader[23] == 0))
    {
        return 3;
    }
    
    // Select AY or FM based on song flag
    writenextreg(0x06, (readnextreg(0x06) & 0xfc) + (zak_flags & 64) ? 0 : 1);
    
    if (zak_flags & 64) 
    {
        drawstringz("YM2149F", 0, 20);
    }
    else
    {
        drawstringz("AY-3-8910", 0, 20);
    }
    drawstringz("Hz", 2, 19);
    printnum(zakheader[20], 0, 19);

    if (zak_chiptype == 1) drawstringz("Normal", 0, 21);
    if (zak_chiptype == 2) drawstringz("Turbosound", 0, 21);
    if (zak_chiptype == 3) drawstringz("Turbosound Next", 0, 21);      
    
    // read strings into buffer, sanitize strings and print strings on screen
    ofs = readstring(f, 5);
    ofs = readstring(f, ofs);
    readstring(f, ofs);
    // at this point we're ready to load the data
    return 0;
}

// Allocate pages and load the whole file while there's data to be read..
void readsongdata(char f)
{
    unsigned short b;
    do
    {
        pages[0]++;
        pages[pages[0]] = allocpage();
        writenextreg(0x56, pages[pages[0]]);
        b = fread(f, (unsigned char*)0xc000, 8192);
    }
    while (b == 8192);
}

void vis()
{
    unsigned char i;
    unsigned char j;
    unsigned char prog;
    unsigned char mask;
    unsigned short goal;
    unsigned short acc;
    
    // no div, no problem.... kinda
    acc = 0;
    goal = kblock * 32 * 8;
    prog = 0;
    while (goal > acc)
    {
        acc += zak_kblocks;
        prog++;
    }
    
    mask = prog & 7;
    mask = 0xff << (7-mask);
    prog >>= 3;

    for (i = 0; i < 32; i++)
    {
        *((unsigned char *)yofs[23] + i) = (i > prog) ? 0 : ((i < prog) ? 0xff : mask);
    }
        
    prog = (srcofs >> 8) & 31;
    
    for (i = 0; i < 32; i++)
    {
        *((unsigned char *)yofs[23] + i + 512) = (i >= prog) ? 0 : 0xff;
    }

    prog = ofs >> 5;
    
    for (i = 0; i < 32; i++)
    {
        *((unsigned char *)yofs[23] + i + 1024) = (i >= prog) ? 0 : 0xff;
    }
    
    // register dots
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < 4; i++)
        {
            *((unsigned char *)yofs[5 + j*4] + 28 + i) = ayregs[j*16+i] & 0x55;
            *((unsigned char *)yofs[5 + j*4] + 28 + i + 512) = (ayregs[j*16+i] >> 1) & 0x55;
            *((unsigned char *)yofs[5 + j*4+1] + 28 + i) = ayregs[j*16+i+4] & 0x55;
            *((unsigned char *)yofs[5 + j*4+1] + 28 + i + 512) = (ayregs[j*16+i+4] >> 1) & 0x55;
            *((unsigned char *)yofs[5 + j*4+2] + 28 + i) = ayregs[j*16+i+8] & 0x55;
            *((unsigned char *)yofs[5 + j*4+2] + 28 + i + 512) = (ayregs[j*16+i+8] >> 1) & 0x55;
            *((unsigned char *)yofs[5 + j*4+3] + 28 + i) = ayregs[j*16+i+12] & 0x55;
            *((unsigned char *)yofs[5 + j*4+3] + 28 + i + 512) = (ayregs[j*16+i+12] >> 1) & 0x55;

        }
    }
}

void openfile(char *f)
{
    unsigned char temp[128];
    unsigned char i;
    i = 0;
    while (i < 127 && cmdline[i] != 0 && cmdline[i] != 0xd && cmdline[i] != ':')
    {
        temp[i] = cmdline[i];
        i++;
    }
    temp[i] = 0;
    drawstringz(temp, 0, 3);    
    *f = fopen(temp, 1);
}

void drawlogo()
{
    unsigned char x,y,c;
    for (c = 0, y = 0; y < 58; y++)
    {
        for (x = 0; x < 4; x++, c++)
        {
            *((unsigned char*)(yofs[0]+y*32+x+28)) = zaklogo[c];
        }
    }
}

// refresh rate, in hz, multiplied by 16.
const unsigned short modefreq[16] = {
789,805,830,845,874,902,930,800,
930,949,978,995,1029,1062,1096,960
};


void main()
{     
    char f;
    char r;
    char r7;
    char vidmode;
    unsigned long templong;
    unsigned long templong2;
    unsigned short i;
    r7 = readnextreg(0x07); // turbo
    writenextreg(0x07, 3); // set speed to 28MHz
    
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,7,24*32);
    gPort254 = 0;
    drawlogo();
    vidmode = (readnextreg(0x11) & 7) | ((readnextreg(0x5) & 4)?8:0);
    

    r = allocpage();
    f = readnextreg(0x57);
    writenextreg(0x57, r);    
    nextregbackup[0x55] = readnextreg(0x55); // mmu 5
    nextregbackup[0x56] = readnextreg(0x56); // mmu 6
    nextregbackup[0x57] = f;                 // mmu 7
    nextregbackup[0x06] = readnextreg(0x06); // peripheral2 for fm vs ay flag
    nextregbackup[0x07] = r7;                // turbo
    nextregbackup[0x22] = readnextreg(0x22); // video line interrupt control reg
    nextregbackup[0x23] = readnextreg(0x23); // video line interrupt data reg
    pages[0] = 1; // number of allocated pages
    pages[1] = r; // allocated top page
    openfile(&f);
    r = checkhdr(f);
    if (r)
    {
        fclose(f);
        switch (r)
        {
        case 1: drawstringz("Not a zak file", 0, 0); break;
        case 2: drawstringz("Not an AY/FM zak file", 0, 0); break;
        case 3: drawstringz("Unsupported play frequency", 0, 0); break;
        case 4: drawstringz("File not found", 0, 0); break;
        }
    }
    else
    {        
        readsongdata(f);
        fclose(f); // avoid disk issues if we crash after this
        templong = 0;
        maxscan = findmaxscan();

        for (i = 0; i < maxscan; i++)
            templong += modefreq[vidmode];
        // templong is now scanlines per second * 16
        i = zakheader[20]; // song replay freq
        // we can't wait over a frame, so.. make things more frequent
        frameskip = 0;        
        while (i < 70) // display freqs go to almost 70hz(!)
        {
            i += zakheader[20];
            frameskip++; // how many frames to just schedule next int and return
        }
        templong2 = 0;
        scanlineskip = 0;
        // divide scanlines per second by our update rate
        while (templong2 < templong)
        {
            templong2 += i;
            scanlineskip++;
        }       
        scanlineskip >>= 4; // div by 16, and we have our scanlineskip value
                
        memset(ayregs, 0, 3*16);
        drawstringz("ZAK player 0.2 by Jari Komppa", 0, 0);
        drawstringz("http://iki.fi/sol", 0, 1);        
               
        debugvalue = 0;
        activepage = 0;
        ofs = 0;
        srcofs = 0xa000;
        framedelay = 0;
        state = 0;
        kblock = 0;

        fillbufb();
        copybufbtoa();
        fillbufb();

        setupisr7();
        readkeyboard();
        writenextreg(0x22, 2+4); // enable line interrupt, disable ula interript
        writenextreg(0x23, 0); // set interrupt to row 0
        nextscanline = 0;
        ei();
        while (!KEYDOWN(SPACE) && state != 4)
        {
            readkeyboard();
            vis(); 
            if (state == 1)
            {
                copybufbtoa();
                state = 2;
            }
            if (state == 3)
            {
                fillbufb();
                state = 0;
            }
        }    
        di();    
        closeisr7();
    }
    zeroay(); // silence, please
    writenextreg(0x22, nextregbackup[0x22]);
    writenextreg(0x23, nextregbackup[0x23]);

    writenextreg(0x06, nextregbackup[0x06]);
    writenextreg(0x07, nextregbackup[0x07]);
    for (r = 0; r < pages[0]; r++)
        freepage(pages[r+1]);
    writenextreg(0x55, nextregbackup[0x55]);
    writenextreg(0x56, nextregbackup[0x56]);
    writenextreg(0x57, nextregbackup[0x57]);
}
