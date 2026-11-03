
#include "dynarray.h"


DWORD MoArray_FindElementMemcmp(
	const void *pToFind,
	const void *pArray, 
	DWORD nElements, 
	DWORD elementSize)
{
	const char *pCur = (const char *)pToFind;
	DWORD i;


	for(i=0; i < nElements; i++)
	{
		if(memcmp(pCur, pToFind, elementSize) == 0)
			return i;
	
		pCur += elementSize;
	}

	return BAD_INDEX;
}
