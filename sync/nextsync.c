/* 
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

#include "yofstab.h"
#include "fona.h"

#define TIMEOUT 10000

// xxxsmbbb
// where b = border color, m is mic, s is speaker
__sfr __at 0xfe gPort254;

extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void makepath(char *pathspec); // must be 0xff terminated!

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
extern unsigned short receive(char *b);
extern char checksum(char *dp, unsigned short len);

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
        if (*t == '\n')
        {
            x = 0;
            y++;
        }
        else
        {
            drawchar(*t, x, y);
            x++;
            if (x == 32)
            {
                x = 0;
                y++;
            }
        }
        y = checkscroll(y);
        t++;
    }
    y++;
    y = checkscroll(y);
    return y;
}

unsigned char uitoa(unsigned long v, char *b)
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
    uitoa(v, temp);
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

void flush_uart()
{
    while (readuarttx() & 1)
    {
        readuartrx();
    }
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

unsigned char strinstr(char *a, char *b, unsigned short len, char blen)
{
    if (!*b || !blen) return 1;
    while (len)
    {
        if (*a == *b)
        {
            unsigned char i = 0;
            while (b[i] && a[i] == b[i]) i++;
            if (i >= blen)
                return 1;
        }
        a++;
        len--;
    }
    return 0;
}

char bufinput(char *buf, unsigned short *len)
{
    unsigned short timeout = TIMEOUT;
    unsigned short ofs = 0;
    unsigned short i = 0;
    unsigned short hdr = 0;
    unsigned short destsize = 0xfff;
    while (timeout)
    {
        ofs += receive(buf + ofs);
        for (; i < ofs; i++)
        {
            if (buf[i] == ':') // should get "recv nnn bytes\r\nSEND OK\r\n\r\n+IPD,nnn:"
            {
                while (timeout && ofs < 2048)
                {
                    ofs += receive(buf + ofs);
                    if (ofs >= destsize)
                    {
                        *len = hdr;
                        return 0;
                    }
                    if (ofs > i + 2)
                    {
                        hdr = ((buf[i+1]<<8) | buf[i+2]);
                        destsize = hdr + i + 1;
                    }
                    timeout--;
                }
            }
        }        
        
        timeout--;
    }    
    return 1;
}

unsigned char atcmd(char *cmd, char *expect, char expectlen, char *buf)
{
    unsigned short len = 0;
    unsigned short timeout = TIMEOUT;
    unsigned char l = 0;
        
    while (cmd[l]) l++;
    flush_uart();
    send(cmd, l);

    while (timeout && len < 2048)
    {        
        len += receive(buf + len);
        timeout--;
        if (strinstr(buf, expect, len, expectlen))
            return 0;
    }    
    return 1;
}

void noconfig()
{
        // 12345678901234567890123456789012
    print("Server configuration not found.\n\n"
          "Give server name or ip address\n"
          "as a parameter to create the\n"
          "server configuration.\n\n"
          "(Running nextsync.py shows\n"
          "both server name and the ip\n"
          "address)", 0, 3);
}

void cipxfer(char *cmd, unsigned short cmdlen, unsigned char *output, unsigned short *len, unsigned char **dataptr)
{    
    const char *cccmd="AT+CIPSEND=12345\r\n";
    char *cipsendcmd=(char*)cccmd;
    char p = 11;
    p += uitoa(cmdlen, cipsendcmd + p);
    cipsendcmd[p] = '\r'; p++;
    cipsendcmd[p] = '\n'; p++;
    cipsendcmd[p] = 0;
    atcmd(cipsendcmd, ">", 1, output); // cipsend prompt
    *len = 0;
    send(cmd, cmdlen);
    if (bufinput(output, len)) return;
    
    while (*output != ':') 
    {
        output++;
    }
    //output++;
    *dataptr = output + 2 + 1; // skip size bytes
    *len -= 2; // reduce size bytes    
}

void flush_uart_hard()
{
    unsigned short timeout = TIMEOUT;
    while (timeout)
    {
        if (readuarttx() & 1)
        {
            readuartrx();
            timeout = TIMEOUT;
        }
        timeout--;
    }
}

char gofast(char *inbuf, char y)
{
    atcmd("AT+UART_CUR=1152000,8,1,0,0\r\n", "", 0, inbuf);
    setupuart(12);
    //atcmd("AT+UART_CUR=2000000,8,1,0,0\r\n", "", 0, inbuf);
    //setupuart(14);
    flush_uart_hard();
    if (atcmd("\r\n\r\n", "ERROR", 5, inbuf))
    {
        print("Can't talk to esp fast", 0, y);
        return 1;
    }     
    return 0;
}

unsigned char createfilewithpath(char * fn)
{
    unsigned char filehandle;
    char * slash;
    filehandle = fopen(fn, 2 + 0x0c);  // write + create new file, delete existing
    if (filehandle) return filehandle;
    // Okay, couldn't create the file, so let's try to make the path.
    // We need to call makepath for each directory in the tree to build
    // complex paths.
    slash = fn;    
    while (*slash) 
    {
        slash++;
        if (*slash == '/')
        {
            *slash = 0xff;    
            makepath(fn);    
            *slash = '/';
        }
    }
    return fopen(fn, 2 + 0x0c); // if it still doesn't work, well, it doesn't.
}


void transfer(char *fn, char *inbuf, char y)
{
    unsigned char *dp;
    unsigned long received = 0;
    unsigned short len;
    unsigned char filehandle;
    filehandle = createfilewithpath(fn);
    if (filehandle == 0)
    {
        y = print("Unable to open file", 0, y);
    }
    else
    {            
        do
        {
            cipxfer("Get", 3, inbuf, &len, &dp);
retry:
            if (checksum(dp, len-2)==0)
            {
                received += len - 2;
                if (len <= 2 || (received & 1024) == 0) // skip every second print for tiny speedup
                    printnum(received, 5, y);
                fwrite(filehandle, dp, len-2);
            }
            else
            {
                flush_uart_hard();
                cipxfer("Retry", 5, inbuf, &len, &dp);
                goto retry;
            }
        } 
        while (len > 2);
        fclose(filehandle);
    }
}

void main()
{                                 //1234567890123456789012
    const char *cipstart_prefix  = "AT+CIPSTART=\"TCP\",\"";
    const char *cipstart_postfix = "\",2048\r\n";
    const char *conffile         = "c:/sys/config/nextsync.cfg";
    char inbuf[4096];    
    char fn[256];
    unsigned char fnlen;
    unsigned long filelen;
    unsigned char y;   
    unsigned char *dp;
    unsigned short len;     
    unsigned char nextreg7;
    char fastuart = 0;
    char filehandle;
    
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz

    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
          
    y = 0;
    
    y = print("NextSync 0.7 by Jari Komppa", 0, y);
    y++;
 
    len = parse_cmdline(fn);
    if (*fn)
    {
        y = print("Setting server to:", 0, y);
        y = print(fn, 0, y);
        y = print("-> ", 0 , y); y--;
        y = print((char*)conffile, 3, y);
        filehandle = createfilewithpath((char*)conffile);
        if (filehandle == 0)
        {
            y = print("Failed to open file", 0, y);
            goto bailout;
        }
        
        fwrite(filehandle, fn, len);
        fclose(filehandle);        
        goto bailout;
    }

    filehandle = fopen((char*)conffile, 1); // read + open existing
    if (filehandle == 0)        
    {
        noconfig();
        goto bailout;
    }
    len = fread(filehandle, fn, 255);
    fclose(filehandle);
    fn[len] = 0;
 
    // select esp uart, set 17-bit prescalar top bits to zero
    writeuartctl(16); 
    // set the baud rate
    setupuart(0);

    if (atcmd("\r\n\r\n", "ERROR", 5, inbuf)) 
    {
        // Maybe we're already going fast?
        fastuart = 1;
        setupuart(12);
        flush_uart_hard();
        if (atcmd("\r\n\r\n", "ERROR", 5, inbuf)) 
        {
            print("Can't talk to esp", 0, y);
            goto bailout;
        }
    }

    if (!fastuart && gofast(inbuf, y))
        goto bailout;

    // Try disconnecting just in case.
    atcmd("AT+CIPCLOSE\r\n\r\n", "ERROR", 5, inbuf);

    y = print("Connecting to ", 0, y); y--;
    y = print(fn, 14, y);

    memcpy(inbuf+1024, cipstart_prefix, 19);
    memcpy(inbuf+1024+19, fn, len);
    memcpy(inbuf+1024+19+len, cipstart_postfix, 9); // take care to copy the terminating zero

    if (atcmd(inbuf+1024, "OK", 2, inbuf))
    {
        print("Unable to connect", 0, y);
        goto bailout;
    }
    
    // Check server version/request protocol
    cipxfer("Sync2", 5, inbuf, &len, &dp);

    if (memcmp(dp, "NextSync2", 9) != 0)
    {
        y = print("Server version mismatch", 0, y);
        dp[len] = 0;
        y = print(dp, 0, y);
        y = printnum(len, 0, y); y++;
        goto closeconn;
    }
    
    do
    {        

        cipxfer("Next", 4, inbuf, &len, &dp);
retrynext:
        if (checksum(dp, len-2) == 0)
        {
            filelen = ((unsigned long)dp[0] << 24) | ((unsigned long)dp[1] << 16) | ((unsigned long)dp[2] << 8) | (unsigned long)dp[3];        
            fnlen = dp[4];
            memcpy(fn, dp+5, fnlen);
            fn[fnlen] = 0;
            if (*fn)
            {
                y = print("File:\nSize:\nXfer:", 0, y);
                print(fn, 5, y-3);
                printnum(filelen, 5, y-2);
                transfer(fn, inbuf, y-1);
                y = checkscroll(y);
            }
        }
        else
        {
            flush_uart_hard();
            cipxfer("Retry", 5, inbuf, &len, &dp);
            goto retrynext;
        }
    }
    while (*fn != 0);
    
closeconn:
    y+=2;
    y = checkscroll(y);
    if (atcmd("AT+CIPCLOSE\r\n", "OK", 2, inbuf))
    {
        y=print("Close failed", 0, y);
        goto bailout;
    }
    if (y > 16)
    {
        scrollup();
        y -= 8;
    }
    print("All done", 0, y);
bailout:
    atcmd("AT+UART_CUR=115200,8,1,0,0\r\n", "", 0, inbuf); // restore uart speed
    writenextreg(0x07, nextreg7); // restore speed
    return;
}