extern void conprint(char *txt) __z88dk_fastcall;
extern char *cmdline;
extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void makepath(char *pathspec); // must be 0xff terminated!


void println(char * t)
{
	*((char *)23692) = 255; // disable scroll? prompt
	conprint(t);
	conprint("\r");
}

void printversion()
{
    conprint(
	   //12345678901234567890123456789012
		"NextSync 1.2 by Jari Komppa\r"
		"http://iki.fi/sol\r");
}

void printhelp()
{
	printversion();
	conprint(
	   //12345678901234567890123456789012
		"Wifi transfer files from PC\r"
		"\r"
		"SYNOPSIS:\r"
		".SYNC [servername/ip addr]\r"
		"\r"
		"Either run without parameters\r"
		"to sync files, or give server\r"
		"address or name to save config.\r"
		"\r"
		"Please read nextsync.txt for\r"
		"further instructions.\r\r");
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
    println(temp);
}

unsigned char strinstr(char *a, char *b, unsigned short len, char blen)
{
    if (!*b || !blen) return 1;
    while (len)
    {
        if (*a == *b)
        {            
            unsigned char i = 0;
            while (i < blen && a[i] == b[i]) i++;
            if (i >= blen)
                return 1;
        }
        a++;
        len--;
    }
    return 0;
}

char memcmp(char *a, char *b, unsigned char l)
{
    unsigned char i = 0;
    while (i < l)
    {
        char v = a[i] - b[i];
        if (v != 0) return v;            
        i++;
    }
    return 0;
}

unsigned char parse_cmdline(char *f)
{
    unsigned char i;   
    
    if (!cmdline)
    {
        f[0] = 0;
        return 0;
    }

    i = 0;
    while (i < 127 && cmdline[i] != 0 && cmdline[i] != 0xd && cmdline[i] != ':')
    {
        f[i] = cmdline[i];
        i++;        
    }

    f[i] = 0;
    return i;
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
