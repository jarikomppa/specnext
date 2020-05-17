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

unsigned char printn(char * t, char n, unsigned char x, unsigned char y)
{
    while (n)
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
        n--;
    }
    return y + 1;
}

unsigned char atoi(unsigned short v, char *b)
{
    unsigned short d = v;
    unsigned char p = 0;
    b[p] = '0';
    if (d >= 10000) { while (v >= 10000) { b[p]++; v -= 10000; } p++; b[p] = '0'; }
    if (d >= 1000) { while (v >= 1000) { b[p]++; v -= 1000; } p++; b[p] = '0'; }
    if (d >= 100) { while (v >= 100) { b[p]++; v -= 100; } p++; b[p] = '0'; }
    if (d >= 10) { while (v >= 10) { b[p]++; v -= 10; } p++; b[p] = '0'; }
    while (v >= 1) { b[p]++; v -= 1; } p++; 
    b[p] = 0;
    return p;
}

char printnum(unsigned short v, unsigned char x, unsigned char y)
{
    char temp[6];
    atoi(v, temp);
    return print(temp, x, y);    
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

void bufinput(unsigned char *buf, unsigned short *len)
{
    unsigned short timeout = 1000;
    unsigned char t;
    unsigned short ofs = 0;
    while (timeout)
    {
        t = readuarttx();
        if (t & 1)
        {
            ofs += receive(buf + ofs);
        }
        else
        {
            timeout--;
        }
    }    
    *len = ofs - 1;
}

unsigned char atcmd(char *cmd, char *expect, char *buf)
{
    unsigned short timeout = 1000;
    unsigned char t;
    unsigned char l = 0;
    while (cmd[l]) l++;
    send(cmd, l);
    readkeyboard();

    while (timeout && !KEYDOWN(SPACE))
    {        
        t = readuarttx();
        if (t & 1)
        {
            receive(buf);
            //print(buf, x, y);
            if (strinstr(buf, expect))
                return 0;
        }
        else
        {
            timeout--;
        }
        readkeyboard();
    }    
    return 1;
}


void cipxfer(char *cmd, unsigned short cmdlen, unsigned char *output, unsigned short *len, unsigned char **dataptr)
{    
    const char *cccmd="AT+CIPSEND=12345\r\n";
    char *cipsendcmd=(char*)cccmd;
    char p = 11;
    unsigned short l = cmdlen;
    p += atoi(cmdlen, cipsendcmd+p);
    cipsendcmd[p] = '\r'; p++;
    cipsendcmd[p] = '\n'; p++;
    send(cipsendcmd, p);
    bufinput(output, len);
    // todo: verify we have '>'
    send(cmd, cmdlen);
    bufinput(output, len);
    l = *len;
    while (*output != ':') 
    {
        output++;
        l--;
    }
    output++;
    *dataptr = output;
    *len = l;
}

void main()
{ 
    char inbuf[1024];
    char fn[128];
    unsigned char fnlen;
    unsigned short filelen;
    unsigned char x, y;   
    unsigned char *dp;
    unsigned short len; 
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
    x = 0;
    y = 0;
    
    y = print("NextSync 0.1 by Jari Komppa", x, y);
    y++;
    // select esp uart
    writeuartctl(0); 
    // set the baud rate
    setupuart();

    if (atcmd("\r\n", "ERROR", inbuf)) 
    {
        print("Can't talk to esp", 0, y);
        goto bailout;
    }
    atcmd("AT+CIPCLOSE\r\n", "", inbuf);
    //if (atcmd("AT+CIPSTART=\"TCP\",\"192.168.1.225\",2048\r\n", "OK", inbuf)) 
    if (atcmd("AT+CIPSTART=\"TCP\",\"DESKTOP-NAIUV3A\",2048\r\n", "OK", inbuf)) 
    {
        print("Unable to connect", 0, y);
        goto bailout;
    }
    
    // Check server version
    cipxfer("Sync", 4, inbuf, &len, &dp);
    y = printn(dp, len, x, y);
    printnum(len, x, y); y++;

    if (memcmp(dp, "NextSync1", 9) != 0)
    {
        print("Server version mismatch", 0, y);
        goto closeconn;
    }
    
    do
    {
        cipxfer("Next", 4, inbuf, &len, &dp);
        filelen = (dp[0] << 8) | dp[1];
        fnlen = dp[2];
        memcpy(fn, dp+3, fnlen);
        fn[fnlen] = 0;
        if (*fn)
        {
            print("File:", 0, y);
            y = print(fn, 5, y);
            print("Size:", 0, y);
            y = printnum(filelen, 5, y);
            // todo: xfer            
        }
    }
    while (*fn != 0);
    
closeconn:
    y++;
    if (atcmd("AT+CIPCLOSE\r\n", "OK", inbuf))
    {
        print("Close failed", 0, y);
        goto bailout;
    }
    print("All done", 0, y);
bailout:
    return;
}