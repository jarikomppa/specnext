#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
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
}HEADER;
HEADER h;


int main(int parc, char ** pars)
{
    if (parc < 2)
    {
        printf("Usage: %s nex file\n", pars[0]);
        return 0;
    }
    FILE * f = fopen(pars[1], "rb");
    if (!f)
    {
        printf("Unable to open %s\n", pars[1]);
        return 0;
    }
    fread(&h, 1, 512, f);
    fclose(f);
    if (memcmp(h.Next, "Next", 4) != 0) 
    {
        printf("Not a specnext .nex header\n");
        return 0;
    }
    
    char * border[8] = {"Black", "Blue", "Red", "Magneta", "Green", "Cyan", "Yellow", "White"};
    
    printf("VersionNumber     : %c%c%c%c\n", h.VersionNumber[0], h.VersionNumber[1], h.VersionNumber[2], h.VersionNumber[3]);
    printf("RAM_Required      : %d -> %dk\n", h.RAM_Required, h.RAM_Required ? 1792 : 768);
    printf("NumBanksToLoad    : %d -> %dk\n", h.NumBanksToLoad, h.NumBanksToLoad * 16);
    printf("LoadingScreen     : %d -> ", h.LoadingScreen);
    if (h.LoadingScreen & 128) printf("NoPalBlock ");
    if (h.LoadingScreen & 64) printf("SeeFlags2 ");
    if (h.LoadingScreen & 16) printf("Hi-Colour ");
    if (h.LoadingScreen & 8) printf("Hi-Res ");
    if (h.LoadingScreen & 4) printf("Lo-Res ");
    if (h.LoadingScreen & 2) printf("ULA ");
    if (h.LoadingScreen & 1) printf("Layer2 ");
    printf("\n");
    printf("BorderColour      : %d -> %s\n", h.BorderColour, border[h.BorderColour]);
    printf("SP                : %04xh\n", h.SP);
    printf("PC                : %04xh\n", h.PC);
    printf("NumExtraFiles     : %d\n", h.NumExtraFiles);
    printf("Banks             :");
    for (int i = 0; i < 16; i++)
    {
        printf("%2x", i);
    }
    for (int i = 0; i < 64 + 48; i++)
    {
        if ((i & 15) == 0)
            printf("\n                   ");
        printf("%2d", h.Banks[i]);
    }
    printf("\n");
    printf("loadingBar        : %d\n", h.loadingBar);
    printf("loadingColour     : %d\n", h.loadingColour);
    printf("loadingBankDelay  : %d\n", h.loadingBankDelay);
    printf("loadedDelay       : %d\n", h.loadedDelay);
    printf("dontResetRegs     : %d\n", h.dontResetRegs);
    printf("CoreRequired      : %d.%d.%d\n", h.CoreRequired[0], h.CoreRequired[1], h.CoreRequired[2]);
    printf("HiResColours      : %d\n", h.HiResColours);
    printf("EntryBank         : %d\n", h.EntryBank);
    printf("FileHandleAddress : %04xh\n", h.FileHandleAddress);
    printf("ExpansionBusEnable: %d\n", h.ExpansionBusEnable);
    printf("HasChecksum       : %d -> %08x\n", h.HasChecksum, h.CRC);
    printf("FirstBankOffset   : %d bytes\n", h.FirstBankOffset);
    printf("CLIBufferAddress  : %04xh\n", h.CLIBufferAddress);
    printf("CLIBufferSize     : %xh (%d)\n", h.CLIBufferSize, h.CLIBufferSize);
    printf("LoadingScreen2    : %d -> ", h.LoadingScreen2);
    switch (h.LoadingScreen2) {
        case 0: printf("N/A\n"); break;
        case 1: printf("Layer 2 320x256x8bpp\n"); break;
        case 2: printf("Layer 2 640x256x4bpp\n"); break;
        case 3: printf("Tilemode screen\n"); break;
        default: printf("Unknown\n"); break;
    }
    printf("HasCopperBlock    : %d\n", h.HasCopperBlock);
    printf("Tilemode          : %02x %02x %02x %02x\n", h.Tilemode[0], h.Tilemode[1], h.Tilemode[2], h.Tilemode[3]);
    printf("LoadingBarYPos    : %d\n", h.LoadingBarYPos);

    
    return 0;
}