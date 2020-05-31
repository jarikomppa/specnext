/* 
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

extern const unsigned short yofs[];

extern const unsigned char fona_png[];

#define TIMEOUT 20000
#define TIMEOUT_FLUSHUART 10000

//#define SYNCSLOW
//#define SYNCFAST

//#define DEBUGMODE
//#define DISKLOG

#ifdef DEBUGMODE
#define SETX(x) 
#define SETY(y) 
#else
#define SETX(x) scr_x = (x)
#define SETY(y) scr_y = (y)
#endif


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
extern unsigned short mulby10(unsigned short input) __z88dk_fastcall;

extern unsigned short framecounter;
extern char *cmdline;
extern unsigned char scr_x;
extern unsigned char scr_y;
extern char dbg;

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

#ifdef DEBUGMODE
void drawcharx(unsigned char c)
{
    unsigned char i;
    unsigned char *p = (unsigned char*)yofs[scr_y] + scr_x;
    unsigned short ofs = c * 8;
    for (i = 0; i < 8; i++)
    {
        *p = fona_png[ofs] ^ 0xff;
        ofs++;
        p += 256;
    }
}
#endif

extern void drawchar(unsigned char c);

extern void scrollup();

extern void checkscroll();

void print(char * t)
{
#ifdef DEBUGMODE
t;
#else
    while (*t)
    {
        if (*t == '\n')
        {
            scr_x = 0;
            scr_y++;
        }
        else
        {
            drawchar(*t);
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
#endif
}

#ifdef DEBUGMODE
void putchar(char t)
{
    drawchar(t);
    scr_x++;
    if (scr_x == 32)
    {
        scr_x = 0;
        scr_y++;
    }
    checkscroll();
}

void putcharx(char t)
{
    drawcharx(t);
    scr_x++;
    if (scr_x == 32)
    {
        scr_x = 0;
        scr_y++;
    }
    checkscroll();
}
#endif

extern unsigned char uitoa(unsigned long v, char *b);

void printnum(unsigned long v);

void flush_uart()
{
#ifdef DISKLOG
    fwrite(dbg, "[fu]", 4);
#endif    
    while (readuarttx() & 1)
    {
#ifdef DEBUGMODE
        putcharx(readuartrx());
#elif defined(DISKLOG)
        char t = readuartrx();
        fwrite(dbg, &t, 1);
#else        
        readuartrx();
#endif        
    }
} 

// Do the maximum effort to empty the uart.
void flush_uart_hard()
{
    unsigned short timeout = TIMEOUT_FLUSHUART;
#ifdef DISKLOG
    fwrite(dbg, (char*)"[fuh]", 5);
#endif    
    while (timeout)
    {
        if (readuarttx() & 1)
        {
#ifdef DEBUGMODE
            putcharx(readuartrx());
#elif defined(DISKLOG)
            char t = readuartrx();
            fwrite(dbg, &t, 1);
#else        
            readuartrx();
#endif        
            timeout = TIMEOUT_FLUSHUART;
        }
        timeout--;
    }
}


#if defined(DEBUGMODE)
unsigned short receive(unsigned char *b)
{
    unsigned short count = 0;
    while (readuarttx() & 1)
    {
        *b = readuartrx();
        count++;
        putchar(*b);
        b++;
    }
    return count;
} 
#endif

unsigned char receive_slow()
{
    unsigned short timeout = TIMEOUT;
    while (timeout && !(readuarttx() & 1)) { timeout--; } // wait for data.
    return readuartrx(); // returns 0 on no data
}

void send(const char *b, unsigned char bytes)
{
    unsigned short timeout = TIMEOUT;
    unsigned char t;
#ifdef DISKLOG
    fwrite(dbg, (char*)"[s]", 3);
    fwrite(dbg, (char*)b, bytes);
#endif    
    while (timeout && bytes)
    {
        // busy wait until byte is transmitted
        do
        {
            timeout--;
            t = readuarttx();
        }
        while (timeout && t & 2);
        
        writeuarttx(*b);

        gPort254 = *b & 7;
        b++;
        bytes--;
    }
    gPort254 = 0;
}

extern unsigned char strinstr(char *a, char *b, unsigned short len, char blen);

// Anatomy of a cipxfer:
// [s]"AT+CIPSENDEX=5\r\n"
// [at]"AT+CIPSENDEX=5\r\r\n\r\nOK\r\n> "
// [s]"Sync3"
// [bi]"\r\nRecv 5 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,14:\0\x0eNextSync33\x0a\0"
unsigned short bufinput(char *buf)
{
    unsigned short timeout = TIMEOUT;
    unsigned short datalen = 0;
    unsigned short ofs = 0;
    unsigned char r;
    while (timeout && receive_slow() != '+') { timeout--; }
    // TODO: size/speed opt
    if (receive_slow() != 'I') return 0; // should be I
    if (receive_slow() != 'P') return 0; // should be P
    if (receive_slow() != 'D') return 0; // should be D
    if (receive_slow() != ',') return 0; // should be ,
    datalen = receive_slow() - '0'; // first digit
    r = receive_slow();
    while (r != ':')
    {
        datalen = mulby10(datalen);
        datalen += r - '0';
        r = receive_slow();
        if (r != ':' && (r < '0' || r > '9')) return 0;
    }
    if (datalen > 2048 || datalen == 0) return 0;
    do
    {
        ofs += receive(buf + ofs);
        timeout--;
    }
    while (timeout && ofs < datalen);
    return ofs;    
}

unsigned char atcmd(char *cmd, char *expect, char expectlen, char *buf)
{
    unsigned short len = 0;
    unsigned short timeout = TIMEOUT;
    unsigned short retrycount = 100;
    unsigned char l = 0;
        
    while (cmd[l]) l++;        
retryatcmd:
    flush_uart();
    send(cmd, l);      

    while (timeout && len < 2048)
    {        
        len += receive(buf + len);
        timeout--;
        if (strinstr(buf, expect, len, expectlen))
        {
#ifdef DISKLOG
        fwrite(dbg, "[at]", 4);
        fwrite(dbg, buf, len);
#endif
            return 0;
        }
        if (strinstr(buf, "busy", len, 4))
        {
            if (!retrycount)
                return 1;
#ifdef DISKLOG
        fwrite(dbg, "[atb]", 5);
        fwrite(dbg, buf, len);
#endif
            len = 0;
            retrycount--;
            goto retryatcmd;
        }
    }    
#ifdef DISKLOG
    fwrite(dbg, "[at0]", 5);
    fwrite(dbg, buf, len);
#endif
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
    const char *cipsendcmd_c="AT+CIPSENDEX=0\r\n";
    char *cipsendcmd = (char *)cipsendcmd_c;
    unsigned short received, expected;
    unsigned short timeout = 16; // relatively small timeout needed because bufinput has timeout
    cipsendcmd[13] = '0' + cmdlen;
    *len = 0;
    if (atcmd(cipsendcmd, ">", 1, output)) // cipsend prompt
    {
        return;
    }
    flush_uart();
    expected = 2; // always expect at least 2 bytes. Actually, we should expect at least 5.. size+checksums
    received = 0;
    send(cmd, cmdlen);
    do 
    {
        unsigned short r = bufinput(output + received);
        received += r;
        if (expected == 2 && received > 2)
        {
            expected = ((output[0]<<8) | output[1]);
            if (expected < 5 || expected > 2048)
            {
                return;
            }
        }
        timeout--;
    }
    while (timeout && received < expected);
    *dataptr = output + 2; // skip size bytes
    *len = received - 2; // reduce size bytes    
}

char gofast(char *inbuf)
{
#ifdef SYNCFAST    
    #define FAST_UART_MODE 14    
    atcmd("AT+UART_CUR=2000000,8,1,0,0\r\n", "", 0, inbuf);
#else    
    #define FAST_UART_MODE 12
    atcmd("AT+UART_CUR=1152000,8,1,0,0\r\n", "", 0, inbuf);
#endif    

    setupuart(FAST_UART_MODE);
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
    unsigned char packetno = 0;
restart:
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
            if (dp[len-1] != packetno)
            {
                flush_uart_hard();
                cipxfer("Restart", 7, inbuf, &len, &dp);
                fclose(filehandle);
                len = 0;
                packetno = 0;
                received = 0;
                goto restart;
            }
retry:
            if (len && checksum(dp, len-3)==0)
            {
                received += len - 3;
                // skip every second print for tiny speedup (except the last print)
                if (len <= 3 || (received & 1024) == 0) 
                {
                    SETX(5);
                    printnum(received);
                    SETY(scr_y -1);
                }
                fwrite(filehandle, dp, len-3);
                packetno++;
            }
            else
            {                
                flush_uart_hard();
                cipxfer("Retry", 5, inbuf, &len, &dp);
                goto retry;
            }
        } 
        while (len > 3);
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
    char fastuart = 0;
    char filehandle;
    char retrycount;
    
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz

    // cls
    memset((unsigned char*)yofs[0],0,192*32);
    memset((unsigned char*)yofs[0]+192*32,4,24*32);
          
    SETX(0);
    
    print("NextSync 0.9 by Jari Komppa");
    print("http://iki.fi/sol");
    SETY(scr_y+1);
 
    len = parse_cmdline(fn);
    if (*fn)
    {
        print("Setting server to:");
        print(fn);
        print("-> "); SETY(scr_y-1); SETX(3);
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
 
#ifdef DISKLOG
    dbg = createfilewithpath((char*)"syncdebug.dat");
#endif 
    // select esp uart, set 17-bit prescalar top bits to zero
    writeuartctl(16); 
    // set the baud rate (default)
    setupuart(0);

    if (atcmd("\r\n", "ERROR", 5, inbuf)) 
    {
#ifndef SYNCSLOW
        // Maybe we're already going fast?
        fastuart = 1;
        setupuart(FAST_UART_MODE);
        flush_uart_hard();
        if (atcmd("\r\n", "ERROR", 5, inbuf)) 
        {
#endif
            print("Can't talk to esp.\nResetting esp, try again.");
            // reset esp
            writenextreg(0x02, 128);
            // wait for 5+ frames
            for (len = 0; len < 10000; len++);
            writenextreg(0x02, 0);
            goto bailout;
#ifndef SYNCSLOW
        }
#endif        
        // if we get this far, esp was already at the fast rate
        // (which can happen if you reset the next while
        // transfer is going on)
    }

#ifndef SYNCSLOW
    if (!fastuart && gofast(inbuf))
        goto bailout;
#endif

    atcmd("ATE0\r\n", "OK", 2, inbuf); // command echo off; if on, we might match server name as OK/ERROR/BUSY =)

    print("Connecting to "); SETY(scr_y-1); SETX(14);
    print(fn);

retryconnect:

    // Try disconnecting a few times just in case.
    retrycount = 10;
    while (retrycount && atcmd("AT+CIPCLOSE\r\n", "ERROR", 5, inbuf)) { retrycount--; }

    // Build the cipstart command in the huge buffer we have,
    // far enough that the atcommand shouldn't wreck it..
    memcpy(inbuf+2048, cipstart_prefix, 19);
    memcpy(inbuf+2048+19, fn, len);
    memcpy(inbuf+2048+19+len, cipstart_postfix, 9); // take care to copy the terminating zero

    if (atcmd(inbuf+2048, "OK", 2, inbuf))
    {
        print("Unable to connect");
        goto bailout;
    }
        
    print("Handshake..");
    
    retrycount = 0;
retryhandshake:
    // Check server version/request protocol
    cipxfer("Sync3", 5, inbuf, &len, &dp);

    if (len < 9 || memcmp(dp, "NextSync3", 9) != 0)
    {        
        retrycount++;
        if (retrycount < 5)
        {
            if (len == 0)
            {
                print("Retry connect..");
                goto retryconnect;
            }
            flush_uart_hard();
            print("Retry handshake..");
            goto retryhandshake;
        }
        print("Server version mismatch");
        dp[len] = 0;
        print(dp);
        printnum(len); SETY(scr_y+1);
        goto closeconn;
    }

    print("Connected\n");
    
    do
    {        

        cipxfer("Next", 4, inbuf, &len, &dp);
retrynext:
        if (checksum(dp, len-3) == 0)
        {
            filelen = ((unsigned long)dp[0] << 24) | ((unsigned long)dp[1] << 16) | ((unsigned long)dp[2] << 8) | (unsigned long)dp[3];        
            fnlen = dp[4];
            memcpy(fn, dp+5, fnlen);
            fn[fnlen] = 0;
            if (*fn)
            {
                print(fn);
                print("Size:\nXfer:"); SETX(5); SETY(scr_y - 2);
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
    SETY(scr_y + 2);
    checkscroll();
    print("Shutting down..");
    cipxfer("Bye", 3, inbuf, &len, &dp);
    atcmd("AT+CIPCLOSE\r\n", "", 0, inbuf);
    if (scr_y > 16)
    {
        scrollup();
        scr_y -= 8;
    }
bailout:
    atcmd("AT+UART_CUR=115200,8,1,0,0\r\n", "", 0, inbuf); // restore uart speed
    print("All done");
    writenextreg(0x07, nextreg7); // restore cpu speed
#ifdef DISKLOG
    fclose(dbg);
#endif    
}