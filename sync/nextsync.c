/* 
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

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
extern void setupuart(unsigned char rateindex) __z88dk_fastcall;

extern void memcpy(char *dest, const char *source, unsigned short count);

extern unsigned short framecounter;
extern char *cmdline;

unsigned char parse_cmdline(char *f)
{
    unsigned char i;
    i = 0;
    while (cmdline && i < 127 && cmdline[i] != 0 && cmdline[i] != 0xd && cmdline[i] != ':')
    {
        f[i] = cmdline[i];
        i++;
    }
    f[i] = 0;
    return i;
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

void scrollup()
{
    unsigned char i, j;

    for (i = 0; i < 16; i++)
    {
        unsigned char* src = (unsigned char*)yofs[i+8];
        unsigned char* dst = (unsigned char*)yofs[i];
        for (j = 0; j < 8; j++)
        {
            memcpy(dst, src, 32);
            src += 256;
            dst += 256;
        }
    }
    for (i = 16; i < 24; i++)
    {
        unsigned char* dst = (unsigned char*)yofs[i];
        for (j = 0; j < 8; j++)
        {
            memset(dst, 0, 32);
            dst += 256;
        }
    }
}

unsigned char checkscroll(unsigned char y)
{
    // todo: if y == 24, scroll up
    if (y >= 24)
    {
        scrollup();
        y -= 8;
    }
    return y;
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
        y = checkscroll(y);
        t++;
    }
    y++;
    y = checkscroll(y);
    return y;
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
        y = checkscroll(y);
        t++;
        n--;
    }
    y++;
    y = checkscroll(y);
    return y;
}

unsigned char atoi(unsigned long v, char *b)
{
    unsigned long d = v;
    unsigned char dig = 0;
    unsigned long tt[] = 
    {
        1000000000,
        100000000,
        10000000,
        1000000,
        100000,
        10000,
        1000,
        100,
        10,
        1,
        0
    };
    unsigned char p = 0;    
    b[p] = '0';
    if (v != 0)
    {
        do 
        {
            unsigned long t = tt[dig];
            if (d >= t) { while (v >= t) { b[p]++; v -= t; } p++; b[p] = '0'; }        
            dig++;
        }  
        while (tt[dig] > 0);    
    }
    else
    {
        p++;
    }
    b[p] = 0;
    return p;
}

char printnum(unsigned long v, unsigned char x, unsigned char y)
{
    char temp[16];
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
    unsigned short timeout = 100; 
    do 
    {
        t = readuarttx();
        if (t & 1)
        {
            *b = readuartrx();
            gPort254 = *b & 7;
            b++;
            count++;
            timeout = 100; // TODO: figure out how low we can go with this reliably
        }
        // Without timeout it's possible we empty the uart and stop
        // receiving before it's ready.
        timeout--;
    }
    while (timeout);
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

unsigned char strinstr(char *a, char *b, unsigned short len)
{
    if (!*b) return 1;
    while (len)
    {
        if (*a == *b)
        {
            unsigned char i = 0;
            while (b[i] && a[i] == b[i]) i++;
            if (b[i] == 0)
                return 1;
        }
        a++;
        len--;
    }
    return 0;
}

char bufinput(char *buf, char *expect, unsigned short *len)
{
    unsigned short timeout = 20000;
    unsigned char t;
    unsigned short ofs = 0;
    while (timeout)
    {
        t = readuarttx();
        if (t & 1)
        {
            ofs += receive(buf + ofs);
            //printn(buf, ofs, 0, 10);
            *len = ofs - 1;
            if (strinstr(buf, expect, ofs))
            {
                return 0;
            }
            
            timeout = 20000;
        }
        else
        {
            timeout--;
        }
    }    
    return 1;
}

unsigned char atcmd(char *cmd, char *expect, char *buf)
{
    unsigned short len = 0;
    unsigned short timeout = 20000;
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
            len += receive(buf);
            //printn(buf, len, 0, 10);
            if (strinstr(buf, expect, len))
                return 0;
            timeout = 20000;
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
    bufinput(output, ">", len); // cipsend prompt
    send(cmd, cmdlen);
    if (bufinput(output, ":", len)) return; // should get "recv nnn bytes\r\nSEND OK\r\n\r\n+IPD,nnn:"
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
                                        //1234567890123456789012
    static const char *cipstart_prefix  = "AT+CIPSTART=\"TCP\",\"";
    static const char *cipstart_postfix = "\",2048\r\n";
    static const char *conffile         = "c:/sys/config/nextsync.cfg";
    char inbuf[2048];    
    char fn[256];
    unsigned char fnlen;
    unsigned long filelen;
    unsigned long received;
    unsigned char x, y;   
    unsigned char *dp;
    unsigned short len;     
    unsigned char nextreg7;
    char filehandle;
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
    
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz
          
    x = 0;
    y = 0;
    
    y = print("NextSync 0.5 by Jari Komppa", x, y);
    y++;
 
    len = parse_cmdline(fn);
    if (*fn)
    {
        y = print("Setting server to:", 0, y);
        y = print(fn, 0, y);
        y = print("-> ", 0 , y); y--;
        y = print((char*)conffile, 3, y);
        filehandle = fopen((char*)conffile, 2 + 8); // write + open existing or create file
        if (filehandle == 0)
        {
            y = print("Failed to open file", 0, y);
            goto bailout;
        }
        fwrite(filehandle, fn, len);
        fclose(filehandle);        
        goto bailout;
    }

    filehandle = fopen((char*)conffile, 1);
    if (filehandle == 0)
    {           // 12345678901234567890123456789012
        y = print("Server configuration not found.", 0, y);
        y++; y = checkscroll(y);
        y = print("Give server name or ip address", 0, y);
        y = print("as a parameter to create the", 0, y);
        y = print("server configuration.", 0, y);
        y++; y = checkscroll(y);
        y = print("(Running nextsync.py shows", 0, y);
        y = print("both server name and the ip", 0, y);
        y = print("address)", 0, y);
        goto bailout;
    }
    len = fread(filehandle, fn, 255);
    fclose(filehandle);
    fn[len] = 0;
 
    // select esp uart
    writeuartctl(0); 
    // set the baud rate
    setupuart(0);

    if (atcmd("\r\n\r\n", "ERROR", inbuf)) 
    {
        print("Can't talk to esp", 0, y);
        goto bailout;
    }
    
    atcmd("AT+UART_CUR=1152000,8,1,0,0\r\n", "", inbuf);
    setupuart(12);
    if (atcmd("\r\n\r\n", "ERROR", inbuf))
    {
        print("Can't talk to esp fast", 0, y);
        goto bailout;
    }   

    // Try disconnecting just in case.
    atcmd("AT+CIPCLOSE\r\n\r\n", "ERROR", inbuf);

    y = print("Connecting to:", 0, y);
    y = print(fn, 0, y);

    memcpy(inbuf+1024, cipstart_prefix, 19);
    memcpy(inbuf+1024+19, fn, len);
    memcpy(inbuf+1024+19+len, cipstart_postfix, 8);

    if (atcmd(inbuf+1024, "OK", inbuf))
    {
        print("Unable to connect", 0, y);
        goto bailout;
    }
    
    // Check server version/request protocol
    cipxfer("Sync1", 5, inbuf, &len, &dp);

    if (memcmp(dp, "NextSync1", 9) != 0)
    {
        y = print("Server version mismatch", 0, y);
        y = printn(dp, len, x, y);
        y = printnum(len, x, y); y++;
        goto closeconn;
    }
    
    do
    {
        cipxfer("Next", 4, inbuf, &len, &dp);
        filelen = ((unsigned long)dp[0] << 24) | ((unsigned long)dp[1] << 16) | ((unsigned long)dp[2] << 8) | (unsigned long)dp[3];
        fnlen = dp[4];
        memcpy(fn, dp+5, fnlen);
        fn[fnlen] = 0;
        if (*fn)
        {
            y = print("File:", 0, y);
            y--;
            y = print(fn, 5, y);
            y = print("Size:", 0, y);
            y--;
            y = printnum(filelen, 5, y);
            received = 0;
retry:
            filehandle = fopen(fn, 2 + 8); // write + open existing or create file
            if (filehandle == 0)
            {
                y = print("Unable to open file", 0, y);
            }
            else
            {            
                do
                {
                    unsigned char checksum = 0;
                    unsigned short i;
                    cipxfer("Get", 3, inbuf, &len, &dp);
                    for (i = 0; i < len - 1; i++)
                        checksum ^= dp[i];
                    received += len - 1;
                    if (checksum == dp[len-1])
                    {
                        y = print("Xfer:", 0, y);
                        y--;
                        printnum(received, 5, y);
                        fwrite(filehandle, dp, len-1);
                    }
                    else
                    {
                        received = 0;
                        cipxfer("Retry", 5, inbuf, &len, &dp);
                        fclose(filehandle);
                        goto retry;
                    }
                } 
                while (len > 1);
                fclose(filehandle);
                y++;
            }
        }
    }
    while (*fn != 0);
    
closeconn:
    y+=2;
    y = checkscroll(y);
    if (atcmd("AT+CIPCLOSE\r\n", "OK", inbuf))
    {
        y=print("Close failed", 0, y);
        goto bailout;
    }
    if (y > 16)
    {
        scrollup();
        y -= 8;
    }
    y=print("All done", 0, y);
bailout:
    atcmd("AT+UART_CUR=115200,8,1,0,0\r\n", "", inbuf);
    writenextreg(0x07, nextreg7); // restore speed
    return;
}