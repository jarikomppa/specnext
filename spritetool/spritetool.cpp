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

	printf("Found %d unique colors\n", colorcount);
	SQ* q;
	unsigned char* idxmap;
	unsigned char* palette;
	q = sq_alloc();
	sq_addcolormap(q, data, x * y, 4);
	int count =	sq_reduce(q, &idxmap, &palette, NULL, 256);
	free(idxmap);
	for (int i = 0; i < count * 3; i++) { printf(" %02x", palette[i]); if (i % 3 == 2) printf("\n"); }
	printf("\n");
	free(palette);
	printf("%d total colors\n", count);

	return 0;
}