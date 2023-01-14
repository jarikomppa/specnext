extern unsigned char fopen(unsigned char *fn, unsigned char mode);
extern void fclose(unsigned char handle);
extern unsigned short fread(unsigned char handle, unsigned char* buf, unsigned short bytes);
extern void fwrite(unsigned char handle, unsigned char* buf, unsigned short bytes);
//extern void makepath(char *pathspec); // must be 0xff terminated!
extern void conprint(char *txt) __z88dk_fastcall;
extern void println(char * t) __z88dk_fastcall;
extern unsigned char strinstr(char *a, char *b, unsigned short len, unsigned short blen);
extern void printhelp();
extern void printversion();
extern char memcmp(char *a, char *b, short l);
extern unsigned char parse_cmdline(char *f) __z88dk_fastcall;
extern unsigned char createfilewithpath(char *fn) __z88dk_fastcall;

extern void writenextreg(unsigned char reg, unsigned char val);
extern unsigned char readnextreg(unsigned char reg) __z88dk_fastcall;
extern unsigned char allocpage();
extern void freepage(unsigned char page);

extern void flush_uart();
extern void flush_uart_hard();
unsigned char receive_slow();

// xxxsmbbb
// where b = border color, m is mic, s is speaker
__sfr __at 0xfe gPort254;
__sfr __banked __at 0x133b UART_TX;
__sfr __banked __at 0x143b UART_RX;
__sfr __banked __at 0x153b UART_CTL;

extern unsigned short receive(char *b);
extern char checksum(char *dp, unsigned short len);

extern void memcpy(char *dest, const char *source, unsigned short count);
extern unsigned short mulby10(unsigned short input) __z88dk_fastcall;

extern unsigned short framecounter;
extern char *cmdline;
extern char dbg;
extern unsigned short corever;
extern char scr_ct;
