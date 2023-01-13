extern void conprint(char *txt) __z88dk_fastcall;
extern void println(char *txt) __z88dk_fastcall;

void printversion()
{
    conprint(
	   //12345678901234567890123456789012
		"NextSync 1.2+dev by Jari Komppa\r"
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
