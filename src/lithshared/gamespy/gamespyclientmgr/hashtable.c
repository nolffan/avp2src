/* hashtable.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * GameSpy's hash table: see hashtable.h for the functions' descriptions. A
 * fixed, dynamically allocated array of buckets, each a DArray.
 */
#include <stdlib.h>
#include <string.h>
#include "darray.h"
#include "hashtable.h"

struct HashImplementation
{
	DArray *buckets;
	int nbuckets;
	TableElementFreeFn freefn;
	TableHashFn hashfn;
	TableCompareFn compfn;
};

HashTable TableNew(int elemSize, int nBuckets,
				   TableHashFn hashFn, TableCompareFn compFn,
				   TableElementFreeFn freeFn)
{
	return TableNew2(elemSize, nBuckets, 4, hashFn, compFn, freeFn);
}

HashTable TableNew2(int elemSize, int nBuckets, int nChains,
					TableHashFn hashFn, TableCompareFn compFn,
					TableElementFreeFn freeFn)
{
	HashTable table;
	int i;

	table = (HashTable)malloc(sizeof(struct HashImplementation));
	table->buckets = (DArray *)malloc(nBuckets * sizeof(DArray));
	for (i = 0; i < nBuckets; i++)
		table->buckets[i] = ArrayNew(elemSize, nChains, freeFn);
	table->nbuckets = nBuckets;
	table->freefn = freeFn;
	table->compfn = compFn;
	table->hashfn = hashFn;
	return table;
}

void TableFree(HashTable table)
{
	int i;

	for (i = 0; i < table->nbuckets; i++)
		ArrayFree(table->buckets[i]);
	free(table->buckets);
	free(table);
}

int TableCount(HashTable table)
{
	int i, count = 0;

	for (i = 0; i < table->nbuckets; i++)
		count += ArrayLength(table->buckets[i]);
	return count;
}

void TableEnter(HashTable table, const void *newElem)
{
	int hash, itempos;

	hash = table->hashfn(newElem, table->nbuckets);
	itempos = ArraySearch(table->buckets[hash], newElem, table->compfn, 0, 0);
	if (itempos == NOT_FOUND)
		ArrayAppend(table->buckets[hash], newElem);
	else
		ArrayReplaceAt(table->buckets[hash], newElem, itempos);
}

int TableRemove(HashTable table, const void *delElem)
{
	int hash, itempos;

	hash = table->hashfn(delElem, table->nbuckets);
	itempos = ArraySearch(table->buckets[hash], delElem, table->compfn, 0, 0);
	if (itempos == NOT_FOUND)
		return 0;
	else
		ArrayDeleteAt(table->buckets[hash], itempos);
	return 1;
}

void *TableLookup(HashTable table, const void *elemKey)
{
	int hash, itempos;

	hash = table->hashfn(elemKey, table->nbuckets);
	itempos = ArraySearch(table->buckets[hash], elemKey, table->compfn, 0, 0);
	if (itempos == NOT_FOUND)
		return NULL;
	else
		return ArrayNth(table->buckets[hash], itempos);
}

void TableMap(HashTable table, TableMapFn fn, void *clientData)
{
	int i;

	for (i = 0; i < table->nbuckets; i++)
		ArrayMap(table->buckets[i], fn, clientData);
}
