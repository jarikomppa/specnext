/* 
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

#include "yofstab.h"
#include "fona.h"

#define TIMEOUT 30000

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

extern unsigned char scr_x;
extern unsigned char scr_y;

#define SETXY(x,y) { scr_x = (x); scr_y = (y); }

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

void checkscroll()
{
    if (scr_y >= 24)
    {
        scrollup();
        scr_y -= 8;
    }
}

void print(char * t)
{
    while (*t)
    {
        if (*t == '\n')
        {
            scr_x = 0;
            scr_y++;
        }
        else
        {
            drawchar(*t, scr_x, scr_y);
            scr_x++;
            if (scr_x == 32)
            {
                scr_x = 0;
                scr_y++;
            }
        }
        checkscroll();
        t++;
    }
    scr_y++;
    scr_x = 0;
    checkscroll();
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

void printnum(unsigned long v)
{
    char temp[16];
    uitoa(v, temp);
    return print(temp);    
}

void flush_uart()
{
    while (readuarttx() & 1)
    {
        readuartrx();
    }
} 

// Do the maximum effort to empty the uart.
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
            unsigned char i = 1;
            while (i < blen && a[i] == b[i]) i++;
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
    unsigned short hdr = 0;
    unsigned short destsize = 0xfff;
    // Trying to keep this loop relatively tight, because
    // the uart fifo overrun is a real problem.
    while (timeout && ofs < 2048)
    {
        ofs += receive(buf + ofs);
        if (ofs >= destsize)
        {
            *len = hdr;
            return 0;
        }
        if (ofs > 2)
        {
            hdr = ((buf[0]<<8) | buf[1]);
            destsize = hdr;
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
retryatcmd:
    flush_uart();
    send(cmd, l);

    while (timeout && len < 2048)
    {        
        len += receive(buf + len);
        timeout--;
        if (strinstr(buf, "busy", len, 4))
            goto retryatcmd;
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
          "address)");
}

// max cmdlen = 9
void cipxfer(char *cmd, unsigned char cmdlen, unsigned char *output, unsigned short *len, unsigned char **dataptr)
{    
    *len = 0;
    flush_uart();
    send(cmd, cmdlen);
    if (bufinput(output, len)) return;
    *dataptr = output + 2; // skip size bytes
    *len -= 2; // reduce size bytes    
}

char gofast(char *inbuf)
{
    atcmd("AT+UART_CUR=1152000,8,1,0,0\r\n", "", 0, inbuf);
    setupuart(12);
    // Tests show that upping the uart speed didn't actually
    // affect the transfer rate. Let's keep it to 10x std
    // for stability.
    //atcmd("AT+UART_CUR=2000000,8,1,0,0\r\n", "", 0, inbuf);
    //setupuart(14);
    flush_uart_hard();
    if (atcmd("\r\n", "ERROR", 5, inbuf))
    {
        print("Can't talk to esp fast");
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
            *slash = 0xff; // makepath wants strings to end with 0xff
            makepath(fn);    
            *slash = '/';
        }
    }
    return fopen(fn, 2 + 0x0c); // if it still doesn't work, well, it doesn't.
}


void transfer(char *fn, char *inbuf)
{
    unsigned char *dp;
    unsigned long received = 0;
    unsigned short len;
    unsigned char filehandle;
    filehandle = createfilewithpath(fn);
    if (filehandle == 0)
    {
        print("Unable to open file");
    }
    else
    {            
        do
        {
            cipxfer("Get", 3, inbuf, &len, &dp);
retry:
            if (len && checksum(dp, len-2)==0)
            {
                received += len - 2;
                // skip every second print for tiny speedup (except the last print)
                if (len <= 2 || (received & 1024) == 0) 
                {
                    SETXY(5, scr_y);
                    printnum(received);
                    SETXY(scr_x, scr_y-1);
                }
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
    unsigned char *dp;
    unsigned short len;     
    unsigned char nextreg7;
    char filehandle;
    char retrycount;
    
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz

    // cls
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
          
    SETXY(0,0);
    
    print("NextSync 0.8 by Jari Komppa");
    print("http://iki.fi/sol");
    
    SETXY(0,3);
 
    len = parse_cmdline(fn);
    if (*fn)
    {
        print("Setting server to:");
        print(fn);
        print("-> "); SETXY(3, scr_y-1);
        print((char*)conffile);
        filehandle = createfilewithpath((char*)conffile);
        if (filehandle == 0)
        {
            print("Failed to open file");
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
    // set the baud rate (default)
    setupuart(0);

    if (atcmd("\r\n\r\n", "ERROR", 5, inbuf)) 
    {
        flush_uart_hard();
        if (atcmd("\r\n\r\n", "ERROR", 5, inbuf)) 
        {
            print("Can't talk to esp");
            goto bailout;
        }
    }
    
    if (gofast(inbuf))
        goto bailout;


    print("Connecting to "); SETXY(14, scr_y-1);
    print(fn);

    // Build the cipstart command in the huge buffer we have,
    // far enough that the atcommand shouldn't wreck it..
    memcpy(inbuf+2048, cipstart_prefix, 19);
    memcpy(inbuf+2048+19, fn, len);
    memcpy(inbuf+2048+19+len, cipstart_postfix, 9); // take care to copy the terminating zero

    // Try disconnecting just in case.
    atcmd("AT+CIPCLOSE\r\n", "ERROR", 5, inbuf);

    flush_uart_hard();

    if (atcmd(inbuf+2048, "OK", 2, inbuf))
    {
        print("Unable to connect");
        goto bailout;
    }

    print("Connected");
    
    if (atcmd("AT+CIPMODE=1\r\n", "OK", 2, inbuf))
    {
        print("Unable to set cip mode");
        goto closeconn;
    }
    
    // Let's go transparent
    if (atcmd("AT+CIPSEND\r\n", ">", 1, inbuf))
    {
        print("Failed to cipsend");
        goto closeconn;
    }
    
    retrycount = 0;
retryhandshake:
    // Check server version/request protocol
    cipxfer("Sync2", 5, inbuf, &len, &dp);
    //dp[len] = 0;
    //printnum(len);
    //print(dp);

    if (memcmp(dp, "NextSync2", 9) != 0)
    {
        retrycount++;
        if (retrycount < 20)
        {
            flush_uart_hard();
            goto retryhandshake;
        }
        print("Server version mismatch");
        dp[len] = 0;
        print(dp);
        printnum(len);
        print("");
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
                print("File:\nSize:\nXfer:"); SETXY(5, scr_y -3);
                print(fn); SETXY(5, scr_y);
                printnum(filelen);
                transfer(fn, inbuf);
                checkscroll();
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
    SETXY(0, scr_y+2);
    checkscroll();
    /*
    if (atcmd("AT+CIPCLOSE\r\n", "OK", 2, inbuf))
    {
        goto bailout;
    }*/
    cipxfer("Bye", 3, inbuf, &len, &dp);
    if (scr_y > 16)
    {
        scrollup();
        scr_y -= 8;
    }
    print("All done");
bailout:
    //atcmd("AT+CIPMODE=0\r\n", "OK", 0, inbuf); // restore CIP mode
    //atcmd("AT+UART_CUR=115200,8,1,0,0\r\n", "", 0, inbuf); // restore uart speed
    writenextreg(0x07, nextreg7); // restore cpu speed
    // reset esp
    writenextreg(0x02, 128);
    // wait for 5+ frames
    for (len = 0; len < 10000; len++);
    writenextreg(0x02, 0);
}