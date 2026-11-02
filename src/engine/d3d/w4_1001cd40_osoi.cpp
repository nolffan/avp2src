// *********************************************************************** //
//
//  MODULE:   W4_1001CD40_OSOI (.CPP)  -- /Os /Oi-
//
//  PURPOSE:  Two functions the d3d.ren link referenced by address and
//            nobody had written, that CALL CRT routines the intrinsics
//            would expand: 0x1001cd40 (CMoArray<T32>::InternalNiceSetSize,
//            memcpy at 0x100805d0) and 0x1001e950 (the environment-map
//            coordinates, sqrt at 0x1007fc98).
//
//  Flag set: /Os /Oi- -- /Os padding (a 4-byte-aligning `mov %esi,%esi` /
//  `lea 0x0(%esi),%esi`, then 0x90 bytes) and CRT calls, not intrinsics.
//  Measured: 0x1001cd40 is byte-identical here and 313 bytes off at /Os
//  (memcpy expanded inline).
//
// *********************************************************************** //

#include <math.h>
#include "ltbasedefs.h"

// stdlith's LAlloc, as an abstract interface (never defined here, so no
// vtable is emitted).  `struct` because that is how the callers' mangled
// references spell it.
struct IAllocator
{
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};

// dynarray.h's DefaultCache; its accessors are out of line in the shipped
// build (0x1000a660..0x1000a690, reconstructed in d3dbatchc.cpp).
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};


struct T1001cd40 { uint32 m_d[8]; };

// ---- 0x1001cd40 -- CMoArray<T32>::InternalNiceSetSize, verbatim from the header.
//      The bFast arm CALLS memcpy (0x100805d0).
struct Arr1001cd40
{
	uint32*      m_pVTable;                             // +0x00
	T1001cd40* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	T1001cd40* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001ced0
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	int32 sub_1001cd40(uint32 newSize, int32 bFast, IAllocator* pAlloc);
};

int32 Arr1001cd40::sub_1001cd40(uint32 newSize, int32 bFast, IAllocator* pAlloc)
{
	T1001cd40* pNewArray;
	uint32 i, nToCopy;

	if (newSize < m_nElements)
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() + (m_nElements - newSize));
		m_nElements = newSize;
		return 1;
	}
	else if (newSize > m_nElements && (m_nElements + m_Cache.GetCacheSize()) >= newSize)
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - (newSize - m_nElements));
		m_nElements = newSize;
		return 1;
	}
	else if (newSize == m_nElements)
	{
		return 1;
	}

	pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
	if (!pNewArray)
		return 0;

	nToCopy = m_nElements;
	if (nToCopy > newSize)
		nToCopy = newSize;

	if (bFast)
	{
		memcpy(pNewArray, m_pArray, sizeof(T1001cd40) * nToCopy);
	}
	else
	{
		for (i = 0; i < nToCopy; i++)
		{
			pNewArray[i] = m_pArray[i];
		}
	}

	_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
	m_pArray = pNewArray;
	m_nElements = newSize;
	m_Cache.SetCacheSize(m_Cache.GetWantedCache());

	return 1;
}




// ======================================================================= //
//  0x1001e950 -- the environment-map texture coordinates of one vertex:
//  reflect the eye->vertex direction about the plane normal, normalise
//  it (ltvector.h's Norm, whose Mag CALLS the CRT sqrt 0x1007fc98 --
//  hence /Oi-), and map the two components that face away from the
//  dominant axis into [0,1] with -0.5*c+0.5.  The 0.54439 threshold is a
//  double literal (`fcompl` against 0x100a50b0).
// ======================================================================= //
struct LTVec  { float x, y, z; };
struct SPlane { LTVec m_vNormal; float m_fDist; };

void sub_1001e950(const LTVec* pEye, const LTVec* pPos, const SPlane* pPlane,
                  float* pU, float* pV)
{
	const LTVector& vEye = *(const LTVector*)pEye;
	const LTVector& vPos = *(const LTVector*)pPos;
	const LTVector& vN   = *(const LTVector*)&pPlane->m_vNormal;

	LTVector vDir = vPos - vEye;
	LTVector vRefl = vDir - vN * (vDir.Dot(vN) * 2.0f);
	vRefl.Norm();

	if (vN.y * vN.y > 0.54439 || vN.x * vN.x < vN.z * vN.z)
	{
		*pU = -0.5f * vRefl.x + 0.5f;
		*pV = -0.5f * vRefl.y + 0.5f;
	}
	else
	{
		*pU = -0.5f * vRefl.z + 0.5f;
		*pV = -0.5f * vRefl.y + 0.5f;
	}
}
