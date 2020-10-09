/*
QMedian (c) 1998-2020 Jari Komppa http://iki.fi/sol/
Released under Unlicense. Google it.

This is a stb-like single-header library for quantizing 8-bit palettes.

In one source file, do this:

	#define SOL_QMEDIAN_IMPLEMENTATION
	#include "sol_qmedian.h"

Usage example (combining three palettes):

	SQ *q;
	unsigned char *idxmap;
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
	unsigned char *idxmap;
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

// Data type for the output index map. By default unsigned char,
// but in case you want indexes over 255, you may want to use int.
#define SQ_IDXMAPTYPE unsigned char

struct sqi_colorstruc;
struct sqi_colormapstruc;

typedef struct sq_quantstruc 
{
	struct sqi_colorstruc* mFirst;
	struct sqi_colorstruc* mLast;
	struct sqi_colorstruc* mZeromap;
	struct sqi_colorstruc* mZerolast;
	struct sqi_colormapstruc* mColmap;
	struct sqi_colormapstruc* mLastcolmap;
	int mColors;
	int mZeros;
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
extern int sq_addcolormap(SQ *q, unsigned char *aColmap, int aColors, int aStride);

/* This will reduce the colors to palwid. It will allocate memory
 * for idxmap and pal, and will free everything that has to do
 * with SQ *q, including *q itself.
 * idxmap is index map to the palette, so you can remap the
 * colors of your pictures or whatever..
 * *(idxmap + old_color) == new_color;
 */
extern int sq_reduce(SQ* q, SQ_IDXMAPTYPE** aIdxmap, unsigned char** aPal, int* aOutcolorCount, int aPalwid);

#ifdef SOL_QMEDIAN_IMPLEMENTATION

#include <stdlib.h> // exit, malloc, free
#include <string.h> // memset

// re-sort after quantization
#define SQI_RE_SORT
// kill duplicates
#define SQI_DUPENUKE

#define SQI_SWAP(a, b, c) { c = a; a = b; b = c; }

union sqi_colorchunk 
{
	unsigned char mComponent[4]; // r,g,b,light
	unsigned int mBlock;
};

struct sqi_colorstruc 
{
	struct sqi_colorstruc *mNext;
	int mColoridx;
	union sqi_colorchunk mData;
};

struct sqi_colormapstruc 
{
	struct sqi_colormapstruc *mNext;
	struct sqi_colorstruc *mCol;
};

/*
 * local sqi_calloc
 * - Allocate memory; if failed, exit the program completely; if
 *   successful, clear the allocated memory to zero.
 */
void* sqi_calloc(int aSize)
{
	void *temp;
	temp = malloc(aSize);
	if (temp == NULL)
	{
		exit(1);
	}
	memset(temp, 0, aSize);
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
 * local add_color
 * - add a color to the color list
 */
void sqi_add_color(SQ *q, struct sqi_colorstruc *aNode, int aR, int aG, int aB)
{
	aNode->mColoridx = q->mColors;
	aNode->mNext = NULL;
	aNode->mData.mComponent[0] = aR;
	aNode->mData.mComponent[1] = aG;
	aNode->mData.mComponent[2] = aB;
	aNode->mData.mComponent[3] = 0;
	if (q->mFirst == NULL) 
	{
		q->mFirst = aNode;
	}
	else 
	{
		q->mLast->mNext = aNode;
	}
	q->mLast = aNode;
	q->mColors++;
}

/*
 * public sq_addcolormap
 * - add a color map to be quantized
 *   returns index to the idxmap (see qreduce)
 */
int sq_addcolormap(SQ* q, unsigned char* aColmap, int aColors, int aStride)
{
	int a, ret;
	struct sqi_colormapstruc* temp;
	ret = q->mColors;
	temp = (struct sqi_colormapstruc*)sqi_calloc(sizeof(struct sqi_colormapstruc));
	temp->mCol = (struct sqi_colorstruc*)sqi_calloc(sizeof(struct sqi_colorstruc) * aColors);
	temp->mNext = NULL;
	if (q->mColmap == NULL)
	{
		q->mColmap = temp;
	}
	else
	{
		q->mLastcolmap->mNext = temp;
	}
	q->mLastcolmap = temp;
	for (a = 0; a < aColors; a++)
	{
		sqi_add_color(q, temp->mCol + a, *(aColmap + a * aStride + 0), *(aColmap + a * aStride + 1), *(aColmap + a * aStride + 2));
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
	walker = q->mColmap;
	while (walker != NULL)
	{
		prev = walker;
		free(walker->mCol);
		walker = walker->mNext;
		free(prev);
	}
}

/*
 * local examine_group
 * - Find out the largest component in sub-colorspace and return it and
 *   its size.
 */
void sqi_examine_group(struct sqi_colorstruc *aGroup, int *aComponent, int *aSize, int *aSum)
{
	int rmin, rmax, gmin, gmax, bmin, bmax, rs, gs, bs, v;
	rs = rmin = rmax = aGroup->mData.mComponent[0];
	gs = gmin = gmax = aGroup->mData.mComponent[1];
	bs = bmin = bmax = aGroup->mData.mComponent[2];
	aGroup = aGroup->mNext;
	while (aGroup != NULL)
	{
		v = aGroup->mData.mComponent[0];
		rs += v;
		if (rmin > v) rmin = v;
		if (rmax < v) rmax = v;
		v = aGroup->mData.mComponent[1];
		gs += v;
		if (gmin > v) gmin = v;
		if (gmax < v) gmax = v;
		v = aGroup->mData.mComponent[2];
		bs += v;
		if (bmin > v) bmin = v;
		if (bmax < v) bmax = v;
		aGroup = aGroup->mNext;
	}
	*aSize = rmax - rmin;
	*aComponent = 0;
	*aSum = rs;
	if ((gmax - gmin) > *aSize)
	{
		*aSize = gmax - gmin;
		*aComponent = 1;
		*aSum = gs;
	}
	if ((bmax - bmin) > *aSize)
	{
		*aSize = bmax - bmin;
		*aComponent = 2;
		*aSum = bs;
	}
}

/*
 * local cut_group
 * - cut a sub-colorspace in two sub-colorspaces by splitting the
 *   linked list at median. Returns the new sub-colorspace.
 */
struct sqi_colorstruc* sqi_cut_group(struct sqi_colorstruc *aGroup, int aComponent, int aMax)
{
	int median, count;
	struct sqi_colorstruc *second = NULL, *fore = NULL;
	median = aMax / 2;
	count = 0;
	while (count <= median) 
	{
		count += aGroup->mData.mComponent[aComponent];
		fore = second;
		second = aGroup;
		aGroup = aGroup->mNext;
	}
	if (second == NULL)
	{
		return NULL;
	}
	if (fore == NULL) 
	{
		second->mNext = NULL;
		second = aGroup;
	}
	else 
	{
		fore->mNext = NULL;
	}
	return second;
}

/*
 * local sort_group
 * - Sorts a sub-colorspace by given component with one-pass radix sort.
 */
struct sqi_colorstruc* sqi_sort_group(struct sqi_colorstruc* aCol, int aComponent)
{
	struct sqi_colorstruc *bucket[256], *top[256];
	int a, s, l;
	memset(bucket, 0, sizeof(bucket)); // set bucket[n] to NULL
	// Step 1: cut list into N lists
	while (aCol != NULL) 
	{
		a = aCol->mData.mComponent[aComponent];
		if (bucket[a] == NULL) 
		{
			bucket[a] = aCol;
		}
		else 
		{
			top[a]->mNext = aCol;
		}
		top[a] = aCol;
		aCol = aCol->mNext;
		top[a]->mNext = NULL;
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
			top[l]->mNext = bucket[a];
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
	// Sort by all dimensions, leaving identical colors next to each other
	q->mFirst = sqi_sort_group(q->mFirst, 0);
	q->mFirst = sqi_sort_group(q->mFirst, 1);
	q->mFirst = sqi_sort_group(q->mFirst, 2);
	last = col = q->mFirst;
	lastidx = col->mColoridx;
	lastcol = col->mData.mBlock;
	col = col->mNext;
	while (col != NULL) 
	{
		if (col->mData.mBlock == lastcol) 
		{ 
			// duplicate
			col->mData.mBlock = lastidx; // set datablock to point to real color instead
			last->mNext = col->mNext;    // removed from the list
			if (q->mZeromap == NULL) 
			{
				q->mZeromap = col;
			}
			else 
			{
				q->mZerolast->mNext = col;
			}
			q->mZerolast = col;
			q->mZerolast->mNext = NULL;  // added to zero-list
			col = last;
			q->mZeros++;
		}
		else 
		{ 
			// not dupe
			lastidx = col->mColoridx;
			lastcol = col->mData.mBlock;
		}
		last = col;
		col = col->mNext;
	}
}

/*
 * public sq_reduce
 * - do the quantize, build index map, free all allocated resources.
 *   returns total number of colors in the palette.
 */
int sq_reduce(SQ *q, SQ_IDXMAPTYPE**aIdxmap, unsigned char **aPal, int *aOutindices, int aPalwid)
{
	int i, n, totalcolors, comp[3], count, groups;
	struct sqi_colorstruc* col;
	struct sqi_colorstruc** group;
	int *groupcomponent, *groupcomponentsize, *groupcomponentsum, *groupsorted;
#ifdef SQI_RE_SORT
	SQ_IDXMAPTYPE* remapmap;
	unsigned char* temppal, * tempcp;
	SQ* reorder;
#endif
	totalcolors = q->mColors;
	*aPal = (unsigned char *)sqi_calloc(sizeof(unsigned char) * aPalwid * 3);
	*aIdxmap = (SQ_IDXMAPTYPE *)sqi_calloc(sizeof(SQ_IDXMAPTYPE) * totalcolors);
	group = (struct sqi_colorstruc **)sqi_calloc(sizeof(struct sqi_colorstruc*) * aPalwid);
	groupsorted = (int *)sqi_calloc(sizeof(int) * aPalwid);
	groupcomponent = (int *)sqi_calloc(sizeof(int) * aPalwid);
	groupcomponentsize = (int *)sqi_calloc(sizeof(int) * aPalwid);
	groupcomponentsum = (int *)sqi_calloc(sizeof(int) * aPalwid);
#ifdef SQI_DUPENUKE
	sqi_dupenuke(q);
#endif
	if ((q->mColors - q->mZeros) <= aPalwid) 
	{
		aPalwid = q->mColors - q->mZeros;
		if (aOutindices)
		{
			*aOutindices = totalcolors;
		}
		/*
		 * If number of input colors is less than requested output,
		 * just sort the colors and so on. Don't do any real reducing, that is.
		 */
		q->mFirst = sqi_sort_group(q->mFirst, 0);
		q->mFirst = sqi_sort_group(q->mFirst, 2);
		q->mFirst = sqi_sort_group(q->mFirst, 1);
		col = q->mFirst;
		i = 0;
		while (col != NULL) 
		{
			*(*aIdxmap + col->mColoridx) = i;
			*(*aPal + i * 3 + 0) = col->mData.mComponent[0];
			*(*aPal + i * 3 + 1) = col->mData.mComponent[1];
			*(*aPal + i * 3 + 2) = col->mData.mComponent[2];
			col = col->mNext;
			i++;
		}
		col = q->mZeromap;
		while (col != NULL) 
		{
			*(*aIdxmap + col->mColoridx) = *(*aIdxmap + col->mData.mBlock);
			col = col->mNext;
		}
		free(group);
		free(groupsorted);
		free(groupcomponent);
		free(groupcomponentsize);
		free(groupcomponentsum);
		sqi_free_colmaps(q);
		free(q);
		return aPalwid;
	}
	// Set up, analyze initial group (i.e, all incoming colors)
	groups = 1;
	group[0] = q->mFirst;
	sqi_examine_group(group[0], &groupcomponent[0], &groupcomponentsize[0], &groupcomponentsum[0]);
	groupsorted[0] = -1;
	while (groups < aPalwid)
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
		group[groups] = sqi_cut_group(group[i], groupcomponent[i], groupcomponentsum[i]);
		// If we can't cut, we're done
		if (group[groups] == NULL)
		{
			break;
		}
		// Analyze, store, increment, continue.
		sqi_examine_group(group[i], &groupcomponent[i], &groupcomponentsize[i], &groupcomponentsum[i]);
		sqi_examine_group(group[groups], &groupcomponent[groups], &groupcomponentsize[groups], &groupcomponentsum[groups]);
		groupsorted[groups] = groupsorted[i];
		groups++;
	}

	// Build palette
	for (i = 0; i < groups; i++)
	{
		col = group[i];
		*(*aIdxmap + col->mColoridx) = i;
		count = comp[0] = comp[1] = comp[2] = 0;
		// Compute the average color for the group
		while (col != NULL)
		{
			count++;
			comp[0] += col->mData.mComponent[0];
			comp[1] += col->mData.mComponent[1];
			comp[2] += col->mData.mComponent[2];
			col = col->mNext;
		}
		if (groupcomponentsize[i] > 1)
		{ 
			// Average the colors in the group
			*(*aPal + i * 3 + 0) = (comp[0] + count / 2) / count;
			*(*aPal + i * 3 + 1) = (comp[1] + count / 2) / count;
			*(*aPal + i * 3 + 2) = (comp[2] + count / 2) / count;
		}
		else
		{ 
			// in case the group is small, averaging may lead into
			// duplicate colors in tiny color spaces; using one
			// of the original colors is "good enough"
			*(*aPal + i * 3 + 0) = group[i]->mData.mComponent[0];
			*(*aPal + i * 3 + 1) = group[i]->mData.mComponent[1];
			*(*aPal + i * 3 + 2) = group[i]->mData.mComponent[2];
		}
	}
	col = q->mZeromap;
	while (col != NULL)
	{
		*(*aIdxmap + col->mColoridx) = *(*aIdxmap + col->mData.mBlock);
		col = col->mNext;
	}
	free(group);
	free(groupsorted);
	free(groupcomponent);
	free(groupcomponentsize);
	free(groupcomponentsum);
	if (aOutindices)
	{
		*aOutindices = totalcolors;
	}
	sqi_free_colmaps(q);
	free(q);
#ifdef SQI_RE_SORT
	reorder = sq_alloc();
	sq_addcolormap(reorder, *aPal, aPalwid, 3);
	aPalwid = sq_reduce(reorder, &remapmap, &temppal, NULL, aPalwid);
	SQI_SWAP(*aPal, temppal, tempcp);
	free(temppal);
	for (i = 0; i < totalcolors; i++)
	{
		*(*aIdxmap + i) = *(remapmap + *(*aIdxmap + i));
	}
	free(remapmap);
#endif /* RE_SORT */
	return aPalwid; // Number of colors in palette
}

#endif // SOL_QMEDIAN_IMPLEMENTATION
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SOL_QMEDIAN_H
