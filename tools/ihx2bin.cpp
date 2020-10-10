/*
 * Part of Jari Komppa's zx spectrum next suite 
 * https://github.com/jarikomppa/specnext
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain) 
 */

#include <stdio.h>
#include <stdlib.h>
#include "../common/decode_ihx.h"


int main(int parc, char ** pars)
{
    if (parc < 3)
    {
        printf("Usage: input.ihx output.raw\n");
        return 0;
    }
    
    unsigned char *data = new unsigned char[0x10000];
    FILE * f = fopen(pars[1], "rb");
    if (!f)
    {
        printf("File not found");
        return 0;
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    unsigned char * src = new unsigned char[len+1];
    fread(src, 1, len, f);
    fclose(f);
    src[len] = 0;
    int start, end;
    len = decode_ihx(src, len, data, start, end);
    f = fopen(pars[2], "wb");
    fwrite(data + start, 1, len, f);
    fclose(f);
    printf("%d bytes written\n", len);
    return 0;
}