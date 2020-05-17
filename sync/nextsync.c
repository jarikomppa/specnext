/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

#define HWIF_IMPLEMENTATION
#include "hwif.c"

#include "yofstab.h"
#include "fona.h"

extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);

extern void writenextreg(unsigned char reg, unsigned char val);
extern unsigned char readnextreg(unsigned char reg);
extern unsigned char allocpage();
extern void freepage(unsigned char page);

extern void writeuarttx(unsigned char val);
extern unsigned char readuarttx();
extern void writeuartrx(unsigned char val);
extern unsigned char readuartrx();
extern void writeuartctl(unsigned char val);
extern unsigned char readuartctl();
extern void setupuart();

extern unsigned short framecounter;
extern char *cmdline;

void drawchar(unsigned char c, unsigned char x, unsigned char y)
{
    unsigned char i;
    unsigned char *p = (unsigned char*)yofs[y] + x;
    unsigned short ofs = c * 8;
    for (i = 0; i < 8; i++)
    {
        *p = fona_png[ofs];
        ofs++;
        p += 256;
    }
}

unsigned char print(char * t, unsigned char x, unsigned char y)
{
    while (*t)
    {
        drawchar(*t, x, y);
        x++;
        if (x == 32)
        {
            x = 0;
            y++;
        }
        // todo: if y == 24, scroll up
        t++;
    }
    return y + 1;
}

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
    print(temp, x, y);    
}

void waitfordata()
{
    unsigned char t;
    do 
    {
        t = readuarttx();
    }
    while (!(t & 1));
}

unsigned short receive(char *b)
{
    unsigned char t;
    unsigned short count = 0;
    do 
    {
        t = readuarttx();
        if (t & 1)
        {
            *b = readuartrx();
            gPort254 = *b & 7;
            b++;
            count++;
        }
    }
    while (t & 1);
    *b = 0;
    gPort254 = 0;
    return count;
}

void send(const char *b, unsigned char bytes)
{
    unsigned char t;
    while (bytes)
    {
        // busy wait until byte is transmitted
        do
        {
            t = readuarttx();
        }
        while (t & 2);
        
        writeuarttx(*b);
        gPort254 = *b & 7;
        b++;
        bytes--;
    }
    gPort254 = 0;
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

unsigned char strinstr(char *a, char *b)
{
    if (!*b) return 1;
    while (*a)
    {
        if (*a == *b)
        {
            unsigned char i = 0;
            while (b[i] && a[i] == b[i]) i++;
            if (b[i] == 0)
                return 1;
        }
        a++;
    }
    return 0;
}

unsigned char atcmd(char *cmd, char *expect, unsigned char x, unsigned char y)
{
    char inbuf[128];
    unsigned char t;
    unsigned char l = 0;
    while (cmd[l]) l++;
    send(cmd, l);
    readkeyboard();

    while (!KEYDOWN(SPACE))
    {
        t = readuarttx();
        if (t & 1)
        {
            receive(inbuf);
            y = print(inbuf, x, y);
            if (strinstr(inbuf, expect))
                return y;            
        }
        readkeyboard();
    }    
    return 0;
}

void main()
{ 
    char inbuf[128];
    unsigned char x, y, t;    
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
    x = 0;
    y = 0;
    
    y = print("NextSync 0.1 by Jari Komppa", x, y);
    // select esp uart
    writeuartctl(0); 
    // set the baud rate
    setupuart();

    y = atcmd("\r\n", "ERROR", x, y);
    if (y == 0) goto bailout;
    atcmd("AT+CIPCLOSE\r\n","", x, y);
    y = atcmd("AT+CIPSTART=\"TCP\",\"192.168.1.225\",2048\r\n", "OK", x, y);
    if (y == 0) goto bailout;
    y = atcmd("AT+CIPSEND=7\r\n", ">", x, y);
    if (y == 0) goto bailout;
    send("Hallo\r\n", 7);
    readkeyboard();
    while (!KEYDOWN(SPACE))
    {
        t = readuarttx();
        if (t & 1)
        {
            receive(inbuf);
            print(inbuf, x, y); y++;    
        }
        readkeyboard();
    }
    atcmd("AT+CIPCLOSE\r\n","OK", x, y);
    /*
    waitfordata();
    print("Receiving", x, y); y++;
    receive(inbuf);
    print(inbuf, x, y); y++;    
    */
bailout:
    return;
}