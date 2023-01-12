/* 
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

/*
One of the reasons why this code is confusing is that there's various
layers in play.

- Dot command harness. For a while we can exit cleanly without much of a hassle.
  Some things are done at this level, like printing out help.
- Runtime proper. Have altered system state, needs a lot of cleanup. This starts from
  printversion() onwards.
- Talking to UART. Things may go wrong here. Different UART speeds need different
  settings, and these are not exact, so some drifting may happen if we're unlucky 
  (or have calculated things wrong). 
- Talking to ESP through UART. A lot of things can go wrong here. There are different
  ESP firmwares that may act slightly differently. Network environment may change the
  behavior too.
- Talking to nextsync server through ESP through UART. The nextsync server is, luckily,
  something we control again so it's not a complete unknown. Also, network stacks on
  PCs are more stable than what we have on the speccy side so we can largely ignore it.  

Memory map plan:

bank 0: rom
bank 1: dot

bank 2: ULA screen, system vars
bank 3: free(?)

bank 4: output buffer
bank 5: output buffer

bank 6: input buffer
bank 7: input buffer


*/

#include "nextsync.h"

#define TIMEOUT 20000
#define TIMEOUT_FLUSHUART 10000

//#define SYNCSLOW
//#define SYNCFAST

// See calc_prescalar.c for the prescalar calculation code
static const unsigned short prescalar_values[] = {
  243,   248,   255,   260,   269,   277,   286,   234, // (0) 115200
  486,   496,   511,   520,   538,   555,   572,   468, // (1) 57600
  729,   744,   767,   781,   807,   833,   859,   703, // (2) 38400
  896,   914,   942,   960,   992,  1024,  1056,   864, // (3) 31250
 1458,  1488,  1534,  1562,  1614,  1666,  1718,  1406, // (4) 19200
 2916,  2976,  3069,  3125,  3229,  3333,  3437,  2812, // (5) 9600
 5833,  5952,  6138,  6250,  6458,  6666,  6875,  5625, // (6) 4800
11666, 11904, 12276, 12500, 12916, 13333, 13750, 11250, // (7) 2400
  121,   124,   127,   130,   134,   138,   143,   117, // (8) 230400
   60,    62,    63,    65,    67,    69,    71,    58, // (9) 460800
   48,    49,    51,    52,    53,    55,    57,    46, // (10) 576000
   30,    31,    31,    32,    33,    34,    35,    29, // (11) 921600
   24,    24,    25,    26,    26,    27,    28,    23, // (12) 1152000
   18,    19,    19,    20,    20,    21,    22,    18, // (13) 1500000
   14,    14,    14,    15,    15,    16,    16,    13  // (14) 2000000    
};

// Uart setup based on code by D. ‘Xalior’ Rimron-Soutter
void setupuart(char mode)
{
   unsigned short prescalar = prescalar_values[mode * 8 + (readnextreg(0x11) & 0x07)];
   
   UART_CTL = (UART_CTL & 0x40) | 0x10 | (unsigned char)(prescalar >> 14);
   UART_RX = 0x80 | (unsigned char)(prescalar >> 7);
   UART_RX = (unsigned char)(prescalar) & 0x7f;
}

// Just flush as much as is in the queue right now.
void flush_uart()
{
    while (UART_TX & 1)
    {
        UART_RX;
    }
} 

// Do the maximum effort to empty the uart.
void flush_uart_hard()
{
    unsigned short timeout = TIMEOUT_FLUSHUART;
    while (timeout)
    {
        if (UART_TX & 1)
        {
            UART_RX;
            timeout = TIMEOUT_FLUSHUART;
        }
        timeout--;
    }
}

unsigned char receive_slow()
{
#ifdef SYNCSLOW
    unsigned short timeout = 200;
#else
    unsigned short timeout = 20;
#endif    
    while (timeout && !(UART_TX & 1)) 
    { 
        // wait for data.
        timeout--; 
    }
    if (!timeout) return 0;
    return UART_RX;
}

void send(const char *b, unsigned char bytes)
{
    unsigned short timeout = TIMEOUT;
    unsigned char t;
    while (timeout && bytes)
    {
        // busy wait until byte is transmitted
        do
        {
            timeout--;
            t = UART_TX;
        }
        while ((t & 2) && timeout); // bit 1 = 1 if the Tx buffer is full
        
        UART_TX = *b;

        gPort254 = *b & 7;
        b++;
        bytes--;
    }
    gPort254 = 0;

    // On later core versions, UART Tx buffer size is 64 not 1, so bytes are accepted faster but still
    // sent at the same rate. To preserve previous timings, wait for buffer to empty before continuing.
    // On core versions where flag bit 4 does not exist yet, skip this Tx buffer flush.
    if (corever >= 0x310a) // 3.01.10
    {
        timeout = TIMEOUT;
        do
        {
            timeout--;
            t = UART_TX;       
        }
        while (!(t & 16) && timeout); // bit 4 = 1 if the Tx buffer is empty
    }
}

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
    while (r != ':') // TODO: possible inf loop
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
            return 0;
        }
        if (strinstr(buf, "busy", len, 4))
        {
            if (!retrycount)
                return 1;
            len = 0;
            retrycount--;
            goto retryatcmd;
        }
    }    
    return 1;
}

// max cmdlen = 9
void cipxfer(char *cmd, unsigned char cmdlen, unsigned char *output, unsigned short *len, unsigned char **dataptr)
{    
    const char *cipsendcmd_c="AT+CIPSENDEX=0\r\n";
    char *cipsendcmd = (char *)cipsendcmd_c;
    unsigned short received, expected;
    unsigned short timeout = 5; // relatively small timeout needed because bufinput has timeout
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

#ifndef SYNCSLOW
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
        println("Can't talk to esp fast");
        return 1;
    }     
    return 0;
}
#endif

char transfer(char *fn, unsigned char *inbuf)
{
    unsigned char *dp;
    unsigned long received = 0;
    unsigned short len;
    unsigned char filehandle;
    unsigned char packetno = 0;
    unsigned char failcount = 0;

restart:
    filehandle = createfilewithpath(fn);
    if (filehandle == 0)
    {
        println("Unable to open file");
        return 0;
    }

    do
    {
        cipxfer("Get", 3, inbuf, &len, &dp);
retry:
        if (dp[len - 1] != packetno)
        {
            if (len == 5+3 && checksum(dp, len - 3) == 0 && memcmp(dp, "Error", 5) == 0)
            {
                goto doretry;
            }
            flush_uart_hard();
            cipxfer("Restart", 7, inbuf, &len, &dp);
            fclose(filehandle);
            len = 0;
            packetno = 0;
            received = 0;
            failcount++;
            if (failcount > 5) goto failure;                        
            goto restart;
        }
        
        if (len && checksum(dp, len - 3) == 0)
        {
            len -= 3;
            received += len;
            fwrite(filehandle, dp, len);
            packetno++;
            failcount = 0;
        }
        else
        {                
doretry:
            failcount++;
            if (failcount > 5) goto failure;
            flush_uart_hard();
            cipxfer("Retry", 5, inbuf, &len, &dp);
            goto retry;
        }
    } 
    while (len != 0);

    fclose(filehandle);
    return 0;
failure:
    fclose(filehandle);
    return 1;
}

void main()
{                                 //1234567890123456789012
    const char *cipstart_prefix  = "AT+CIPSTART=\"TCP\",\"";
    const char *cipstart_postfix = "\",2048\r\n";
    const char *conffile         = "c:/sys/config/nextsync.cfg";
    char fn[256];
    char inbuf[2048];
    char scratch[2048];
    unsigned char fnlen;
    unsigned long filelen;
    unsigned char *dp;
    unsigned short len = 0;     
    unsigned char nextreg6;
    unsigned char nextreg7;
    char fastuart = 0;
    char filehandle;
    char retrycount;
	
	scr_ct = *((char *)23692); // grab old value of screen counter

    len = parse_cmdline(fn);

    if (!len)
        filehandle = fopen((char*)conffile, 1); // read + open existing
        
    if ((len && fn[0] == '-') || (!len && filehandle == 0))
    {
        // Probably asking for help.
		printhelp();
        goto terminate;
    }
        
    if (*fn)
    {
        conprint("Setting server to:");
        conprint(fn);
        conprint("\r-> ");
        conprint((char*)conffile);
        conprint("\r");
        memcpy((char*)inbuf, (char*)conffile, 27);     // Constants are located below $4000, so copy
        filehandle = createfilewithpath((char*)inbuf); // filename into temp buffer to keep IDE_PATH happy.
        if (filehandle == 0)
        {
            conprint("Failed to open file\r");
            goto terminate;
        }
        
        fwrite(filehandle, fn, len);
        fclose(filehandle);        
        conprint("Ok\r");
        goto terminate;
    }

    len = fread(filehandle, fn, 255);
    fclose(filehandle);
    fn[len] = 0;

    nextreg6 = readnextreg(0x06);
    writenextreg(0x06, nextreg6 & 0x7d); // disable turbo key & 50/60 switch (leave other bits alone)
    nextreg7 = readnextreg(0x07);
    writenextreg(0x07, 3); // 28MHz
              
	*((char *)23692) = 255; // disable scroll? prompt
	printversion();

    // read Next core version - e.g. 3.01.10 will be 0x310a
    corever = readnextreg(0x01) * 256 + readnextreg(0x0e);

    // select esp uart, set 17-bit prescalar top bits to zero
    UART_CTL = 16; 
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
            println("Can't talk to esp.\nResetting esp, try again.");
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

    println("Connecting to ");
    println(fn);

retryconnect:

    // Try disconnecting a few times just in case.
    retrycount = 10;
    while (retrycount && atcmd("AT+CIPCLOSE\r\n", "ERROR", 5, inbuf)) { retrycount--; }

    memcpy(scratch, cipstart_prefix, 19);
    memcpy(scratch+19, fn, len);
    memcpy(scratch+19+len, cipstart_postfix, 9); // take care to copy the terminating zero

    if (atcmd(scratch, "OK", 2, inbuf))
    {
        println("Unable to connect");
        goto bailout;
    }
        
    println("Handshake..");
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
                println("Retry connect..");
                goto retryconnect;
            }
            flush_uart_hard();
            println("Retry handshake..");
            goto retryhandshake;
        }
        println("Server version mismatch");
        dp[len] = 0;
        println(dp);
        goto closeconn;
    }

    println("Connected\r");
    
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
                println(fn);
                if (transfer(fn, inbuf))
                {
                    println("Lost connection.");
                    goto closeconn;
                }
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
    println("Shutting down..");
    cipxfer("Bye", 3, inbuf, &len, &dp);
    atcmd("AT+CIPCLOSE\r\n", "", 0, inbuf);
bailout:
    atcmd("AT+UART_CUR=115200,8,1,0,0\r\n", "", 0, inbuf); // restore uart speed
    println("All done");
    writenextreg(0x07, nextreg7); // restore cpu speed
    writenextreg(0x06, nextreg6); // restore turbo key & 50/60 switch
	*((char *)23692) = scr_ct; // restore old value of scr_ct
    // TODO: restore border (from basic var)
terminate:
    return;
}