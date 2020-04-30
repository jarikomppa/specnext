/*
 * Part of Jari Komppa's zx spectrum next suite
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
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

void isr()
{
    unsigned char *p = (char*)0xe200;
    unsigned char *pages = (char*)0xe100;
    unsigned char *d = (char*)0xc000;
    unsigned short ofs = ((unsigned short)p[1]) | (((unsigned short)p[2]) << 8);
    unsigned char reg, val;
    writenextreg(0x56, pages[2 + p[0]]);
    if (p[3])
    {
        p[3]--;
        return;
    }

    do
    {
        val = d[ofs]; ofs++;
        reg = d[ofs]; ofs++;
        if (reg < 48)
        {
            p[0x100 + reg] = val;
            setaychip(reg >> 4);
            aywrite(reg & 15, val);
            //port254(val & 7);
        }
        if (ofs == 8192)
        {
            ofs = 0;
            p[0]++;
            writenextreg(0x56, pages[2 + p[0]]);            
        }
    } while ((reg & 0x80) == 0);
    p[3] = val - 1;
    
    p[1] = ofs;
    p[2] = ofs >> 8;
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
    char *p = (char*)0xe400;
    char ofs;
    if (f == 0) return 4;
    fread(f, p, 28); // zak header length
    // is signature ok?
    if (memcmp(p, "CHIPTUNE", 8) != 0)
    {
        return 1;
    }
    // is this an AY/YM file?
    if (!(p[10] == 1 ||
          p[10] == 2 ||
          p[10] == 3) ||
          (p[11] & 2) == 2)
    {
        return 2;
    }
    
    // Is this a 50hz file?    
    if (!(p[20] == 50 &&
          p[21] == 0 &&
          p[22] == 0 && 
          p[23] == 0))
    {
        return 3;
    }
    
    // Select AY or FM based on song flag
    writenextreg(0x06, (readnextreg(0x06) & 0xfc) + (p[11] & 64) ? 0 : 1);
    // read strings into buffer, sanitize strings and print strings on screen
    ofs = readstring(f, 4);
    ofs = readstring(f, ofs);
    readstring(f, ofs);
    // at this point we're ready to load the data
    return 0;
}

void readsongdata(char f)
{
    char *p = (char*)0xe000;
    unsigned short b;
    do
    {
        p[0x100]++;
        p[0x100 + p[0x100]] = allocpage();
        writenextreg(0x56, p[0x100 + p[0x100]]);
        b = fread(f, (unsigned char*)0xc000, 8192);
    }
    while (b == 8192);
}

void vis()
{
    unsigned char *p = (unsigned char*)0xe200;
    unsigned char i;
    unsigned char j;
    unsigned char prog = p[2];
    
    for (i = 0; i < 32; i++)
    {
        *((unsigned char *)0x50e0 + i) = (i >= prog) ? 0 : 0xff;
    }
    
    for (j = 0; j < 3; j++)
    {
        //prog = (p[0x101 + j * 16] << 1) |  (p[0x100 + j * 16] >> 7);
        prog = p[0x100 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i ) = (i == prog) ? 0xff : 0;
        }

        //prog = (p[0x103 + j * 16] << 1) |  (p[0x102 + j * 16] >> 7);
        prog = p[0x102 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i + 256) = (i == prog) ? 0xff : 0;
        }

        //prog = (p[0x105 + j * 16] << 1) |  (p[0x104 + j * 16] >> 7);
        prog = p[0x104 + j * 16] >> 3;

        for (i = 0; i < 32; i++)
        {
            *((unsigned char *)yofs[20 + j] + i + 512) = (i == prog) ? 0xff : 0;                
        }
    }


}

void main()
{     
    char *p = (char*)0xe000;
    char f;
    char r;    
    r = allocpage();
    f = readnextreg(0x57);
    writenextreg(0x57, r);    
    p[0x55] = readnextreg(0x55);
    p[0x56] = readnextreg(0x56);
    p[0x57] = f; // 0x57
    p[0x06] = readnextreg(0x06); // peripheral2
    p[0x07] = readnextreg(0x07); // turbo
    p[0x100] = 1; // number of allocated pages
    p[0x101] = r; // allocated top page
    writenextreg(0x07, 3); // set speed to 28MHz
    f = fopen("adversary.zak", 1);
    r = checkhdr(f);
    if (r)
    {
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
        drawstringz("ZAK player 0.1 by Jari Komppa", 0, 0);
        drawstringz("http://iki.fi/sol", 0, 1);        
        p[0x200] = 0;
        p[0x201] = 0;
        p[0x202] = 0;
        p[0x203] = 0;
        setupisr7();
        readkeyboard();
        ei();
        while (!KEYDOWN(SPACE))
        {
            // todo: visualize here
            readkeyboard();
            vis(); 
        }    
        di();    
        closeisr7();
    }
    writenextreg(0x06, p[0x06]);
    writenextreg(0x07, p[0x07]);
    for (r = 0; r < p[0x100]; r++)
        freepage(p[0x101+r]);
    writenextreg(0x55, p[0x55]);
    writenextreg(0x56, p[0x56]);
    writenextreg(0x57, p[0x57]);
    fclose(f);
}
