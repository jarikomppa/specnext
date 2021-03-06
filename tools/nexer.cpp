/*
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/decode_ihx.c"


void writehdr(int aRam, FILE * aF)
{
    struct {
        unsigned char Next[4];			//"Next"
        unsigned char VersionNumber[4];	//"V1.1" = Gold distro. V1.2 allows entering with PC in a 16K bank >= 8.
        unsigned char RAM_Required;		//0=768K, 1=1792K
        unsigned char NumBanksToLoad;	//0-112 x 16K banks
        unsigned char LoadingScreen;	//1 = layer2 at 16K page 9, 2=ULA loading, 4=LORES, 8=HiRes, 16=HIColour, +128 = don't load palette.
        unsigned char BorderColour;		//0-7 ld a,BorderColour:out(254),a
        unsigned short SP;				//Stack Pointer
        unsigned short PC;				//Code Entry Point : $0000 = Don't run just load.
        unsigned short NumExtraFiles;	//NumExtraFiles
        unsigned char Banks[64 + 48];	//Which 16K Banks load.	: Bank 5 = $0000-$3fff, Bank 2 = $4000-$7fff, Bank 0 = $c000-$ffff
        unsigned char loadingBar;		//Loading bar off=0/on=1
        unsigned char loadingColour;	//Loading bar Layer2 index colour
        unsigned char loadingBankDelay;	//Delay after each bank
        unsigned char loadedDelay;		//Delay (frames) after loading before running
        unsigned char dontResetRegs;	//Don't reset the registers
        unsigned char CoreRequired[3];	//CoreRequired byte per value, decimal, not string. ordering... Major, Minor, Subminor
        unsigned char HiResColours;		//to be anded with three, and shifted left three times, and add the mode number for hires and out (255),a
        unsigned char EntryBank;		//V1.2: 0-112, this 16K bank will be paged in at $C000 before jumping to PC. The default is 0, which is the default upper 16K bank anyway.
        unsigned short FileHandleAddress;
        unsigned char ExpansionBusEnable;
        unsigned char HasChecksum;
        unsigned int  FirstBankOffset;
        unsigned short CLIBufferAddress;
        unsigned short CLIBufferSize;
        unsigned char LoadingScreen2;
        unsigned char HasCopperBlock;
        unsigned char Tilemode[4];
        unsigned char LoadingBarYPos; 
        unsigned char RestOf512Bytes[512 - (4 + 4 + 1 + 1 + 1 + 1 + 2 + 2 + 2 + 64 + 48 + 1 + 1 + 1 + 1 + 1 + 3 + 1 + 1 + 2 + 1 + 1 + 4 + 2 + 2 + 1 + 1 + 1 + 4 + 4)];
        unsigned int  CRC;
    } h;
    memset(&h, 0, sizeof(h));
    
    h.Next[0] = 'N';
    h.Next[1] = 'e';
    h.Next[2] = 'x';
    h.Next[3] = 't';
    h.VersionNumber[0] = 'V';
    h.VersionNumber[1] = '1';
    h.VersionNumber[2] = '.';
    h.VersionNumber[3] = '1';
    h.RAM_Required = aRam;
    h.NumBanksToLoad = 1;
    h.SP = 0xffff;
    h.PC = 0xc000;
    h.Banks[0] = 1;
    //memcpy(&h.Banks[16], "Made with nexer  http://iki.fi/sol", 17 + 17);
    
    fwrite(&h, 1, sizeof(h), aF);
}


unsigned char * readfile(char * aFn, int &aLen)
{    
    FILE * f = fopen(aFn, "rb");
    if (!f)
    {
        printf("File not found:\"%s\"\n", aFn);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char * src = new unsigned char[len + 1];
    fread(src, 1, len, f);
    fclose(f);
    src[len] = 0;
    aLen = len;
    return src;
}

void writecode(char * aIhx, FILE *aF)
{
    unsigned char *data = new unsigned char[0x10000];
    int len;
    unsigned char *src = readfile(aIhx, len);
    if (src == 0)
        exit(0);
    int start, end;
    len = decode_ihx(src, len, data, start, end);
    if (start != 0xc000)
    {
        printf("Start address is not 0xc000");
        exit(0);
    }
    fwrite(data + start, 1, len, aF);
    delete[] data;
}

int main(int parc, char ** pars)
{
    if (parc < 3)
    {
        printf("Usage: input.ihx output.nex\n");
        return 0;
    }
    printf("input : %s\noutput: %s\n", pars[1], pars[2]);
    FILE * f;
    f = fopen(pars[2], "wb");
    writehdr(0, f);
    writecode(pars[1], f);
    fclose(f);
    return 0;
}