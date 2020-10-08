/*
QMedian (c) 1998-2020 Jari Komppa http://iki.fi/sol/
Released under Unlicense. Google it.

This is a stb-like single-header library for quantizing 8-bit palettes.

In one source file, do this:

	#define SOL_QMEDIAN_IMPLEMENTATION
	#include "sol_qmedian.h"

Usage example (combining three palettes):

	SQ *q;
	int *idxmap;
	int idx[3];
	unsigned char *my_final_palette;
	q = sq_alloc();
	idx[0] = sq_addcolormap(q, my_palette, 256, 3);
	idx[1] = sq_addcolormap(q, my_other_palette, 256, 3);
	idx[2] = sq_addcolormap(q, my_yet_another_palette, 256, 3);
	sq_reduce(q, &idxmap, &my_final_palette, NULL, 256);
	...
	final_palette_index = *(idxmap + idx[1] + old_color_index_in_my_other_palette);
	...
	free(idxmap);
	free(my_final_palette);

Usage example (converting 32bpp RGBX image to paletted image):

	SQ *q;
	int *idxmap;
	int idxmapsize;
	unsigned char *palette;
	q = sq_alloc();
	sq_addcolormap(q, data, x * y, 4);
	sq_reduce(q, &idxmap, &palette, &idxmapsize, 256);
	... 
	idxmap is now paletted image (with idxmapsize bytes), palette is 768 byte palette
	...
	free(idxmap);
	free(palette);

	*/

#ifndef SOL_QMEDIAN_H
#define SOL_QMEDIAN_H
#ifdef __cplusplus
extern "C" {
#endif

struct sqi_colorstruc;
struct sqi_colormapstruc;

typedef struct sq_quantstruc 
{
	struct sqi_colorstruc* first;
	struct sqi_colorstruc* last;
	struct sqi_colorstruc* zeromap;
	struct sqi_colorstruc* zerolast;
	struct sqi_colormapstruc* colmap;
	struct sqi_colormapstruc* lastcolmap;
	int colors;
	int zeros;
} SQ;

/* Use sq_alloc to allocate a new quantize base. */
extern SQ* sq_alloc();

/* Add colormaps with qaddcolormap. Stride, typically 3 or 4,
 * states how many bytes per each color. If stride is 4, the
 * 4th byte is ignored.
 * This function returns the index to the idxmap where this
 * currently added colormap will start.
 * This call does not free the colmaps.
 */
extern int sq_addcolormap(SQ *q, unsigned char *colmap, int colors, int stride);

/* This will reduce the colors to palwid. It will allocate memory
 * for idxmap and pal, and will free everything that has to do
 * with SQ *q, including *q itself.
 * idxmap is index map to the palette, so you can remap the
 * colors of your pictures or whatever..
 * *(idxmap + old_color) == new_color;
 */
extern int sq_reduce(SQ* q, int** idxmap, unsigned char** pal, int* outcolors, int palwid);

#ifdef SOL_QMEDIAN_IMPLEMENTATION

#include <stdlib.h> // exit, malloc, free
#include <string.h> // memset

// re-sort after quantization
#define SQI_RE_SORT
// cut by brightness as well as r,g and b
//#define SQI_CUT_LIGHTBASE
// kill duplicates
#define SQI_DUPENUKE

#define SQI_SWAP(a, b, c) { c = a; a = b; b = c; }

union sqi_colorchunk 
{
	unsigned char component[4]; // r,g,b,light
	unsigned int block;
};

struct sqi_colorstruc 
{
	struct sqi_colorstruc *next;
	int coloridx;
	union sqi_colorchunk data;
};

struct sqi_colormapstruc 
{
	struct sqi_colormapstruc *next;
	struct sqi_colorstruc *col;
};

/*
 * local sqi_calloc
 * - Allocate memory; if failed, exit the program completely; if
 *   successful, clear the allocated memory to zero.
 */
void* sqi_calloc(int size)
{
	void *temp;
	temp = malloc(size);
	if (temp == NULL)
	{
		exit(1);
	}
	memset(temp, 0, size);
	return temp;
}

/*
 * public newquant
 * - initialize quantize structure
 */
SQ *sq_alloc(void)
{
	return (SQ *)sqi_calloc(sizeof(SQ));
}

/*
 * local add_zero
 * - add a color to the zero-list (black color)
 */
void sqi_add_zero(SQ *q, struct sqi_colorstruc *temp)
{
	temp->coloridx = q->colors;
	temp->data.block = -1;
	temp->next = NULL;
	if (q->zeromap == NULL)
	{
		q->zeromap = temp;
	}
	else 
	{
		q->zerolast->next = temp;
	}
	q->zerolast = temp;
	q->colors++;
	q->zeros++;
}

/*
 * local add_color
 * - add a color to the color list
 */
void sqi_add_color(SQ *q, struct sqi_colorstruc *temp, int r, int g, int b)
{
	temp->coloridx = q->colors;
	temp->next = NULL;
	temp->data.component[0] = r;
	temp->data.component[1] = g;
	temp->data.component[2] = b;
	temp->data.component[3] = (r * 3 + g * 6 + b) / 10;
	if (q->first == NULL) 
	{
		q->first = temp;
	}
	else 
	{
		q->last->next = temp;
	}
	q->last = temp;
	q->colors++;
}

/*
 * public sq_addcolormap
 * - add a color map to be quantized
 *   returns index to the idxmap (see qreduce)
 */
int sq_addcolormap(SQ* q, unsigned char* colmap, int colors, int stride)
{
	int a, ret;
	struct sqi_colormapstruc* temp;
	ret = q->colors;
	temp = (struct sqi_colormapstruc*)sqi_calloc(sizeof(struct sqi_colormapstruc));
	temp->col = (struct sqi_colorstruc*)sqi_calloc(sizeof(struct sqi_colorstruc) * colors);
	temp->next = NULL;
	if (q->colmap == NULL)
	{
		q->colmap = temp;
	}
	else
	{
		q->lastcolmap->next = temp;
	}
	q->lastcolmap = temp;
	for (a = 0; a < colors; a++)
	{
		if ((*(colmap + a * stride + 0) == 0) && (*(colmap + a * stride + 1) == 0) && (*(colmap + a * stride + 2) == 0))
		{
			sqi_add_zero(q, temp->col + a);
		}
		else
		{
			sqi_add_color(q, temp->col + a, *(colmap + a * stride + 0), *(colmap + a * stride + 1), *(colmap + a * stride + 2));
		}
	}
	return ret;
}

/*
 * local free_colmaps
 * - frees all allocated colormaps
 */
void sqi_free_colmaps(SQ *q)
{
	struct sqi_colormapstruc *walker, *prev;
	walker = q->colmap;
	while (walker != NULL)
	{
		prev = walker;
		free(walker->col);
		walker = walker->next;
		free(prev);
	}
}

/*
 * local examine_group
 * - Find out the largest component in sub-colorspace and return it and
 *   its size.
 */
void sqi_examine_group(struct sqi_colorstruc *g, int *groupcomponent, int *size)
{
	int rmin, rmax, gmin, gmax, bmin, bmax, rs, gs, bs, a, groupsize;
#ifdef SQI_CUT_LIGHTBASE
	int lmin, lmax, ls;
	lmin = lmax = g->data.component[3];
	ls = 0;
#endif
	rmin = rmax = g->data.component[0];
	gmin = gmax = g->data.component[1];
	bmin = bmax = g->data.component[2];
	rs = gs = bs = 0;
	g = g->next;
	while (g != NULL)
	{
		a = g->data.component[0];
		if (rmin > a) rmin = a;
		if (rmax < a) rmax = a;
		rs += a;
		a = g->data.component[1];
		if (gmin > a) gmin = a;
		if (gmax < a) gmax = a;
		gs += a;
		a = g->data.component[2];
		if (bmin > a) bmin = a;
		if (bmax < a) bmax = a;
		bs += a;
#ifdef SQI_CUT_LIGHTBASE
		a = g->data.component[3];
		if (lmin > a) lmin = a;
		if (lmax < a) lmax = a;
		ls += a;
#endif
		g = g->next;
	}
	groupsize = rmax - rmin;
	*groupcomponent = 0;
	*size = rs;
	if ((gmax - gmin) > groupsize)
	{
		groupsize = gmax - gmin;
		*groupcomponent = 1;
		*size = gs;
	}
#ifdef SQI_CUT_LIGHTBASE
	if ((lmax - lmin) > groupsize)
	{
		groupsize = lmax - lmin;
		*groupcomponent = 3;
		*size = ls;
	}
#endif
	if ((bmax - bmin) > groupsize)
	{
		*groupcomponent = 2;
		*size = bs;
	}
}

/*
 * local cut_group
 * - cut a sub-colorspace in two sub-colorspaces by splitting the
 *   linked list at median. Returns the new sub-colorspace.
 */
struct sqi_colorstruc* sqi_cut_group(struct sqi_colorstruc *g, int component, int max)
{
	int median, count;
	struct sqi_colorstruc *second = NULL, *fore = NULL;
	median = max / 2;
	count = 0;
	while (count < median) 
	{
		count += g->data.component[component];
		fore = second;
		second = g;
		g = g->next;
	}
	if (second == NULL)
	{
		return NULL;
	}
	if (fore == NULL) 
	{
		second->next = NULL;
		second = g;
	}
	else 
	{
		fore->next = NULL;
	}
	return second;
}

/*
 * local sort_group
 * - Sorts a sub-colorspace by given component with one-pass radix sort.
 */
struct sqi_colorstruc* sqi_sort_group(struct sqi_colorstruc* col, int component)
{
	struct sqi_colorstruc *bucket[256], *top[256];
	int a, s, l;
	memset(bucket, 0, sizeof(bucket)); // set bucket[n] to NULL
	// Step 1: cut list into N lists
	while (col != NULL) 
	{
		a = col->data.component[component];
		if (bucket[a] == NULL) 
		{
			bucket[a] = col;
		}
		else 
		{
			top[a]->next = col;
		}
		top[a] = col;
		col = col->next;
		top[a]->next = NULL;
	}
	// Step 2: re-link the list.
	/* s = index of the first full bucket
     * l = last used bucket
	 * a = counter
	 */
	s = 0;
	while (bucket[s] == NULL)
	{
		s++;
	}
	a = l = s;
	a++;
	while (a < 256) 
	{
		if (bucket[a] != NULL) 
		{
			top[l]->next = bucket[a];
			l = a;
		}
		a++;
	}
	return bucket[s];
}

/*
 * local dupenuke
 * - kill duplicate colors by first sorting the list by all components,
 *   then comparing adjacent colors, moving duplicates to the zero-list.
 */
void sqi_dupenuke(SQ* q) 
{
	struct sqi_colorstruc *col, *last;
	int lastidx, lastcol;
	q->first = sqi_sort_group(q->first, 0);
	q->first = sqi_sort_group(q->first, 1);
	q->first = sqi_sort_group(q->first, 2);
	last = col = q->first;
	lastidx = col->coloridx;
	lastcol = col->data.block;
	col = col->next;
	while (col != NULL) 
	{
		if (col->data.block == lastcol) 
		{ // duplicate
			col->data.block = lastidx; // set datablock to point to real color instead
			last->next = col->next;    // removed from the list
			if (q->zeromap == NULL) 
			{
				q->zeromap = col;
			}
			else 
			{
				q->zerolast->next = col;
			}
			q->zerolast = col;
			q->zerolast->next = NULL;  // added to zero-list
			col = last;
			q->zeros++;
		}
		else 
		{ // not dupe
			lastidx = col->coloridx;
			lastcol = col->data.block;
		}
		last = col;
		col = col->next;
	}
}

/*
 * public sq_reduce
 * - do the quantize, build index map, free all allocated resources.
 *   returns total number of colors in the palette.
 */
int sq_reduce(SQ *q, int **idxmap, unsigned char **pal, int *outindices, int palwid)
{
	int i, n, totalcolors, comp[3], count, groups;
	struct sqi_colorstruc* col;
	struct sqi_colorstruc** group;
	int *groupcomponent, *groupcomponentsize, *groupsorted;
#ifdef SQI_RE_SORT
	int* remapmap;
	unsigned char* temppal, * tempcp;
	SQ* reorder;
#endif
	totalcolors = q->colors;
	*pal = (unsigned char *)sqi_calloc(sizeof(char) * palwid * 3);
	*idxmap = (int *)sqi_calloc(sizeof(int) * totalcolors);
	group = (struct sqi_colorstruc **)sqi_calloc(sizeof(struct sqi_colorstruc*) * palwid);
	groupsorted = (int *)sqi_calloc(sizeof(int) * palwid);
	groupcomponent = (int *)sqi_calloc(sizeof(int) * palwid);
	groupcomponentsize = (int *)sqi_calloc(sizeof(int) * palwid);
#ifdef SQI_DUPENUKE
	sqi_dupenuke(q);
#endif
	if ((q->colors - q->zeros) < palwid) 
	{
		palwid = q->colors - q->zeros;
		if (outindices)
		{
			*outindices = totalcolors;
		}
		/*
		 * If number of input colors is less than requested output,
		 * just sort the colors, make sure color 0 is black, and
		 * so on. Don't do any real reducing, that is.
		 */
		q->first = sqi_sort_group(q->first, 3);
		i = 1;
		*(*pal + 0) = 0;
		*(*pal + 1) = 0;
		*(*pal + 2) = 0;
		col = q->first;
		while (col != NULL) 
		{
			*(*idxmap + col->coloridx) = i;
			*(*pal + i * 3 + 0) = col->data.component[0];
			*(*pal + i * 3 + 1) = col->data.component[1];
			*(*pal + i * 3 + 2) = col->data.component[2];
			col = col->next;
			i++;
		}
		col = q->zeromap;
		while (col != NULL) 
		{
			if (col->data.block == -1)
			{
				*(*idxmap + col->coloridx) = 0;
			}
			else
			{
				*(*idxmap + col->coloridx) = *(*idxmap + col->data.block);
			}
			col = col->next;
		}
		free(group);
		free(groupsorted);
		free(groupcomponent);
		free(groupcomponentsize);
		sqi_free_colmaps(q);
		free(q);
		return palwid;
	}
	// Set up, analyze initial group (i.e, all incoming colors)
	groups = 1;
	group[0] = q->first;
	sqi_examine_group(group[0], &groupcomponent[0], &groupcomponentsize[0]);
#ifdef SQI_DUPENUKE
	groupsorted[0] = 2;
#else
	groupsorted[0] = -1;
#endif
	while (groups < (palwid - 1))
	{
		// Find largest group (in a dimension, not volume)
		i = 0;
		for (n = 0; n < groups; n++)
		{
			if (groupcomponentsize[n] > groupcomponentsize[i])
			{
				i = n;
			}
		}
		// If the group was not sorted by its largest dimension, sort it
		if (groupsorted[i] != groupcomponent[i])
		{
			group[i] = sqi_sort_group(group[i], groupcomponent[i]);
			groupsorted[i] = groupcomponent[i];
		}
		// Cut the group in half at median point
		group[groups] = sqi_cut_group(group[i], groupcomponent[i], groupcomponentsize[i]);
		// If we can't cut, we're done
		if (group[groups] == NULL)
		{
			break;
		}
		// Analyze, store, increment, continue.
		sqi_examine_group(group[i], &groupcomponent[i], &groupcomponentsize[i]);
		sqi_examine_group(group[groups], &groupcomponent[groups], &groupcomponentsize[groups]);
		groupsorted[groups] = groupsorted[i];
		groups++;
	}

	*(*pal + 0) = 0;
	*(*pal + 1) = 0;
	*(*pal + 2) = 0;

	// Build palette
	for (i = 0; i < groups; i++)
	{
		col = group[i];
		*(*idxmap + col->coloridx) = i;
		count = comp[0] = comp[1] = comp[2] = 0;
		// Compute the average color for the group
		while (col != NULL)
		{
			count++;
			comp[0] += col->data.component[0];
			comp[1] += col->data.component[1];
			comp[2] += col->data.component[2];
			col = col->next;
		}
		*(*pal + (i + 1) * 3) = comp[0] / count;
		*(*pal + (i + 1) * 3) = comp[1] / count;
		*(*pal + (i + 1) * 3) = comp[2] / count;
	}
	col = q->zeromap;
	while (col != NULL)
	{
		if (col->data.block == -1)
		{
			*(*idxmap + col->coloridx) = 0;
		}
		else
		{
			*(*idxmap + col->coloridx) = *(*idxmap + col->data.block);
		}
		col = col->next;
	}
	free(group);
	free(groupcomponent);
	free(groupcomponentsize);
	if (outindices)
	{
		*outindices = totalcolors;
	}
	sqi_free_colmaps(q);
	free(q);
#ifdef SQI_RE_SORT
	reorder = sq_alloc();
	sq_addcolormap(reorder, *pal, palwid, 3);
	sq_reduce(reorder, &remapmap, &temppal, NULL, palwid);
	SQI_SWAP(*pal, temppal, tempcp);
	free(temppal);
	for (i = 0; i < totalcolors; i++)
	{
		*(*idxmap + i) = *(remapmap + *(*idxmap + i));
	}
	free(remapmap);
#endif /* RE_SORT */
	return palwid; /* total width of idxmap */
}

#endif // SOL_QMEDIAN_IMPLEMENTATION
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SOL_QMEDIAN_H
