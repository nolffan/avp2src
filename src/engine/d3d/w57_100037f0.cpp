// *********************************************************************** //
//  w57_100037f0 -- d3d.ren: 0x100037f0, 240 bytes, /O2.
//
//  The sibling of 0x100038e0 (d3dr5c_o2.cpp, identical): slot 7 of the
//  vertex-buffer batchers' vtables at 0x10089260 and 0x100892a0, where
//  0x100038e0 is slot 7 of 0x10089280 / 0x100892c0.  The two bodies are
//  the same instruction for instruction (objdump of both, diffed): the
//  only operand that differs is the FVF -- 0x1c2 / 0x1c4 here against
//  0x2c2 / 0x2c4 there, i.e. D3DFVF_XYZ (2) or XYZRHW (4) with DIFFUSE |
//  SPECULAR and ONE texture-coordinate set (TEX1, 0x100) where 0x100038e0
//  has two (TEX2, 0x200) -- and the branch displacements that follow from
//  the address.  The constants are kept raw, as d3dr5c_o2.cpp keeps them.
//
//  So it is written as that function is, in a file of its own (adding it to
//  d3dr5c_o2.cpp risks the term-order effects of what a TU compiles first),
//  on the same struct spelling so that the two callees resolve through the
//  aliases that file already has: Batch100038e0::sub_1007e030 and
//  CMoArray<D3DVertexBuffer7R5C*>::operator[] (0x1007c870).  The struct
//  name is the sibling's; the member carries its own address, which is
//  what the byte comparison maps.  IDENTICAL.
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>

struct D3DVertexBuffer7R5C;

struct D3D7R5C;
struct D3DVertexBufferDescR5C
{
	uint32	dwSize;                                             // +0x00
	uint32	dwCaps;                                             // +0x04
	uint32	dwFVF;                                              // +0x08
	uint32	dwNumVertices;                                      // +0x0c
};
struct D3D7R5CVtbl
{
	void*	m_Slot00[5];                                        // +0x00..+0x10
	long	(__stdcall* CreateVertexBuffer)(D3D7R5C*,
						D3DVertexBufferDescR5C*,
						D3DVertexBuffer7R5C**, uint32);         // +0x14
};
struct D3D7R5C
{
	D3D7R5CVtbl*	lpVtbl;
};

#define D3DVBCAPS_R5C_SYSTEMMEMORY	0x00000800
#define D3DVBCAPS_R5C_WRITEONLY		0x00010000

// d3dr5c_o2.cpp's CMoArray stand-in, verbatim: the template's name (it
// contains "fault") is what hosts the m_VBs[i] store arm past the epilogue.
struct DefaultCache;

template<class T, class C = DefaultCache>
class CMoArray
{
public:
	T&		operator[](uint32 nIndex) const;            // 0x1007c870

	uint8	m_pad_00[0x14];                             // Arr1007c480's 0x14
};

struct Batch100038e0
{
	uint8			m_pad_00[0x04];
	CMoArray<D3DVertexBuffer7R5C*>	m_VBs;              // +0x04
	uint8			m_pad_18[0x20 - 0x18];
	uint32			m_nVertsPerVB;                      // +0x20
	uint32			m_nNumVBs;                          // +0x24
	uint32			m_n28;                              // +0x28
	uint32			m_bVidMem;                          // +0x2c

	void	sub_1007e030();                             // 0x1007e030
	LTBOOL	sub_100037f0(D3D7R5C* pD3D);
};


LTBOOL Batch100038e0::sub_100037f0(D3D7R5C* pD3D)
{
	D3DVertexBufferDescR5C	desc;
	D3DVertexBuffer7R5C*	pVB;
	uint32					i;
	LTBOOL					bOk;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);
	desc.dwCaps = D3DVBCAPS_R5C_WRITEONLY;

	if (m_n28)
		desc.dwFVF = 0x1c2;
	else
		desc.dwFVF = 0x1c4;

	if (!m_bVidMem)
		desc.dwCaps |= D3DVBCAPS_R5C_SYSTEMMEMORY;

	desc.dwNumVertices = m_nVertsPerVB;

	bOk = LTTRUE;
	for (i = 0; i < m_nNumVBs && bOk; i++)
	{
		if (pD3D->lpVtbl->CreateVertexBuffer(pD3D, &desc, &pVB, 0) < 0)
			bOk = LTFALSE;
		else
			m_VBs[i] = pVB;
	}

	if (!bOk)
		sub_1007e030();

	return bOk;
}
