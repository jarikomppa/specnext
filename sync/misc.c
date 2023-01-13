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
