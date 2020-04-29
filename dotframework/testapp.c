/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

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

extern void setupisr7();
extern void closeisr7();
extern void setupisr0();
extern void di();
extern void ei();

extern unsigned short framecounter;
extern char *cmdline;

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

void test_fileio()
{
    unsigned char buf[128];
    unsigned char f = 137;
    drawstringz("Opening readme.md", 0, 1);
    f = fopen("README.MD", 1);
    printnum(f, 0, 0);
    if (f)
    {
        buf[0] = 137;
        fread(f, buf, 50);
        fclose(f);
        if (buf[0] == 137) 
        {
            drawstringz("testbuf unchanged :(",0,3); 
        }
        else
        {
            buf[50] = 0;
            drawstringz(buf,0,3);
        }
    }
    f = fopen("README.MD", 1);
    printnum(f, 5, 0);
    fclose(f);
    f = fopen("README.MD", 1);
    printnum(f, 10, 0);
    fclose(f);
    f = fopen("README.MD", 1);
    printnum(f, 15, 0);
    //fclose(f);
    f = fopen("README.MD", 1);
    printnum(f, 20, 0);
    fclose(f);
}

void test_mmu()
{
    unsigned char f;

    printnum(readnextreg(0x50 + 0), 0, 13);
    printnum(readnextreg(0x50 + 1), 0, 14);
    printnum(readnextreg(0x50 + 2), 0, 15);
    printnum(readnextreg(0x50 + 3), 0, 16);
    printnum(readnextreg(0x50 + 4), 0, 17);
    printnum(readnextreg(0x50 + 5), 0, 18);
    printnum(readnextreg(0x50 + 6), 0, 19);
    printnum(readnextreg(0x50 + 7), 0, 20);

    f = readnextreg(0x50);
    writenextreg(0x50, 2);

    printnum(readnextreg(0x50 + 0), 10, 13);
    printnum(readnextreg(0x50 + 1), 10, 14);
    printnum(readnextreg(0x50 + 2), 10, 15);
    printnum(readnextreg(0x50 + 3), 10, 16);
    printnum(readnextreg(0x50 + 4), 10, 17);
    printnum(readnextreg(0x50 + 5), 10, 18);
    printnum(readnextreg(0x50 + 6), 10, 19);
    printnum(readnextreg(0x50 + 7), 10, 20);

//    do_halt();
    writenextreg(0x50, f); // return ROM
    
    printnum(readnextreg(0x50 + 0), 20, 13);
    printnum(readnextreg(0x50 + 1), 20, 14);
    printnum(readnextreg(0x50 + 2), 20, 15);
    printnum(readnextreg(0x50 + 3), 20, 16);
    printnum(readnextreg(0x50 + 4), 20, 17);
    printnum(readnextreg(0x50 + 5), 20, 18);
    printnum(readnextreg(0x50 + 6), 20, 19);
    printnum(readnextreg(0x50 + 7), 20, 20);
    port254(5);
    //do_freeze();
}

void test_alloc()
{
    char f1 = 252;
    char f2 = 252;
    char f3 = 252;
    char f4 = 252;
    f1 = allocpage();
    f2 = allocpage();
    f3 = allocpage();
    f4 = allocpage();
    printnum(f1, 0, 0);
    printnum(f2, 0, 1);
    printnum(f3, 0, 2);
    printnum(f4, 0, 3);
    freepage(f1);
    freepage(f2);
    freepage(f3);
    freepage(f4);
    port254(3);
}

void test_audio()
{
    unsigned char *p = (unsigned char*)0;
    unsigned char i;
    while (1)
    {
        for (i = 0; i < 13; i++)
        {
            aywrite(i, *p);
            p++;
        }
        do_halt();
    }
}


void isr()
{
}


void test_isr()
{
    unsigned short bahcounter = 0;
    char p = allocpage();
    char m7 = readnextreg(0x50 + 7);
    writenextreg(0x50 + 7, p);
    setupisr7();
    ei();
    readkeyboard();
    while (!KEYDOWN(SPACE))
    {
        unsigned char *o = (unsigned char*)(0x4000 + (32*192));
        *o = framecounter;
        o++;
        //*o = foo;
        readkeyboard();
        printnum(framecounter, 0, 2);
        //printnum(foo, 0, 3);
        port254(framecounter);
    }
    di();
    closeisr7();
    writenextreg(0x50 + 7, m7);
    freepage(p);    
}

void test_readregs()
{
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        printnum(i, 0, i);
        printnum(readnextreg(i), 3, i);
        printnum(i+16, 7, i);
        printnum(readnextreg(i+16), 10, i);
        printnum(i+32, 14, i);
        printnum(readnextreg(i+32), 17, i);
        printnum(i+48, 20, i);
        printnum(readnextreg(i+48), 23, i);
    }    
}

void test_cmdline()
{
    unsigned char temp[128];
    char i = 0;
    while (i < 127 && cmdline[i] != 0 && cmdline[i] != 0xd && cmdline[i] != ':')
    {
        temp[i] = cmdline[i];
        i++;
    }
    temp[i] = 0;
    drawstringz(temp, 0, 0);    
}

void main()
{ 
    test_cmdline();
//    test_isr();
//    test_audio();
//    test_fileio();      
//    test_mmu();
//    test_alloc();
//    test_readregs();
}
