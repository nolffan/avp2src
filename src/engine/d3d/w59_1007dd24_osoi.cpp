// *********************************************************************** //
//
//  w59_1007dd24_osoi.cpp -- d3d.ren, /Os /Oi- (as d3dr5c.cpp, where this
//  function was written first; moved here later).
//
//  0x1007dd24 -- 188 bytes, 1 caller (0x1007e254, inside 0x1007e1c0 in
//  d3dr5c.cpp).  The vertex-buffer batcher's FLUSH: unlock the current
//  vertex buffer, DrawPrimitiveVB it, advance and wrap.  The class layout,
//  the DirectX 7 slots and what pins each statement are in d3dr5c.cpp's
//  FAMILY 2 banner and its old 0x1007dd24 banner; they are not repeated.
//
//  0x1007dde0 -- 216 bytes, its INDEXED twin (DrawIndexedPrimitiveVB),
//  moved here from w22_1007dde0_os.cpp (/Os there; /Oi- changes nothing in
//  it), is at the end of this file and is byte-identical the same way.
//
//  BYTE-IDENTICAL (both).  What it took, each measured:
//
//    * m_VBs (+0x04) is the REAL StdLith CMoArray<IDirect3DVertexBuffer7*>
//      from dynarray.h, so `m_VBs[m_iCurVB]` is the inlined operator[] ->
//      Get().  That alone turns the shipped "frame-save" style on: no
//      callee-saved register is pushed in the prologue, %edi/%esi are
//      saved into frame slots around their short uses (-0x8, -0x4) and
//      `this` is kept in -0x14 across the calls, and the frame is the
//      shipped 0x30.  With the hand `m_pVBs` pointer (d3dr5c.cpp) icl
//      pushed %esi/%ebx and kept `this` in %ebx: 147 bytes differing at
//      157 bytes long.  This is the save style d3dr5c.cpp, d3dhigh.cpp
//      and d3dr5b.cpp recorded as unreproduced: it comes from code that
//      was inlined.
//    * the unlock reads the element ONCE into a local (shipped computes
//      it once and pushes it as `this` for Unlock);
//    * the wrap test is a `switch` whose default arm is the wrap call --
//      (for 0x1007dde0 the class name w22 chose for icl's cold-word
//      heuristic, VBDefault..., is kept only for the link: with the switch
//      a name without "Default" compiles to the same bytes) --
//      icl sinks that arm below the epilogue (`jae` out, `jmp` back) and
//      spills the result around the call (-0xc), exactly as the shipped
//      code does and as 0x1007e1c0 (d3dr5c.cpp) needed for its own sunk
//      arm.  As an `if`, an early return or `!(a < b)`, the call stays
//      inline and the result lives in a pushed %ebx;
//    * the vertex buffer and m_nCurVertex read into block locals before
//      DrawPrimitiveVB, again as in 0x1007e1c0: icl then loads both
//      before storing the arguments (with the buffer alone: 43 bytes
//      differ).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// The two DirectX 7 interfaces, cut down to the slots used -- the same
// declarations as d3dr5c.cpp.
struct D3DVertexBuffer7R5C;
struct D3DVertexBuffer7R5CVtbl
{
	void*	m_Slot00[4];                                        // +0x00..+0x0c
	long	(__stdcall* Unlock)(D3DVertexBuffer7R5C*);           // +0x10
};
struct D3DVertexBuffer7R5C
{
	D3DVertexBuffer7R5CVtbl*	lpVtbl;
};

struct D3DDevice7R5C;
struct D3DDevice7R5CVtbl
{
	void*	m_Slot00[31];                                       // +0x00..+0x78
	long	(__stdcall* DrawPrimitiveVB)(D3DDevice7R5C*, uint32,
						D3DVertexBuffer7R5C*, uint32, uint32,
						uint32);                                // +0x7c
};
struct D3DDevice7R5C
{
	D3DDevice7R5CVtbl*	lpVtbl;
};


struct Batch1007dd24
{
	void*							m_pVtbl;            // +0x00
	CMoArray<D3DVertexBuffer7R5C*>	m_VBs;              // +0x04..+0x17
	uint32							m_iCurVB;           // +0x18
	uint32							m_nCurVertex;       // +0x1c
	uint32							m_nVertsPerVB;      // +0x20
	uint8							m_pad_24[0x30 - 0x24];
	uint32							m_n30;              // +0x30
	void*							m_pLocked;          // +0x34
	uint32							m_n38;              // +0x38

	void	sub_1007dec4();                             // the wrap, d3dr2d.cpp
	long	sub_1007dd24(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
};


long Batch1007dd24::sub_1007dd24(D3DDevice7R5C* pDev, uint32 nPrimType,
								 uint32 nCount)
{
	long	hr;

	if (!m_n30)
		return 0;

	if (!m_pLocked)
		return 0;

	D3DVertexBuffer7R5C* pVB = m_VBs[m_iCurVB];
	pVB->lpVtbl->Unlock(pVB);

	m_n38 = 0;
	m_pLocked = LTNULL;

	if (!nCount)
		return 0;

	{
		D3DVertexBuffer7R5C*	pDrawVB = m_VBs[m_iCurVB];
		uint32					nVertex = m_nCurVertex;

		hr = pDev->lpVtbl->DrawPrimitiveVB(pDev, nPrimType, pDrawVB,
										   nVertex, nCount, 0);
	}

	m_nCurVertex += nCount;

	switch (m_nCurVertex >= m_nVertsPerVB)
	{
		case 0:
			break;
		default:
			sub_1007dec4();
	}

	return hr;
}


// ======================================================================= //
//  0x1007dde0 -- 216 bytes.  The INDEXED twin of 0x1007dd24.
// ======================================================================= //
struct VB7dde0;
struct VB7dde0Vtbl
{
	void*	m_Slot00[4];                                        // +0x00..+0x0c
	long	(__stdcall* Unlock)(VB7dde0*);                       // +0x10
};
struct VB7dde0 { VB7dde0Vtbl* lpVtbl; };

struct Dev7dde0;
struct Dev7dde0Vtbl
{
	void*	m_Slot00[32];                                       // +0x00..+0x7c
	long	(__stdcall* DrawIndexedPrimitiveVB)(Dev7dde0*, uint32 nPrimType,
						VB7dde0* pVB, uint32 nStartVertex,
						uint32 nNumVertices, uint16* pIndices,
						uint32 nIndexCount, uint32 nFlags);     // +0x80
};
struct Dev7dde0 { Dev7dde0Vtbl* lpVtbl; };

struct VBDefault1007dde0
{
	void*				m_pVtbl;                        // +0x00
	CMoArray<VB7dde0*>	m_VBs;                          // +0x04..+0x17
	uint32		m_iCurVB;                               // +0x18
	uint32		m_nCurVertex;                           // +0x1c
	uint32		m_nVertsPerVB;                          // +0x20
	uint8		m_pad_24[0x30 - 0x24];
	uint32		m_bInit;                                // +0x30
	uint32		m_pLocked;                              // +0x34
	uint32		m_n38;                                  // +0x38

	void	sub_1007dec4();                             // the wrap
	long	sub_1007dde0(Dev7dde0* pDev, uint32 nPrimType, uint32 nVerts,
	                     uint16* pIndices, uint32 nIndices);
};

long VBDefault1007dde0::sub_1007dde0(Dev7dde0* pDev, uint32 nPrimType,
                                     uint32 nVerts, uint16* pIndices,
                                     uint32 nIndices)
{
	long	hr;

	if (!m_bInit)
		return 0;

	if (!m_pLocked)
		return 0;

	VB7dde0* pVB = m_VBs[m_iCurVB];
	pVB->lpVtbl->Unlock(pVB);

	m_n38 = 0;
	m_pLocked = 0;

	if (!nVerts)
		return 0;

	{
		VB7dde0*	pDrawVB = m_VBs[m_iCurVB];
		uint32		nVertex = m_nCurVertex;

		hr = pDev->lpVtbl->DrawIndexedPrimitiveVB(pDev, nPrimType, pDrawVB,
		                                          nVertex, nVerts, pIndices,
		                                          nIndices, 0);
	}

	m_nCurVertex += nVerts;

	switch (m_nCurVertex >= m_nVertsPerVB)
	{
		case 0:
			break;
		default:
			sub_1007dec4();
	}

	return hr;
}
