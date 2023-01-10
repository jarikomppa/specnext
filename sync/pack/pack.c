#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int parc, char** pars)
{
	if (parc < 3)
	{
		printf("Usage: %s infile outfile\n", pars[0]);
		return 0;
	}
	FILE* f = fopen(pars[1], "rb");
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* src = malloc(len);
	fread(src, 1, len, f);
	fclose(f);
	char* dst = malloc(len * 2);

    int i = 0, o = 0;
    while (i < len)
    {
        int skip = 0;
        while (skip + i + 2 < len && !(src[i + skip] == src[i + skip + 1] && src[i + skip] == src[i + skip + 2]))
            skip++;
        if (len - (skip + i) <= 2)
            skip = len - i;
        while (skip > 8192)
        {
            dst[o++] = 0xff;
            dst[o++] = (8192) >> 8;
            dst[o++] = (8192) & 0xff;
            for (int x = 0; x < 8192; x++)
                dst[o++] = src[i++];
            dst[o++] = 1;
            dst[o++] = src[i++];
            skip -= 8192 + 1;
        }
        if (skip < 0xff)
        {
            dst[o++] = skip;
        }
        else
        {
            dst[o++] = 0xff;
            dst[o++] = skip >> 8;
            dst[o++] = skip & 0xff;
        }
        for (int x = 0; x < skip; x++)
            dst[o++] = src[i++];
        if (i < len)
        {
            int run = 1;
            while (run + i < len && src[run + i] == src[i])
                run++;
            if (run > 8192)
                run = 8192;
            if (run < 0xff)
            {
                dst[o++] = run;
            }
            else
            {
                dst[o++] = 0xff;
                dst[o++] = run >> 8;
                dst[o++] = run & 0xff;
            }
            dst[o++] = src[i];
            i += run;
        }
    }

    f = fopen(pars[2], "wb");
    fwrite(dst, 1, o, f);
    fclose(f);

	return 0;
}