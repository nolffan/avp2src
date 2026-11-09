/* darray.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * GameSpy's dynamic array: see darray.h for the functions' descriptions.
 */
#include <stdlib.h>
#include <string.h>
#include "darray.h"

#define DEF_GROWBY 8

struct DArrayImplementation
{
	int count, capacity;
	int elemsize;
	int growby;
	ArrayElementFreeFn elemfreefn;
	void *list; /* array of elements */
};

static void *mylsearch(const void *key, void *base, int count, int size,
					   ArrayCompareFn comparator);
static void *mybsearch(const void *elem, void *base, int num, int elemsize,
					   ArrayCompareFn comparator);

DArray ArrayNew(int elemSize, int numElemsToAllocate,
				ArrayElementFreeFn elemFreeFn)
{
	DArray array;

	array = (DArray)malloc(sizeof(struct DArrayImplementation));
	if (numElemsToAllocate == 0)
		numElemsToAllocate = DEF_GROWBY;
	array->count = 0;
	array->capacity = numElemsToAllocate;
	array->elemsize = elemSize;
	array->growby = numElemsToAllocate;
	array->elemfreefn = elemFreeFn;
	array->list = malloc(array->capacity * array->elemsize);

	return array;
}

static void FreeElement(DArray array, int n);

void ArrayFree(DArray array)
{
	int i;

	for (i = 0; i < array->count; i++)
		FreeElement(array, i);
	free(array->list);
	free(array);
}

/* FreeElement
 * Frees the element at position N in the array
 */
static void FreeElement(DArray array, int n)
{
	if (array->elemfreefn != NULL)
		array->elemfreefn(ArrayNth(array, n));
}

int ArrayLength(const DArray array)
{
	return array->count;
}

void *ArrayNth(DArray array, int n)
{
	return (char *)array->list + array->elemsize * n;
}

/* ArrayAppend
 * Just do an Insert at the end of the array
 */
void ArrayAppend(DArray array, const void *newElem)
{
	ArrayInsertAt(array, newElem, array->count);
}

static void ArrayGrow(DArray array);
static void SetElement(DArray array, const void *elem, int pos);

void ArrayInsertAt(DArray array, const void *newElem, int n)
{
	if (array->count == array->capacity)
		ArrayGrow(array);
	array->count++;
	if (n < array->count - 1) /* if we aren't appending */
		memmove(ArrayNth(array, n + 1), ArrayNth(array, n),
				(array->count - 1 - n) * array->elemsize);
	SetElement(array, newElem, n);
}

/* ArrayGrow
 * Reallocates the array to a new size, increased by growby
 */
static void ArrayGrow(DArray array)
{
	array->capacity += array->growby;
	array->list = realloc(array->list, array->capacity * array->elemsize);
}

/* SetElement
 * Sets the element at pos to the contents of elem
 */
static void SetElement(DArray array, const void *elem, int pos)
{
	memcpy(ArrayNth(array, pos), elem, array->elemsize);
}

void ArrayRemoveAt(DArray array, int n)
{
	if (n < array->count - 1) /* if not last element */
		memmove(ArrayNth(array, n), ArrayNth(array, n + 1),
				(array->count - 1 - n) * array->elemsize);
	array->count--;
}

void ArrayDeleteAt(DArray array, int n)
{
	FreeElement(array, n);
	ArrayRemoveAt(array, n);
}

void ArrayReplaceAt(DArray array, const void *newElem, int n)
{
	FreeElement(array, n);
	SetElement(array, newElem, n);
}

void ArraySort(DArray array, ArrayCompareFn comparator)
{
	qsort(array->list, array->count, array->elemsize, comparator);
}

int ArraySearch(DArray array, const void *key, ArrayCompareFn comparator,
				int fromIndex, int isSorted)
{
	void *res;

	if (array->count == 0)
		return NOT_FOUND;
	if (isSorted)
		res = mybsearch(key, ArrayNth(array, fromIndex),
						array->count - fromIndex, array->elemsize, comparator);
	else
		res = mylsearch(key, ArrayNth(array, fromIndex),
						array->count - fromIndex, array->elemsize, comparator);
	if (res != NULL)
		return ((char *)res - (char *)array->list) / array->elemsize;
	else
		return NOT_FOUND;
}

void ArrayMap(DArray array, ArrayMapFn fn, void *clientData)
{
	int i;

	for (i = 0; i < array->count; i++)
		fn(ArrayNth(array, i), clientData);
}

static void *mylsearch(const void *key, void *base, int count, int size,
					   ArrayCompareFn comparator)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (comparator(key, (char *)base + size * i) == 0)
			return (char *)base + size * i;
	}
	return NULL;
}

static void *mybsearch(const void *elem, void *base, int num, int elemsize,
					   ArrayCompareFn comparator)
{
	int L, H, I, C;

	L = 0;
	H = num - 1;
	while (L <= H)
	{
		I = (L + H) >> 1;
		C = comparator((char *)base + I * elemsize, elem);
		if (C < 0)
			L = I + 1;
		else
			H = I - 1;
	}
	return (char *)base + L * elemsize;
}
