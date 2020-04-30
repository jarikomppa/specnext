/*
 * Part of Jari Komppa's zx spectrum next suite
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
 */
 
 
 /*
 State machine:
 0 - ISR playing from buffer A, app should just wait. ISR 0->1
 1 - ISR moved to buffer B, app should copy B to A.   App 1->2
 2 - App copied B to A, ISR should move to A.         ISR 2->3
 3 - ISR moved to A, app should fill buffer B.        App 3->0
 */



#define HWIF_IMPLEMENTATION
#include "hwif.c"

#include "yofstab.h"
const unsigned char propfont[] = {
#include "font_elegante_pixel.h"
};

extern void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY);
extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);

extern void writenextreg(unsigned char reg, unsigned char val);
extern unsigned char readnextreg(unsigned char reg);
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

void printnum(unsigned char v, unsigned char x, unsigned char y)
{
    char temp[4];
    temp[0] = '0';
    temp[1] = '0';
    temp[2] = '0';
    temp[3] = 0;
    while (v >= 100) { temp[0]++; v -= 100; }
    while (v >= 10) { temp[1]++; v -= 10; }
    while (v >= 1) { temp[2]++; v -= 1; }
    drawstringz(temp, x, y);    
}

#define ACTIVEPAGE 0
#define OFSLO 1
#define OFSHI 2
#define FRAMEDELAY 3

/*
rather wasteful memory layout
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
__at (0xe300) unsigned char ayregs[48];
__at (0xe400) unsigned char buffer_a[1024];
__at (0xe800) unsigned char buffer_b[1024];
__at (0xf000) unsigned char zakheader[100];

// really good candidate for optimization (both size and speed wise)
void copybufbtoa()
{
    unsigned short i;
    for (i = 0; i < 1024; i++)
        buffer_a[i] = buffer_b[i];
}

void fillbufb()
{
}

void isr()
{
    unsigned char *d = (char*)0xc000;
    unsigned char reg, val;
    writenextreg(0x56, pages[2 + activepage]);
    if (framedelay)
    {
        framedelay--;
        return;
    }

    do
    {
        val = d[ofs]; ofs++;
        reg = d[ofs]; ofs++;
        if (reg < 48)
        {
            ayregs[reg] = val;
            setaychip(reg >> 4); // AY 0, 1 or 2
            aywrite(reg & 15, val); // low 4 bits is reg number
            //port254(val & 7);
        }
        if (ofs == 8192)
        {
            ofs = 0;
            activepage++;
            writenextreg(0x56, pages[2 + activepage]);            
        }
    } while ((reg & 0x80) == 0);
    framedelay = val - 1;   
}

char memcmp(char *a, char *b, char l)
{
    char i = 0;
    while (i < l)
    {
        char v = a[i] - b[i];
        if (v != 0) return v;            
        i++;
    }
    return 0;
}

void memset(char *a, char b, char l)
{
    char i = 0;
    while (i < l)
    {
        a[i] = b;
        i++;
    }
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
    if (!(zakheader[10] == 1 ||
          zakheader[10] == 2 ||
          zakheader[10] == 3) ||
          (zakheader[11] & 2) == 2)
    {
        return 2;
    }
    
    // Is this a 50hz file?    
    if (!(zakheader[20] == 50 &&
          zakheader[21] == 0 &&
          zakheader[22] == 0 && 
          zakheader[23] == 0))
    {
        return 3;
    }
    
    // Select AY or FM based on song flag
    writenextreg(0x06, (readnextreg(0x06) & 0xfc) + (zakheader[11] & 64) ? 0 : 1);
    // read strings into buffer, sanitize strings and print strings on screen
    ofs = readstring(f, 4);
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
    unsigned char prog = ofs >> 8;
    
    for (i = 0; i < 32; i++)
    {
        *((unsigned char *)yofs[23] + i) = (i >= prog) ? 0 : 0xff;
    }
    
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < 16; i++)
            *((unsigned char *)yofs[18 + j] + i) = ayregs[j*16+i];
     /*
        prog = ayregs[0 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i ) = (i == prog) ? 0xff : 0;
        }

        prog = ayregs[2 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i + 256) = (i == prog) ? 0xff : 0;
        }

        prog = ayregs[4 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i + 512) = (i == prog) ? 0xff : 0;                
        }
        */
    }
}

void main()
{     
    char f;
    char r;    
    r = allocpage();
    f = readnextreg(0x57);
    writenextreg(0x57, r);    
    nextregbackup[0x55] = readnextreg(0x55);
    nextregbackup[0x56] = readnextreg(0x56);
    nextregbackup[0x57] = f; // 0x57
    nextregbackup[0x06] = readnextreg(0x06); // peripheral2
    nextregbackup[0x07] = readnextreg(0x07); // turbo
    pages[0] = 1; // number of allocated pages
    pages[1] = r; // allocated top page
    writenextreg(0x07, 3); // set speed to 28MHz
    f = fopen("adversary.zak", 1);
    r = checkhdr(f);
    if (r)
    {
        fclose(f);
        switch (r)
        {
        case 1: drawstringz("Not a zak file", 0, 0); break;
        case 2: drawstringz("Not an AY/FM zak file", 0, 0); break;
        case 3: drawstringz("Not a 50hz zak file", 0, 0); break;
        case 4: drawstringz("File not found", 0, 0); break;
        }
    }
    else
    {
        readsongdata(f);
        fclose(f); // avoid disk issues if we crash after this
        memset(ayregs, 0, 3*16);
        drawstringz("ZAK player 0.1 by Jari Komppa", 0, 0);
        drawstringz("http://iki.fi/sol", 0, 1);        
        activepage = 0;
        ofs = 0;
        framedelay = 0;
        setupisr7();
        readkeyboard();
        ei();
        while (!KEYDOWN(SPACE))
        {
            readkeyboard();
            vis(); 
        }    
        di();    
        closeisr7();
    }
    writenextreg(0x06, nextregbackup[0x06]);
    writenextreg(0x07, nextregbackup[0x07]);
    for (r = 0; r < pages[0]; r++)
        freepage(pages[r+1]);
    writenextreg(0x55, nextregbackup[0x55]);
    writenextreg(0x56, nextregbackup[0x56]);
    writenextreg(0x57, nextregbackup[0x57]);
}
