// *********************************************************************** //
//  ph/vertexbuffer.cpp -- Primal Hunt's d3d.ren, TU43 (0x1003b3e0..0x1003c136),
//  /O1.  The last renderer object.
//
//  The DirectX 7 vertex-buffer classes: CVertexBuffer, a ring of
//  IDirect3DVertexBuffer7s that are locked, filled and drawn in turn
//  (vtable 0x10048524, its two last slots pure), and CVertexBufferCache,
//  which keys each buffer and keeps it for a few frames (vtable 0x100485a4;
//  one instance, 0x100995a8).  TU01's four model vertex-buffer objects
//  derive from CVertexBuffer.  The rest is the two CMoArray instantiations
//  they hold (vtables 0x10048544 and 0x10048574), from StdLith's
//  dynarray.h; the link folds most of their members into identical
//  earlier copies.
// *********************************************************************** //

#include <d3d.h>
#include <string.h>

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// ------------------------------------------------------------------------------ //
// The classes.
// ------------------------------------------------------------------------------ //

// One cached vertex buffer's key, size and age.
struct VBCacheEntry
{
			VBCacheEntry()
			{
				m_Key1 = 0;
				m_Key2 = 0;
				m_nVerts = 0;
				m_nAge = 0;
			}

			VBCacheEntry(uint32 key1, uint32 key2, uint32 nVerts, uint32 nAge)
			{
				m_Key1 = key1;
				m_Key2 = key2;
				m_nVerts = nVerts;
				m_nAge = nAge;
			}

	uint32	m_Key1;
	uint32	m_Key2;
	uint32	m_nVerts;
	uint32	m_nAge;		// Frames left before the slot can be reused.
};


class CVertexBuffer
{
public:

							CVertexBuffer();
	virtual					~CVertexBuffer();

	// Unlock the current buffer and draw nVerts of it, then move on.
	virtual HRESULT			Flush(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type, uint32 nVerts);
	virtual HRESULT			FlushIndexed(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type,
								uint32 nVerts, uint16 *pIndices, uint32 nIndices);

	virtual void			Term();
	virtual LTBOOL			Init(LPDIRECT3D7 pD3D, uint32 nVertsPerVB, uint32 nVBs,
								uint32 fvfType, uint32 bVideoMem);

	// Lock the current buffer at the current vertex.
	virtual void*			Lock();

	virtual uint32			GetVertexSize()=0;
	virtual LTBOOL			CreateVBs(LPDIRECT3D7 pD3D)=0;

	void					NextVB();
	void					Discard();

protected:

	void					ReleaseVBs();

public:

	CMoArray<LPDIRECT3DVERTEXBUFFER7>	m_VBs;

	uint32					m_iCurVB;
	uint32					m_nCurVertex;
	uint32					m_nVertsPerVB;
	uint32					m_nNumVBs;
	uint32					m_FVFType;
	uint32					m_bVideoMem;

	LTBOOL					m_bInitted;
	LTBOOL					m_bLocked;
	void					*m_pLockedData;
};


class CVertexBufferCache : public CVertexBuffer
{
public:

							CVertexBufferCache();

	virtual HRESULT			Flush(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type, uint32 nVerts);
	virtual void			Term();
	virtual void*			Lock();
	virtual uint32			GetVertexSize();
	virtual LTBOOL			CreateVBs(LPDIRECT3D7 pD3D);

	// Make the buffer keyed (key1, key2) current; FALSE if it isn't cached.
	LTBOOL					Find(uint32 key1, uint32 key2);

	// Take a free buffer for (key1, key2), growing the cache if bGrow.
	LTBOOL					Alloc(uint32 key1, uint32 key2, uint32 nVerts, LTBOOL bGrow);

	void					Optimize();
	void					AgeCache();

public:

	CMoArray<VBCacheEntry>	m_Cache;

	uint32					m_iNextSlot;
	uint32					m_VertexSize;
	uint32					m_FVF;
	LTBOOL					m_bFilled;		// The current buffer wants optimising.
	uint32					m_nCacheDelay;
	LPDIRECT3D7				m_pD3D;
	LPDIRECT3DDEVICE7		m_pDevice;

	// The model vertex-buffer cache (TU01's).
	static CVertexBufferCache	s_ModelVBCache;
};


// ------------------------------------------------------------------------------ //
// CVertexBuffer.
// ------------------------------------------------------------------------------ //

// 0x1003b3e0
CVertexBuffer::CVertexBuffer() :
	m_iCurVB(0),
	m_nCurVertex(0),
	m_nVertsPerVB(768),
	m_nNumVBs(1),
	m_FVFType(0),
	m_bVideoMem(0),
	m_bInitted(LTFALSE),
	m_bLocked(LTFALSE),
	m_pLockedData(LTNULL)
{
}

// 0x1003b436 (scalar deleting destructor 0x1003b41a)
CVertexBuffer::~CVertexBuffer()
{
	Term();
}

// 0x1003b45b
HRESULT CVertexBuffer::Flush(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type, uint32 nVerts)
{
	HRESULT hResult;
	LPDIRECT3DVERTEXBUFFER7 pVB;
	uint32 iFirst;

	if(!m_bInitted)
		return 0;

	if(!m_bLocked)
		return 0;

	m_VBs[m_iCurVB]->Unlock();
	m_pLockedData = LTNULL;
	m_bLocked = LTFALSE;

	if(!nVerts)
		return 0;

	iFirst = m_nCurVertex;
	pVB = m_VBs[m_iCurVB];
	hResult = pDevice->DrawPrimitiveVB(type, pVB, iFirst, nVerts, 0);

	m_nCurVertex += nVerts;
	if(m_nCurVertex >= m_nVertsPerVB)
	{
		NextVB();
		m_nCurVertex = 0;
	}

	return hResult;
}

// 0x1003b4cc
void CVertexBuffer::NextVB()
{
	m_iCurVB++;
	if(m_iCurVB >= m_nNumVBs)
		m_iCurVB = 0;
}

// 0x1003b4dc
HRESULT CVertexBuffer::FlushIndexed(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type,
	uint32 nVerts, uint16 *pIndices, uint32 nIndices)
{
	HRESULT hResult;
	LPDIRECT3DVERTEXBUFFER7 pVB;
	uint32 iFirst;

	if(!m_bInitted)
		return 0;

	if(!m_bLocked)
		return 0;

	m_VBs[m_iCurVB]->Unlock();
	m_pLockedData = LTNULL;
	m_bLocked = LTFALSE;

	if(!nVerts)
		return 0;

	iFirst = m_nCurVertex;
	pVB = m_VBs[m_iCurVB];
	hResult = pDevice->DrawIndexedPrimitiveVB(type, pVB, iFirst, nVerts,
		pIndices, nIndices, 0);

	m_nCurVertex += nVerts;
	if(m_nCurVertex >= m_nVertsPerVB)
	{
		NextVB();
		m_nCurVertex = 0;
	}

	return hResult;
}

// 0x1003b557
void CVertexBuffer::Discard()
{
	NextVB();
	m_nCurVertex = 0;
}

// 0x1003b565
void CVertexBuffer::Term()
{
	if(m_bInitted)
	{
		if(m_bLocked)
		{
			m_VBs[m_iCurVB]->Unlock();
			m_bLocked = LTFALSE;
			m_pLockedData = LTNULL;
		}

		ReleaseVBs();
		m_bInitted = LTFALSE;
		m_iCurVB = 0;
		m_nCurVertex = 0;
	}
}

// 0x1003b59d
LTBOOL CVertexBuffer::Init(LPDIRECT3D7 pD3D, uint32 nVertsPerVB, uint32 nVBs,
	uint32 fvfType, uint32 bVideoMem)
{
	if(m_bInitted && nVertsPerVB == m_nVertsPerVB && nVBs == m_nNumVBs)
		return LTTRUE;

	Term();

	m_nVertsPerVB = (nVertsPerVB > 0) ? nVertsPerVB : m_nVertsPerVB;
	m_nNumVBs = (nVBs > 0) ? nVBs : m_nNumVBs;
	m_FVFType = fvfType;
	m_bVideoMem = bVideoMem;

	m_VBs.Init(nVBs);

	if(!CreateVBs(pD3D))
		return LTFALSE;

	m_bInitted = LTTRUE;
	return LTTRUE;
}

// 0x1003b611
void* CVertexBuffer::Lock()
{
	HRESULT hResult;
	uint32 flags, nCurVertex;
	uint8 *pData;

	if(m_bLocked)
		return m_pLockedData;

	flags = m_nCurVertex ? (DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_NOOVERWRITE) : (DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_DISCARDCONTENTS);
	hResult = m_VBs[m_iCurVB]->Lock(flags, &m_pLockedData, LTNULL);
	if(FAILED(hResult))
		return LTNULL;

	// Past the vertices already drawn from this buffer.
	pData = (uint8*)m_pLockedData;
	nCurVertex = m_nCurVertex;
	m_pLockedData = pData + GetVertexSize() * nCurVertex;
	m_bLocked = LTTRUE;
	return m_pLockedData;
}

// 0x1003b66f
void CVertexBuffer::ReleaseVBs()
{
	uint32 i;

	for(i=0; i < m_VBs.GetSize(); i++)
	{
		if(m_VBs[i])
			m_VBs[i]->Release();
	}

	m_VBs.Term();
}


// ------------------------------------------------------------------------------ //
// CVertexBufferCache.
// ------------------------------------------------------------------------------ //

// 0x100995a8: initialiser 0x1003b6a2, its atexit destructor 0x1003b6b8
// (the destructor itself, compiler-generated, 0x1003b6d3).  A static data
// member: that is what guards the atexit destructor (a plain global's is
// unguarded).
CVertexBufferCache CVertexBufferCache::s_ModelVBCache;

// 0x1003b6f4
CVertexBufferCache::CVertexBufferCache() :
	m_iNextSlot(0),
	m_VertexSize(0),
	m_FVF(0),
	m_bFilled(LTFALSE),
	m_nCacheDelay(4),
	m_pD3D(LTNULL),
	m_pDevice(LTNULL)
{
	m_FVFType = 1;
}

// 0x1003b730
uint32 CVertexBufferCache::GetVertexSize()
{
	return m_VertexSize;
}

// 0x1003b750
void* CVertexBufferCache::Lock()
{
	D3DVERTEXBUFFERDESC desc;
	LPDIRECT3DVERTEXBUFFER7 pVB;

	if(m_VBs[m_iCurVB])
		return CVertexBuffer::Lock();

	// Make this slot's buffer the first time it's used.
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwCaps = D3DVBCAPS_WRITEONLY;
	if(!m_bVideoMem)
		desc.dwCaps = D3DVBCAPS_WRITEONLY | D3DVBCAPS_SYSTEMMEMORY;
	desc.dwFVF = m_FVF;
	desc.dwNumVertices = m_nVertsPerVB;

	if(SUCCEEDED(m_pD3D->CreateVertexBuffer(&desc, &pVB, 0)))
	{
		m_VBs[m_iCurVB] = pVB;
		return CVertexBuffer::Lock();
	}

	return LTNULL;
}

// 0x1003b7cd
LTBOOL CVertexBufferCache::CreateVBs(LPDIRECT3D7 pD3D)
{
	uint32 i;

	m_pD3D = pD3D;
	m_Cache.SetSize(m_nNumVBs);

	for(i=0; i < m_nNumVBs; i++)
	{
		VBCacheEntry &entry = m_Cache[i];

		entry.m_Key1 = 0;
		entry.m_nAge = 0;
		entry.m_nVerts = 0;
		entry.m_Key2 = 0;
		m_VBs[i] = LTNULL;
	}

	return LTTRUE;
}

// 0x1003b81b
LTBOOL CVertexBufferCache::Find(uint32 key1, uint32 key2)
{
	uint32 i;

	if(m_bFilled)
		Optimize();

	for(i=0; i < m_nNumVBs; i++)
	{
		if(m_Cache[i].m_Key1 == key1 && m_Cache[i].m_Key2 == key2)
		{
			m_iCurVB = i;
			m_nCurVertex = 0;
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// 0x1003b862
LTBOOL CVertexBufferCache::Alloc(uint32 key1, uint32 key2, uint32 nVerts, LTBOOL bGrow)
{
	uint32 iSlot;
	LPDIRECT3DVERTEXBUFFER7 pNewVB;

	if(m_bFilled)
		Optimize();

	// Look for a slot that's aged out.
	iSlot = m_iNextSlot;
	while(m_Cache[iSlot].m_nAge)
	{
		iSlot++;
		if(iSlot >= m_nNumVBs)
			iSlot = 0;

		if(iSlot == m_iNextSlot)
		{
			// They're all in use.
			if(!bGrow)
				return LTFALSE;

			pNewVB = LTNULL;
			if(!m_VBs.Append(pNewVB))
				return LTFALSE;

			m_iCurVB = m_nNumVBs;
			m_nNumVBs = (m_nNumVBs + 1 > 0) ? m_nNumVBs + 1 : m_nNumVBs;

			m_Cache.Append(VBCacheEntry(key1, key2, nVerts, m_nCacheDelay));
			goto Found;
		}
	}

	m_iCurVB = iSlot;
	if(m_VBs[iSlot])
	{
		m_VBs[iSlot]->Release();
		m_VBs[m_iCurVB] = LTNULL;
	}

	m_Cache[iSlot].m_Key1 = key1;
	m_Cache[iSlot].m_Key2 = key2;

Found:
	m_iNextSlot = m_iCurVB + 1;
	if(m_iNextSlot >= m_nNumVBs)
		m_iNextSlot = 0;

	m_bFilled = LTTRUE;
	m_nCurVertex = 0;
	m_nVertsPerVB = (nVerts > 0) ? nVerts : m_nVertsPerVB;
	m_Cache[m_iCurVB].m_nVerts = nVerts;
	return LTTRUE;
}

// 0x1003b975
void CVertexBufferCache::Optimize()
{
	m_bFilled = LTFALSE;
	m_VBs[m_iCurVB]->Optimize(m_pDevice, 0);
}

// 0x1003b98e
HRESULT CVertexBufferCache::Flush(LPDIRECT3DDEVICE7 pDevice, D3DPRIMITIVETYPE type, uint32 nVerts)
{
	HRESULT hResult;
	LPDIRECT3DVERTEXBUFFER7 pVB;
	uint32 iVB, iFirst;

	m_pDevice = pDevice;
	m_Cache[m_iCurVB].m_nAge = m_nCacheDelay;

	if(m_bFilled)
	{
		// The base class moves on to the next buffer; stay on this one.
		iVB = m_iCurVB;
		hResult = CVertexBuffer::Flush(pDevice, type, nVerts);
		m_iCurVB = iVB;
	}
	else
	{
		iFirst = m_nCurVertex;
		pVB = m_VBs[m_iCurVB];
		hResult = pDevice->DrawPrimitiveVB(type, pVB, iFirst, nVerts, 0);
		m_nCurVertex += nVerts;
	}

	return hResult;
}

// 0x1003b9ef
void CVertexBufferCache::Term()
{
	CVertexBuffer::Term();
	m_Cache.Term();
	m_iNextSlot = 0;
	m_bFilled = LTFALSE;
}

// 0x1003ba10
void CVertexBufferCache::AgeCache()
{
	uint32 i;

	for(i=0; i < m_Cache.GetSize(); i++)
	{
		if(m_Cache[i].m_nAge > 0)
			m_Cache[i].m_nAge--;
	}
}
