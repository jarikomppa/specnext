#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define SOL_QMEDIAN_IMPLEMENTATION
#include "sol_qmedian.h"

int main(int parc, char** pars)
{
	if (parc < 3)
	{
		printf("Usage: inputfile outputfile\n");
		return -1;
	}
	int x,y,n;
    unsigned char *data = stbi_load(pars[1], &x, &y, &n, 4);
	if (!data)
	{
		printf("Unable to read \"%s\"\n", pars[1]);
		return -1;
	}
	if ((x & 15) || (y & 15))
	{
		printf("Image resolution (%dx%d) not divisible by 16\n", x, y);
		return -1;
	}
	printf("%d rows, %d columns - total %d sprites\n", y >> 4, x >> 4, (y >> 4) * (x >> 4));

	// Reduce color space to 3 bits (8 levels) each
	for (int i = 0; i < x * y * 4; i++)
		data[i] >>= 5;

	int colors[512];
	for (int i = 0; i < 512; i++)
		colors[i] = 0;
	for (int i = 0; i < x * y; i++)
		colors[(data[i * 4 + 0]) | (data[i * 4 + 1] << 3) | (data[i * 4 + 2] << 6)] = 1;
	int colorcount = 0;
	for (int i = 0; i < 512; i++)
		if (colors[i])
			colorcount++;

	printf("Found %d unique colors, ", colorcount);
	SQ* q;
	unsigned char* idxmap;
	unsigned char* palette;
	int transparent_index = 0xff;
	q = sq_alloc();
	sq_addcolormap(q, data, x * y, 4);
	int count =	sq_reduce(q, &idxmap, &palette, NULL, 256);
	printf("%d total colors after quantization\n", count);
	printf("Top left color (%d, %d, %d) index %d\n", data[0], data[1], data[2], idxmap[0]);
	int keyidx = idxmap[0];
	if (keyidx != transparent_index)
	{
		printf("Remapping %d to %d\n", idxmap[0], transparent_index);
		for (int i = 0; i < x * y; i++)
		{
			if (idxmap[i] == transparent_index)
			{
				idxmap[i] = keyidx;
			}
			else			
			if (idxmap[i] == keyidx)
			{
				idxmap[i] = transparent_index;
			}
		}
		int r = palette[keyidx * 3 + 0];
		int g = palette[keyidx * 3 + 1];
		int b = palette[keyidx * 3 + 2];
		palette[keyidx * 3 + 0] = palette[transparent_index * 3 + 0];
		palette[keyidx * 3 + 1] = palette[transparent_index * 3 + 1];
		palette[keyidx * 3 + 2] = palette[transparent_index * 3 + 2];
		palette[transparent_index * 3 + 0] = r;
		palette[transparent_index * 3 + 1] = g;
		palette[transparent_index * 3 + 2] = b;
	}

	FILE* f = fopen(pars[2], "wb");
	if (!f)
	{
		printf("Unable to open %s\n", pars[2]);
		return -1;
	}
	printf("Outputting palette..\n");
	for (int i = 0; i < 256; i++)
	{
		int c = (palette[i * 3 + 0] << 0) | (palette[i * 3 + 1] << 3) | (palette[i * 3 + 2] << 6);
		fprintf(f, "0x%02x, 0x%02x, // palette index %3d, (%d, %d, %d)\n", c >> 1, c & 1, i, palette[i * 3 + 0], palette[i * 3 + 1], palette[i * 3 + 2]);
	}
	printf("Outputting sprites..\n");
	for (int row = 0, sprite = 0; row < (y >> 4); row++)
	{
		for (int col = 0; col < (x >> 4); col++, sprite++)
		{
			fprintf(f, "// Sprite %d\n", sprite);
			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 16; j++)
				{
					fprintf(f, "0x%02x, ", idxmap[(row * 16 + i) * x + (col * 16 + j)]);
				}
				fprintf(f, "\n");
			}
		}
	}

	fclose(f);

	free(idxmap);
	free(palette);

	printf("All done\n");

	return 0;
}
