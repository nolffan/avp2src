
#include "l_allocator.h"


LAlloc g_DefAlloc;



// -------------------------------------------------------------------------------- //
// LAllocCount implementation.
// -------------------------------------------------------------------------------- //

LAllocCount::LAllocCount(LAlloc *pDelegate)
{
	m_pDelegate = pDelegate;
	
	ClearCounts();
}


void LAllocCount::ClearCounts()
{
	m_nTotalAllocations = 0;
	m_nTotalFrees = 0;
	
	m_TotalMemoryAllocated = 0;
	m_nCurrentAllocations = 0;

	m_nAllocationFailures = 0;
}


// Default allocator functions (declared extern in l_allocator.h; the
// LT2-era lib defines them here — NULL for zero-size, plain malloc/free).
void* DefStdlithAlloc(DWORD size)
{
	if(size == 0)
		return NULL;

	return malloc(size);
}

void DefStdlithFree(void *ptr)
{
	if(ptr)
		free(ptr);
}


void* LAllocCount::Alloc(DWORD size)
{
	void *pRet;
	
	if(size == 0)
		return NULL;

	pRet = m_pDelegate->Alloc(size);
	if(pRet)
	{
		m_nTotalAllocations++;
		m_TotalMemoryAllocated += size;
		m_nCurrentAllocations++;
	}
	else
	{
		m_nAllocationFailures++;
	}

	return pRet;
}


void LAllocCount::Free(void *ptr)
{
	if(!ptr)
		return;

	m_pDelegate->Free(ptr);
	m_nCurrentAllocations--;
	m_nTotalFrees++;
}



// -------------------------------------------------------------------------------- //
// LAllocSimpleBlock implementation.
// -------------------------------------------------------------------------------- //

LAllocSimpleBlock::LAllocSimpleBlock()
{
	Clear();
}


LAllocSimpleBlock::~LAllocSimpleBlock()
{
	Term();
}


BOOL LAllocSimpleBlock::Init(LAlloc *pDelegate, DWORD blockSize)
{
	Term();

	blockSize = (blockSize + 3) & ~3;

	if(blockSize > 0)
	{
		m_pBlock = (BYTE*)pDelegate->Alloc(blockSize);
		if(!m_pBlock)
			return FALSE;
	}

	m_pDelegate = pDelegate;
	m_BlockSize = blockSize;
	m_CurBlockPos = 0;

	return TRUE;
}


void LAllocSimpleBlock::Term()
{
	if(m_pDelegate)
	{
		m_pDelegate->Free(m_pBlock);
	}

	Clear();
}


void* LAllocSimpleBlock::Alloc(DWORD size)
{
	BYTE *pRet;

	if(size == 0)
		return NULL;

	// DWord Align by default...
	size = ((size + 3) & ~3);

	if((m_CurBlockPos + size) > m_BlockSize)
		return NULL;

	pRet = &m_pBlock[m_CurBlockPos];
	m_CurBlockPos += size;
	return pRet;
}


void LAllocSimpleBlock::Free(void *ptr)
{
}


void LAllocSimpleBlock::Clear()
{
	m_pDelegate = NULL;
	m_pBlock = NULL;
	m_CurBlockPos = 0;
	m_BlockSize = 0;
}
