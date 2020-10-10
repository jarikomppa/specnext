/*
 * Part of Jari Komppa's zx spectrum suite 
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
extern void memcpy(char *dest, const char *source, unsigned short count);

static const unsigned char vortex[] = {
#include "vortex.h"
};

static const unsigned char pal[] = {
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
5, 0, 0, 0, 9, 9, 0, 0, 0, 0, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 9, 9, 0, 0, 0, 0, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 9, 9, 0, 0, 0, 0, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 9, 9, 0, 0, 0, 0, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 9, 9, 0, 0, 0, 0, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
5, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 3,
5, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 3,
5, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 0, 0, 0, 3,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
};

void isr()
{
}

void testlores()
{
    unsigned short i, j;
    //writenextreg(0x15, readnextreg(0x15) | 0x80);
    
    //gPort254 = 0; // border to index 0
     // disable ULA
//     PORT_NEXTREG_SELECT= 0x68;
//     PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // turn bit 7 on, disabling ula
    
    PORT_NEXTREG_SELECT = 0x15;
    PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // turn bit 7 on, enabling lores
    
    // The first 48 lines are stored between $4000 and $5800, and the second 48 between $6000 and $7800. 
    // Each byte is an index into the ULA palette. 
    for (i = 0x4000; i < 0x5800; i++)
       *((char*)i) = vortex[i-0x4000];
    for (i = 0x6000; i < 0x7800; i++)
       *((char*)i) = vortex[i-0x6000+0x1800];

//    for (i = 0x5800; i < 0x5B00; i++)
//       *((char*)i) = i;
    
    // Set transparent color to 0   
    PORT_NEXTREG_SELECT = 0x14;
    PORT_NEXTREG_IO = 0;
    // Set fallback color to 0
    PORT_NEXTREG_SELECT = 0x4a;
    PORT_NEXTREG_IO = 0;
    
    // select ULA palette 0    
    PORT_NEXTREG_SELECT = 0x43;
    PORT_NEXTREG_IO = 1; // enable ulanext
    
    PORT_NEXTREG_SELECT = 0x42;
    PORT_NEXTREG_IO = 0xff; // ulanext color mask
    
    j = 0;
    while (1)
    {
        // Dump crap into palette
        PORT_NEXTREG_SELECT = 0x40;
        PORT_NEXTREG_IO = 0;    
        PORT_NEXTREG_SELECT = 0x44;    
        for (i = 0; i < 256; i++)
        {
            PORT_NEXTREG_IO = pal[j & 255];
            PORT_NEXTREG_IO = (j >> 8) & 1;            
            j++;
        }
        j += 17;
        do_halt();
        do_halt();
    }
}

void testlayer2()
{
    unsigned short i, j, k;
    k = 0;
    PORT_NEXTREG_SELECT = 0x69;
    PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // enable layer 2
    
    PORT_NEXTREG_SELECT = 0x70;
    PORT_NEXTREG_IO = 0; // 256x192 resolution, palette offset 0

    // Set transparent color to 0   
    PORT_NEXTREG_SELECT = 0x14;
    PORT_NEXTREG_IO = 0;
    // Set fallback color to 0
    PORT_NEXTREG_SELECT = 0x4a;
    PORT_NEXTREG_IO = 0;
    
    // layer 2 is (by default) in banks 16-21
    PORT_NEXTREG_SELECT = 0x54; // bank 4 $8000-$9fff
    for (i = 0; i < 192; i++)
    {
        PORT_NEXTREG_IO = 18 + (i >> 5);
        k = (i & 31) << 8;
        k^=512;
        for (j = 0; j < 256; j++)
        {
            *((char*)0x8000 + ((j + k)^2)) = vortex[((i >> 1) << 7) | (j >> 1)];
        }
    }
    
    // select layer2 palette 0    
    PORT_NEXTREG_SELECT = 0x43;
    PORT_NEXTREG_IO = 0x10 | 1; // enable ulanext
    
    PORT_NEXTREG_SELECT = 0x42;
    PORT_NEXTREG_IO = 0xff; // ulanext color mask
    
    j = 0;
    while (1)
    {
        // Dump crap into palette
        PORT_NEXTREG_SELECT = 0x40;
        PORT_NEXTREG_IO = 0;    
        PORT_NEXTREG_SELECT = 0x44;    
        for (i = 0; i < 256; i++)
        {
            PORT_NEXTREG_IO = pal[j & 255];
            PORT_NEXTREG_IO = (j >> 8) & 1;            
            j++;
        }
        j += 17;
        do_halt();
        do_halt();
    }
}


void testlores_plus_layer2()
{
    unsigned short i, j, k;
    //writenextreg(0x15, readnextreg(0x15) | 0x80);
    
    //gPort254 = 0; // border to index 0
     // disable ULA
//     NEXTREG_SELECT= 0x68;
//     NEXTREG_IO = NEXTREG_IO | 0x80; // turn bit 7 on, disabling ula
    
    PORT_NEXTREG_SELECT = 0x15;
    PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // turn bit 7 on, enabling lores
    
    // The first 48 lines are stored between $4000 and $5800, and the second 48 between $6000 and $7800. 
    // Each byte is an index into the ULA palette. 
    for (i = 0x4000; i < 0x5800; i++)
       *((char*)i) = vortex[i-0x4000];
    for (i = 0x6000; i < 0x7800; i++)
       *((char*)i) = vortex[i-0x6000+0x1800];

//    for (i = 0x5800; i < 0x5B00; i++)
//       *((char*)i) = i;

    k = 0;
    PORT_NEXTREG_SELECT = 0x69;
    PORT_NEXTREG_IO = PORT_NEXTREG_IO | 0x80; // enable layer 2
    
    PORT_NEXTREG_SELECT = 0x70;
    PORT_NEXTREG_IO = 0; // 256x192 resolution, palette offset 0

    // Set transparent color to 0   
    PORT_NEXTREG_SELECT = 0x14;
    PORT_NEXTREG_IO = 0;
    // Set fallback color to 0
    PORT_NEXTREG_SELECT = 0x4a;
    PORT_NEXTREG_IO = 0;
    
    // layer 2 is (by default) in banks 16-21
    PORT_NEXTREG_SELECT = 0x54; // bank 4 $8000-$9fff
    for (i = 0; i < 192; i++)
    {
        PORT_NEXTREG_IO = 18 + (i >> 5);
        k = (i & 31) << 8;
        for (j = 0; j < 256; j++)
        {   
            unsigned char c = 0;
            unsigned short l = i;
            unsigned short m = j;
            if (l >= 96) 
                l = 191-l;
            if (m >= 128)
                m = 127 - m;
            c = vortex[(l << 7) | (m & 127)];
            *((char*)0x8000 + j + k) = c;
        }
    }
    
    // Set transparent color to 0   
    PORT_NEXTREG_SELECT = 0x14;
    PORT_NEXTREG_IO = 0;
    // Set fallback color to 0
    PORT_NEXTREG_SELECT = 0x4a;
    PORT_NEXTREG_IO = 0;
       
    PORT_NEXTREG_SELECT = 0x42;
    PORT_NEXTREG_IO = 0xff; // ulanext color mask
    
    j = 0;
    k = 0;
    while (1)
    {
        // select ULA palette 0    
        PORT_NEXTREG_SELECT = 0x43;
        PORT_NEXTREG_IO = 1; // enable ulanext
        // Dump crap into palette
        PORT_NEXTREG_SELECT = 0x40;
        PORT_NEXTREG_IO = 0;    
        PORT_NEXTREG_SELECT = 0x44;    
        for (i = 0; i < 256; i++)
        {
            PORT_NEXTREG_IO = pal[j & 255];
            PORT_NEXTREG_IO = (j >> 8) & 1;            
            j++;
        }
        j += 17;

        // select layer2 palette 0    
        PORT_NEXTREG_SELECT = 0x43;
        PORT_NEXTREG_IO = 0x10 | 1; // enable ulanext

        if (j & 1)
            {
        // Dump crap into palette
        PORT_NEXTREG_SELECT = 0x40;
        PORT_NEXTREG_IO = 0;    
        PORT_NEXTREG_SELECT = 0x44;    
        for (i = 0; i < 256; i++)
        {
            PORT_NEXTREG_IO = pal[k & 255] << 2;
            PORT_NEXTREG_IO = (k >> 8) & 1;            
            k++;
        }
        k += 256-17;
    }

        do_halt();
        do_halt();
    }
}

/*
 todo
 - sprites
 - tile mode
 - dma

*/
void main()
{ 
    //testlores();
    //testlayer2();
    testlores_plus_layer2();
    while (1);
}
