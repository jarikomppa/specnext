extern void conprint(char *txt) __z88dk_fastcall;
extern void println(char *txt) __z88dk_fastcall;
extern char *cmdline;
extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void makepath(char *pathspec); // must be 0xff terminated!

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
