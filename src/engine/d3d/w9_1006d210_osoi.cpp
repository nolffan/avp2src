// *********************************************************************** //
//
//  w9_1006d210_osoi.cpp  --  d3d.ren 0x1006d210 and 0x1006d550, the dynamic
//  light-map descriptor's two members (/Os).
//
//  Written for the d3d link from the shipped bytes.  Both are
//  callee-pop __thiscall members of the 0x1c-byte descriptor that
//  big0ae20.cpp (0x1000ae20) and big3dd50.cpp (0x1003dd50) build on their
//  stack:
//
//    0x1006d550  (466 B, `ret $0x10`)  pick the smallest of five square
//                scratch sizes that holds the map, clear or seed the
//                scratch surface, then Lock it and hand back the bits;
//    0x1006d210  (404 B, `ret $0x4`)   Unlock it, and when a map was built
//                Load() it into the size class's texture, move that
//                texture to the tail of the class list and bind it.
//
//  The two callers spell the classes CLightDesc0ae20/Surf0ae20 and
//  CLightDesc3dd50/Surf3dd50.  They are defined here under the 0ae20
//  spelling; the 3dd50 spelling is aliased onto it in
//  link/aliases.rsp (the same address, 0x1006d550 at 0x1000b22d and
//  0x1003e14e; 0x1006d210 at 0x1000b255 and 0x1003e176).
//
//  FLAGS: /Os /Oi-.  Both functions are followed by single-byte 0x90
//  padding (0x1006d395..0x1006d39f, 0x1006d722..0x1006d72f), not the
//  multi-byte nops the /O2 files end with; /O2 was measured on 0x1006d550
//  and is 624 bytes against 466.  0x1006d550 CALLS memset (0x100809a0,
//  `push ; push $0x64 ; push $0 ; push ; call` at 0x1006d638 and
//  0x1006d690), which plain /Os expands to `rep stos`, so intrinsics are
//  off.  Both functions are byte-identical at this setting.
//
//  THREE SPELLINGS THE BYTES PINNED (each measured, the alternative
//  differing):
//    * the zeroed RECT corners are written `top = 0; left = 0;` -- icl
//      emits two zero stores in reverse, so `left` then `top` comes out
//      +0x04 first where the shipped code stores +0x00 first (0x1006d21c,
//      0x1006d6ae).  A braced initialiser rebuilds the whole block;
//    * the size-class search reads the table once through a pointer temp
//      and once by subscript.  Written with two subscripts icl CSEs the
//      load and unrolls the loop by two (517 bytes); the pointer temp
//      suppresses both, giving the shipped two memory-operand compares
//      and the single rolled loop;
//    * the BltFast rect is `right = left + w`, `bottom = top + h` off the
//      rect's own fields: the byte fields are loaded once each.
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"
#include <string.h>


// ---------------------------------------------------------------------- //
//  DirectDraw 7 / Direct3D 7 -- only the slots these two call.
// ---------------------------------------------------------------------- //
struct DDSurf6d210;

struct DDRect6d210
{
	int32	left;                                       // +0x00
	int32	top;                                        // +0x04
	int32	right;                                      // +0x08
	int32	bottom;                                     // +0x0c
};

// DDBLTFX, 0x64 bytes: dwSize at +0x00, dwFillColor at +0x50.
struct DDBltFx6d210
{
	uint32	dwSize;                                     // +0x00
	uint8	m_pad_04[0x50 - 0x04];
	uint32	dwFillColor;                                // +0x50
	uint8	m_pad_54[0x64 - 0x54];
};

// DDSURFACEDESC2, 0x7c bytes: dwSize +0x00, lPitch +0x10, lpSurface +0x24.
struct DDSurfDesc6d210
{
	uint32	dwSize;                                     // +0x00
	uint8	m_pad_04[0x10 - 0x04];
	int32	lPitch;                                     // +0x10
	uint8	m_pad_14[0x24 - 0x14];
	void*	lpSurface;                                  // +0x24
	uint8	m_pad_28[0x7c - 0x28];
};

struct DDSurf6d210Vtbl
{
	uint8	m_pad_00[0x14];
	// +0x14 Blt
	long (__stdcall* Blt)(DDSurf6d210*, DDRect6d210*, DDSurf6d210*,
	                      DDRect6d210*, uint32, DDBltFx6d210*);
	uint8	m_pad_18[0x1c - 0x18];
	// +0x1c BltFast
	long (__stdcall* BltFast)(DDSurf6d210*, uint32, uint32, DDSurf6d210*,
	                          DDRect6d210*, uint32);
	uint8	m_pad_20[0x64 - 0x20];
	// +0x64 Lock
	long (__stdcall* Lock)(DDSurf6d210*, DDRect6d210*, DDSurfDesc6d210*,
	                       uint32, void*);
	uint8	m_pad_68[0x80 - 0x68];
	// +0x80 Unlock
	long (__stdcall* Unlock)(DDSurf6d210*, DDRect6d210*);
};

struct DDSurf6d210
{
	DDSurf6d210Vtbl*	lpVtbl;
};

struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8	m_pad_00[0x8c];
	// +0x8c SetTexture
	long (__stdcall* SetTexture)(D3DDevice*, uint32, DDSurf6d210*);
	uint8	m_pad_90[0xac - 0x90];
	// +0xac Load
	long (__stdcall* Load)(D3DDevice*, DDSurf6d210*, void*, DDSurf6d210*,
	                       DDRect6d210*, uint32);
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};


// ---------------------------------------------------------------------- //
//  The texture record, the same record big6d910.cpp calls Tex6d910:
//  surface at +0x0c, frame stamp at +0x14, list link at +0x1c.
// ---------------------------------------------------------------------- //
struct Tex6d910
{
	uint8		m_pad_00[0x0c];
	DDSurf6d210* m_pSurface0c;                          // +0x0c
	uint8		m_pad_10[0x14 - 0x10];
	uint16		m_nStamp14;                             // +0x14
	uint8		m_pad_16[0x1c - 0x16];
	LTLink		m_Link1c;                               // +0x1c
};

// The scratch record of a size class (0x10091954[i]): only its surface.
struct Scratch6d210
{
	uint8		m_pad_00[0x0c];
	DDSurf6d210* m_pSurface0c;                          // +0x0c
};

// The caller's surface: the source surface record at +0x44 (whose +0x1c
// is the DirectDraw surface), and the default map size / blit origin
// bytes at +0x48..+0x4b.
struct Src6d210
{
	uint8		m_pad_00[0x1c];
	DDSurf6d210* m_pSurface1c;                          // +0x1c
};

struct Surf0ae20
{
	uint8		m_pad_00[0x44];
	Src6d210*	m_pSrc44;                               // +0x44
	uint8		m_b48;                                  // +0x48
	uint8		m_b49;                                  // +0x49
	uint8		m_b4a;                                  // +0x4a
	uint8		m_b4b;                                  // +0x4b
};


// ---------------------------------------------------------------------- //
//  THE DESCRIPTOR, 0x1c bytes.
// ---------------------------------------------------------------------- //
class CLightDesc0ae20
{
public:
	int32	sub_1006d550(Surf0ae20* pSurf, uint32 bFill, uint32 nWidth,
	                     uint32 nHeight);               // 0x1006d550
	int32	sub_1006d210(void* pMap);                   // 0x1006d210

	void*			m_pBits00;                          // +0x00
	int32			m_nPitch04;                         // +0x04
	uint32			m_nClass08;                         // +0x08
	uint32			m_nWidth0c;                         // +0x0c
	uint32			m_nHeight10;                        // +0x10
	LTLink*			m_pList14;                          // +0x14
	Scratch6d210*	m_pScratch18;                       // +0x18
};


// ---------------------------------------------------------------------- //
//  Externals.
// ---------------------------------------------------------------------- //
extern D3DDevice*		g_pD3DDevice;                   // 0x1008d988
extern int32			g_CV_LightMap;                  // 0x10089400
extern uint32			g_10089820[5];                  // 0x10089820
extern uint16			g_1008bea8;                     // 0x1008bea8
extern uint32			g_1008d7cc;                     // 0x1008d7cc
extern uint32			g_1008d974;                     // 0x1008d974  NOT YET in d3ddata.cpp
// The per-stage inverse texture size pair (big0ae20.cpp's spelling).
struct TexScale0ae20
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};

extern TexScale0ae20	g_a1008e664[];                  // 0x1008e664
extern Tex6d910*		g_a1008e684[];                   // 0x1008e684
// 0x10091954: the scratch record per size class, spelled `void*` as
// d3dr3a.cpp spells it.
extern void*			g_10091954[5];                  // 0x10091954
extern LTLink			g_10091968[5];                  // 0x10091968


// ======================================================================= //
//  0x1006d550 -- 466 bytes.
// ======================================================================= //
int32 CLightDesc0ae20::sub_1006d550(Surf0ae20* pSurf, uint32 bFill,
                                    uint32 nWidth, uint32 nHeight)
{
	DDBltFx6d210	fx;
	DDSurfDesc6d210	ddsd;
	DDRect6d210		rDst;
	DDRect6d210		rLock;
	DDSurf6d210*	pDD;
	Src6d210*		pSrc;
	uint32			i;
	long			hr;

	m_nWidth0c  = (nWidth > 0) ? nWidth : pSurf->m_b48;
	m_nHeight10 = (nHeight > 0) ? nHeight : pSurf->m_b49;
	m_pList14 = LTNULL;

	for (i = 0; i < 5; i++)
	{
		uint32* pSize = &g_10089820[i];

		if (m_nWidth0c <= *pSize && m_nHeight10 <= g_10089820[i])
		{
			m_nClass08 = i;
			m_pList14 = &g_10091968[i];
			break;
		}
	}

	if (!m_pList14 || m_pList14 == m_pList14->m_pNext)
		return 0;

	m_pScratch18 = (Scratch6d210*)g_10091954[m_nClass08];
	pDD = m_pScratch18->m_pSurface0c;

	if (bFill)
	{
		pSrc = pSurf->m_pSrc44;

		if (pSrc && g_CV_LightMap)
		{
			rDst.left   = pSurf->m_b4a;
			rDst.top    = pSurf->m_b4b;
			rDst.right  = rDst.left + m_nWidth0c;
			rDst.bottom = rDst.top + m_nHeight10;
			hr = pDD->lpVtbl->BltFast(pDD, 0, 0, pSrc->m_pSurface1c,
			                          &rDst, 0x10);
		}
		else
		{
			memset(&fx, 0, sizeof(fx));
			fx.dwFillColor = 0;
			fx.dwSize = sizeof(fx);
			hr = pDD->lpVtbl->Blt(pDD, LTNULL, LTNULL, LTNULL, 0x400, &fx);
		}

		if (hr)
			return 0;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	rLock.top    = 0;
	rLock.left   = 0;
	rLock.right  = m_nWidth0c;
	rLock.bottom = m_nHeight10;

	if (pDD->lpVtbl->Lock(pDD, &rLock, &ddsd, 0x801, LTNULL))
		return 0;

	m_nPitch04 = ddsd.lPitch;
	m_pBits00 = ddsd.lpSurface;
	return 1;
}


// ======================================================================= //
//  0x1006d210 -- 404 bytes.
// ======================================================================= //
int32 CLightDesc0ae20::sub_1006d210(void* pMap)
{
	DDRect6d210		r;
	Tex6d910*		pTex;
	long			hr;
	float			fInv;

	r.top    = 0;
	r.left   = 0;
	r.right  = m_nWidth0c;
	r.bottom = m_nHeight10;
	m_pScratch18->m_pSurface0c->lpVtbl->Unlock(m_pScratch18->m_pSurface0c, &r);

	if (!pMap)
		return 1;

	pTex = (Tex6d910*)m_pList14->m_pNext->m_pData;

	if (pTex->m_nStamp14 != g_1008bea8)
		pTex->m_nStamp14 = g_1008bea8;

	if (!g_1008d974)
		hr = g_pD3DDevice->lpVtbl->Load(g_pD3DDevice, pTex->m_pSurface0c,
		                                LTNULL, m_pScratch18->m_pSurface0c,
		                                &r, 0);
	else
		hr = g_pD3DDevice->lpVtbl->Load(g_pD3DDevice, pTex->m_pSurface0c,
		                                LTNULL, m_pScratch18->m_pSurface0c,
		                                LTNULL, 0);

	if (hr)
		return 0;

	dl_Remove(&pTex->m_Link1c);
	dl_Insert(m_pList14->m_pPrev, &pTex->m_Link1c);

	g_a1008e684[g_1008d7cc] = pTex;
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, g_1008d7cc,
	                                 pTex->m_pSurface0c);

	fInv = 1.0f / (float)g_10089820[m_nClass08];
	g_a1008e664[g_1008d7cc].m_fInvW00 = fInv;
	g_a1008e664[g_1008d7cc].m_fInvH04 = fInv;
	return 1;
}
