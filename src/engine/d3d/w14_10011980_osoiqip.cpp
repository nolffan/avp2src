// *********************************************************************** //
//
//  w14_10011980_osoiqip.cpp -- sub_10011980 (written for the d3d.ren link),
//  and 0x10011450, its /Qip callee (see there).  2,557 bytes, 0x10011980..0x1001237d, padded with 0x90 to
//  0x10012380.
//
//  Draw one bucket of the deferred second pass (0x10011620, in this file, the
//  only caller: 0x1001181b, 0x1001183e, 0x10011901): the bucket's element
//  chain, with the bucket's texture block.  Three ways, tried in order:
//
//    A. detail textures on (0x1008d950, 0x1008ae00) and the block has both
//       a detail object (+0x2c) and a second block (+0x28): the second
//       block in stage 0, the block itself in stage 1, then 0x10011450 on
//       the detail object and the batched draw 0x1000f6f0 between
//       0x10011450's setup and 0x10011500's restore;
//    B. otherwise the block in stage 0; with 0x1008aea0 on, a bound
//       stage-0 texture, a second block and NO detail object, the second
//       block goes in stage 1 and the draw runs between 0x10011560 and
//       0x100115b0, the texture's own scale/offset (its slot +0x04
//       answering 1) published through 0x1008add4..0x1008addc;
//    C. anything that could not be bound: each element is drawn straight
//       from the screen-vertex pool 0x1008d524 as a triangle fan
//       (DrawPrimitive, FVF 0x1c4), its uv scaled by stage 0's 1/w, 1/h
//       and its colour forced white unless nMode is set, and with bFlag
//       the element goes back on the free list 0x1008d4fc.
//
//  Binding a block to a stage is the texture-record bind every draw
//  routine inlines (big0ae20.cpp, big47230.cpp, w3_1000a7d0_os.cpp): find
//  or make (0x10039bd0) the block's record for the stage, bind it unless
//  it already is -- move it to the head of the used list, count its bytes,
//  SetTexture, alpha ref, publish its 1/w 1/h -- and drop any LOD bias.
//
//  /Os: the pad after the body is 0x90.  /Os /Oi- /Qip
//  (was `_os`): 0x10011450 is identical only as a /Qip pair with its
//  caller here; 0x10011980 itself is far off either way (1944 bytes
//  differ as /Os, 1949 as /Os /Oi- /Qip).
//
//  0x10011980 has only a register entry (bucket in %eax, block in %edx,
//  bFlag in %ecx, nMode in a stack slot): a static of the shipped TU with
//  its only caller 0x10011620, which moved in here from
//  w2_10011620_os.cpp and is identical here -- its four calls were the
//  whole of its residual (268 bytes as a lone /Os file).  0x10011980 was
//  still far (1920 bytes differ; 1949 before; near now, see
//  its banner), and it calls 0x1000f6f0's
//  register entry, which is still an ordinary __cdecl call here.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct DDSurface;
struct DDSurfaceVtbl
{
	uint8   m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface*, uint32 nLOD);               // +0xbc
};
struct DDSurface
{
	DDSurfaceVtbl* lpVtbl;
};

struct D3DDevice;
struct D3DDeviceVtbl
{
	uint8   m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32 nState,
	                                 uint32 nValue);                  // +0x50
	long (__stdcall* GetRenderState)(D3DDevice*, uint32 nState,
	                                 uint32* pValue);                 // +0x54
	uint8   m_pad_58[0x64 - 0x58];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32 nPrimType,
	                                uint32 nFVF, void* pVerts,
	                                uint32 nVerts, uint32 nFlags);    // +0x64
	uint8   m_pad_68[0x8c - 0x68];
	long (__stdcall* SetTexture)(D3DDevice*, uint32 nStage,
	                             DDSurface* pTex);                    // +0x8c
	uint8   m_pad_90[0x94 - 0x90];
	long (__stdcall* SetTextureStageState)(D3DDevice*, uint32, uint32,
	                                       uint32);                   // +0x94
};
struct D3DDevice
{
	D3DDeviceVtbl*  lpVtbl;
};

// ltlink.h's LTLink, as big0ae20.cpp spells the texture list.
struct Link0ae20
{
	Link0ae20*  m_pPrev;                                    // +0x00
	Link0ae20*  m_pNext;                                    // +0x04
	void*       m_pData;                                    // +0x08

	inline void Remove()
	{
		m_pPrev->m_pNext = m_pNext;
		m_pNext->m_pPrev = m_pPrev;
		TieOff();
	}

	inline void TieOff()
	{
		m_pPrev = m_pNext = this;
	}

	inline void AddAfter(Link0ae20* pLink)
	{
		pLink->m_pPrev = this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
	}
};

inline void dl_Insert(Link0ae20* pAfter, Link0ae20* pLink)
{
	pAfter->AddAfter(pLink);
}

inline void dl_Remove(Link0ae20* pLink)
{
	pLink->Remove();
}

// The texture's scale and offset block (+0x18 of a record).
struct TexXform11980
{
	uint8       m_pad_00[0x34];
	float       m_fScale34;                                 // +0x34
	float       m_fOffset38;                                // +0x38
	float       m_fOffset3c;                                // +0x3c
};

// The per-stage texture record (big0ae20.cpp's Tex0ae20 layout).
struct TexBase0ae20
{
public:
	virtual void    Slot00();                               // +0x00
	virtual int     Slot04();                               // +0x04

	float       m_fInvW04;                                  // +0x04
	float       m_fInvH08;                                  // +0x08
	DDSurface*  m_pSurface0c;                               // +0x0c
	uint32      m_nBytes10;                                 // +0x10
	uint16      m_nStamp14;                                 // +0x14
	uint16      m_nAlphaRef16;                              // +0x16
	TexXform11980* m_pXform18;                              // +0x18
	Link0ae20   m_Link1c;                                   // +0x1c
	uint8       m_pad_28[0x30 - 0x28];                      // +0x28
};

struct Tex0ae20 : public TexBase0ae20
{
	Tex0ae20*   m_pNext30;                                  // +0x30
	uint8       m_pad_34[0x42 - 0x34];                      // +0x34
	uint16      m_nStage42;                                 // +0x42
	uint16      m_nLOD44;                                   // +0x44
};

// The bucket's texture block (w2_10011620_os.cpp's name).
struct Bucket729c0;

struct TexBlock729c0
{
	uint8           m_pad_00[0x0c];                         // +0x00
	Tex0ae20*       m_pRecs0c;                              // +0x0c
	uint8           m_pad_10[0x1c - 0x10];                  // +0x10
	Bucket729c0*    m_pBucket1c;                            // +0x1c
	uint8           m_pad_20[0x28 - 0x20];                  // +0x20
	TexBlock729c0*  m_pBlock28;                             // +0x28
	void*           m_pDetail2c;                            // +0x2c
	uint16          m_nStamp30;                             // +0x30
};

// One element of a bucket's chain.
struct Elem729c0
{
	Elem729c0*      m_pFreeNext;                            // +0x00
	uint32          m_iVert04;                              // +0x04
	uint32          m_nVerts08;                             // +0x08
	uint8           m_pad_0c[0x14 - 0x0c];                  // +0x0c
	Elem729c0*      m_pNext14;                              // +0x14
};

struct Bucket729c0
{
	TexBlock729c0*  m_pTexture;                             // +0x00
	Elem729c0*      m_pElements;                            // +0x04
	Bucket729c0*    m_pNextInList;                          // +0x08
};

// A screen vertex of the pool (FVF 0x1c4, 0x20 bytes).
struct PoolVert11980
{
	uint8       m_pad_00[0x10];                             // +0x00
	uint32      m_nColor10;                                 // +0x10
	uint8       m_pad_14[0x18 - 0x14];                      // +0x14
	float       m_fU18;                                     // +0x18
	float       m_fV1c;                                     // +0x1c
};

struct TexScale0ae20
{
	float       m_fInvW00;                                  // +0x00
	float       m_fInvH04;                                  // +0x04
};

struct Stats0ae20
{
	uint8       m_pad_00[0x48];
	uint32      m_nBytes48;                                 // +0x48
};

extern D3DDevice*       g_pD3DDevice;                   // 0x1008d988
extern Tex0ae20*        g_a1008e684[];                  // 0x1008e684  bound per stage
extern TexScale0ae20    g_a1008e664[];                  // 0x1008e664  1/w, 1/h per stage
extern Link0ae20        g_Link1008e634;                 // 0x1008e634  the used list head
extern uint16           g_w1008bea8;                    // 0x1008bea8  frame stamp
extern uint16           g_w1008b870;                    // 0x1008b870  block stamp
extern Stats0ae20*      g_p1008cbcc;                    // 0x1008cbcc
extern uint32           g_n1008bd80;                    // 0x1008bd80  texture changes
extern uint32           g_1008d950;                     // 0x1008d950
extern uint32           g_1008ae00;                     // 0x1008ae00
extern uint32           g_1008aea0;                     // 0x1008aea0
extern uint8*           g_1008d524;                     // 0x1008d524  screen-vertex pool
extern Elem729c0*       g_pFreeElems1008d4fc;           // 0x1008d4fc
extern float            g_1008fe3c;                     // 0x1008fe3c
extern float            g_1008add4;                     // 0x1008add4
extern float            g_1008add8;                     // 0x1008add8
extern float            g_1008addc;                     // 0x1008addc

// 0x10039bd0 -- make a block's record for a stage (big0ae20.cpp's
// spelling, extern "C"); the third argument says whether the block already
// has records.
extern "C" Tex0ae20* sub_10039bd0(TexBlock729c0* pBlock, uint32 nStage,
                                  uint32 bHaveList);
struct D3DDevTSSR5C;
struct D3DDevTSSR5CVtbl
{
	void*	m_Slot00[36];                                       // +0x00..+0x8c
	long	(__stdcall* GetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32* pdwValue);  // +0x90
	long	(__stdcall* SetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32 dwValue);    // +0x94
};
struct D3DDevTSSR5C
{
	D3DDevTSSR5CVtbl*	lpVtbl;
};

extern D3DDevTSSR5C*	g_1008d988;                     // 0x1008d988
extern uint32			g_1008ae80_flag;                // 0x1008ae80

// ---------------------------------------------------------------------- //
//  0x10011450 (stack entry) / 0x10011454 (register entry) -- 162 bytes.
//  SET STAGE 1'S COLOUR OP FOR ONE OF TWO DETAIL-TEXTURE MODES.
//
//  WHAT PINS IT:
//    * `cmp $0x1,%eax` then `cmp $0x2,%eax` with nothing in between and no
//      jump table: a two-case comparison chain on the argument, and the
//      default does nothing.
//    * 0x1008ae80 is the console variable "DetailTextureAdd" registered by
//      0x10014600 above.  Its value picks 8 over 4 as stage 1's COLOROP,
//      and 4 / 8 are D3DTOP_MODULATE / D3DTOP_ADDSIGNED.  The variable is
//      tested with `test %eax,%eax` on the dword at 0x1008ae80 -- the
//      NODE's first field, not the float at +0x08, so the flag is the
//      node's own +0x00.
//    * `mov 0x1008d988,%eax ; mov (%eax),%eax ; call *0x94(%eax)` is the
//      device loaded from a global POINTER, so `g_pDev->lpVtbl->...`.
//    * the second and third calls plant their constants as
//      `movl $imm,N(%esp)` while the first shares `%ecx = 1` for both the
//      stage and the state, which is icl reusing a materialised 1.
// ---------------------------------------------------------------------- //
//
//  IDENTICAL here and not in d3dr5c.cpp: the shipped
//  function is an external /Qip pair -- `mov 0x4(%esp),%eax` at 0x10011450
//  falling into the register entry 0x10011454, which 0x10011980 below
//  calls (0x10011cdb).  icl emits that pair only in a /Qip TU that also
//  holds a caller, so it moved into this file, now /Os /Oi- /Qip
//  (`_osoiqip`).  The device is read into a local ahead of the ?: so its
//  load is hoisted above the flag test, as shipped (without it the
//  register entry comes out one byte longer, device and value swapped
//  between %eax and %edx).
void sub_10011450(unsigned long nMode)
{
	if (nMode == 1)
	{
		D3DDevTSSR5C* pDev = g_1008d988;

		pDev->lpVtbl->SetTextureStageState(pDev, 1, 1,
			g_1008ae80_flag ? 8 : 4);

		g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 4, 2);
	}
	else if (nMode == 2)
	{
		g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 1, 0x12);
	}
}

void sub_10011500();
void sub_10011560();
void sub_100115b0();
// ======================================================================= //
//  0x10011620 -- draw the deferred lightmap second pass (from
//  w2_10011620_os.cpp; moved here).
//
//  Every bucket queued on 0x1008d4c4 (big729c0.cpp's Bucket729c0: texture
//  block, element chain, next) is drawn through 0x10011980 with stage 1
//  modulating the current colour by its texture and the fog colour set
//  from the three FogR/G/B variables (0x10023930, as bigb910.cpp installs
//  it).  Buckets whose lightmap is resident (0x10039f50) are drawn in a
//  second, alpha-blended round after the others; with LightmapsOnly
//  (0x1008ca9c) the element chains go straight to 0x100256a0 instead.
//  Each bucket is returned to the free list at 0x1008d4e0 after clearing
//  its texture block's +0x1c back-pointer, and the list and the
//  screen-vertex pool are emptied.
//
//  The FOGCOLOR save/restore is d3dbatcha.cpp's C1000adc0 with its
//  constructor inlined here (Get then Set, no call to 0x1000adc0) and its
//  restore at the single exit.  An element's first dword is its owner
//  object here (this file's Elem729c0 calls it m_pFreeNext, its meaning
//  once the element is back on the free list).
// ======================================================================= //

struct Holder729c0
{
	uint8           m_pad_00[0x30];
	TexBlock729c0*  m_pBlock30;                         // +0x30
};

struct Obj729c0
{
	uint8           m_pad_00[0x28];
	Holder729c0*    m_p28;                              // +0x28
};

extern Bucket729c0*     g_pBucketList1008d4c4;          // 0x1008d4c4
extern Bucket729c0*     g_pFreeBuckets1008d4e0;         // 0x1008d4e0

extern uint32           g_1008add0;                     // 0x1008add0  d3dneutral.cpp
extern int32            g_CV_LightmapsOnly;             // 0x1008ca9c
extern int32            g_CV_Saturate;                  // 0x1008cb98
extern uint32           g_1008caf4;                     // 0x1008caf4  fog colour
extern uint32           g_1008caf8;                     // 0x1008caf8
extern uint32           g_1008cafc;                     // 0x1008cafc
extern unsigned long    g_1008d520;                     // 0x1008d520  pool used

// LMFullBright (d3dr5c.cpp's CVarR5C): only its value at +0x00.
struct CVarR5C
{
	int32           m_nValue00;                         // +0x00
};
extern CVarR5C          g_1008ae20;                     // 0x1008ae20

extern "C" uint32 sub_10039f50(TexBlock729c0* pBlock, uint32 nStage);
extern "C" void   sub_100256a0(Elem729c0* pChain);
extern "C" uint32 sub_10023930(uint32 a, uint32 b, uint32 c);

static void sub_10011980(Bucket729c0* pBucket, TexBlock729c0* pBlock, uint32 bFlag,
                  uint32 nMode);


// d3dbatcha.cpp's render-state saver, both halves inline here.
class CStateSave11620
{
public:
	inline CStateSave11620(uint32 nState, uint32 nValue)
	{
		m_nState00 = nState;
		g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, nState, &m_nOld04);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState00, nValue);
	}

	inline ~CStateSave11620()
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState00, m_nOld04);
	}

	uint32 m_nState00;                                  // +0x00
	uint32 m_nOld04;                                    // +0x04
};


void sub_10011620()
{
	uint32          dwOldAlpha;
	Bucket729c0*    pDeferred;
	Bucket729c0*    pBucket;
	Bucket729c0*    pNext;
	TexBlock729c0*  pBlock;

	// Stage 1: COLORARG1 = CURRENT, COLORARG2 = TEXTURE.
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 2, 2);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 3, 1);

	if (!g_1008add0 && !g_1008ae20.m_nValue00)
	{
		g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, 0x1b, &dwOldAlpha);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x1b, 1);

		// SRCBLEND: DESTCOLOR when saturating, else ZERO; DESTBLEND:
		// SRCCOLOR.
		if (g_CV_Saturate)
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x13, 9);
		else
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x13, 1);

		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x14, 3);
	}

	CStateSave11620 fogSave(0x22, sub_10023930(g_1008caf4, g_1008caf8, g_1008cafc));

	pDeferred = LTNULL;

	for (pBucket = g_pBucketList1008d4c4; pBucket; pBucket = pNext)
	{
		pNext = pBucket->m_pNextInList;

		if (g_CV_LightmapsOnly)
		{
			sub_100256a0(pBucket->m_pElements);
		}
		else if (pBucket->m_pElements)
		{
			pBlock = (*(Obj729c0**)pBucket->m_pElements)->m_p28->m_pBlock30;

			if (sub_10039f50(pBlock, 0) && !g_1008add0 && !g_1008ae20.m_nValue00)
			{
				if (g_CV_Saturate)
					sub_10011980(pBucket, pBlock, 0, 0);

				pBucket->m_pNextInList = pDeferred;
				pDeferred = pBucket;
				continue;
			}

			sub_10011980(pBucket, pBlock, 1, 0);
		}

		pBucket->m_pTexture->m_pBucket1c = LTNULL;
		*(Bucket729c0**)pBucket = g_pFreeBuckets1008d4e0;
		g_pFreeBuckets1008d4e0 = pBucket;
	}

	if (!g_1008add0 && !g_1008ae20.m_nValue00)
	{
		// SRCBLEND: SRCALPHA; DESTBLEND: INVSRCALPHA when saturating.
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x13, 5);

		if (g_CV_Saturate)
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x14, 6);

		for (pBucket = pDeferred; pBucket; pBucket = pNext)
		{
			pNext = pBucket->m_pNextInList;

			sub_10011980(pBucket,
			             (*(Obj729c0**)pBucket->m_pElements)->m_p28->m_pBlock30,
			             1, g_CV_Saturate);

			pBucket->m_pTexture->m_pBucket1c = LTNULL;
			*(Bucket729c0**)pBucket = g_pFreeBuckets1008d4e0;
			g_pFreeBuckets1008d4e0 = pBucket;
		}

		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x1b, dwOldAlpha);
	}

	g_1008d520 = 0;
	g_pBucketList1008d4c4 = LTNULL;
}


// ======================================================================= //
//  0x1000f6f0 -- draw a bucket.  Moved here from big0f6f0.cpp:
//  it is a /Qip register-only static of this TU -- its two callers,
//  0x10011cec and 0x1001206c inside 0x10011980, enter it with the
//  arguments in registers -- so it lives beside them as a static.
// ======================================================================= //


// ======================================================================= //
//  THE VECTOR AND THE PLANE
// ======================================================================= //
struct LTVec
{
	float x;                                             // +0x00
	float y;                                             // +0x04
	float z;                                             // +0x08
};

// Only ever handed to sub_1001e950 here, never dereferenced by this
// function; big64cc0.cpp decoded 0x10048150 over the same object as
// `n.x*v.x + n.y*v.y + n.z*v.z - d`.
struct SPlane
{
	LTVec m_vNormal;                                     // +0x00
	float m_fDist;                                       // +0x0c
};


// ======================================================================= //
//  THE DRAW VERTEX -- 40 bytes, D3DFVF 0x2c4.  big64cc0.cpp's RVert; every
//  offset below is a load or a store in THESE bytes:
//
//    +0x00..+0x08  the position (three integer `mov`s at 0x1000f813/
//                  0x1000f821/0x1000f82f, interpolated by all six clip
//                  blocks, overwritten by the unclipped transform)
//    +0x0c         written only by the unclipped transform (0x1000fa5c)
//                  and by sub_1000d940
//    +0x10..+0x13  b, g, r, a -- set as one dword to 0xffffffff at
//                  0x1000f832 and 0x1001126f, interpolated byte by byte at
//                  0x1000fd5c/0x1000fd0e/0x1000fcc0/0x1000fdaa
//    +0x14..+0x17  the specular dword g_pfn1008ce8c fills (its second
//                  argument is %edx = %ebx+0x14 at 0x1000f803/0x1000f847);
//                  only its top byte +0x17 is interpolated (0x1000fdf9)
//    +0x18/+0x1c   texture pair ONE -- sub_1001e950 writes it through the
//                  two pointers handed in at 0x1000f88f/0x1000f893, then
//                  0x1000f89c scales it by g_1008add4
//    +0x20/+0x24   texture pair TWO -- the source pair scaled at
//                  0x1000f849/0x1000f85b, or the rotated pair at
//                  0x100112a9/0x100112c3
// ======================================================================= //
struct RVert
{
	LTVec  m_vPos;                                       // +0x00
	float  m_fRHW;                                       // +0x0c
	uint8  m_b;                                          // +0x10
	uint8  m_g;                                          // +0x11
	uint8  m_r;                                          // +0x12
	uint8  m_a;                                          // +0x13
	uint8  m_sb;                                         // +0x14
	uint8  m_sg;                                         // +0x15
	uint8  m_sr;                                         // +0x16
	uint8  m_sa;                                         // +0x17
	float  m_fU;                                         // +0x18
	float  m_fV;                                         // +0x1c
	float  m_fU2;                                        // +0x20
	float  m_fV2;                                        // +0x24
};


// ======================================================================= //
//  THE POOLED VERTEX -- 32 bytes, the shared screen-vertex pool the
//  lightmap pass reserved out of.  `shl $0x5,%ebx ; add 0x1008d524,%ebx`
//  at 0x1001122e/0x10011237 is the indexing, `rep movsl $8` at 0x1001125b
//  the copy and `add $0x20,%ebx` at 0x100112c0 the stride.  d3dr3c.cpp
//  calls the same layout ClipVert.
// ======================================================================= //
struct PoolVert
{
	LTVec  m_vPos;                                       // +0x00
	float  m_fRHW;                                       // +0x0c
	uint8  m_b, m_g, m_r, m_a;                           // +0x10 .. +0x13
	uint8  m_sb, m_sg, m_sr, m_sa;                       // +0x14 .. +0x17
	float  m_fU;                                         // +0x18
	float  m_fV;                                         // +0x1c
};


// ======================================================================= //
//  THE SOURCE VERTEX -- 24 bytes.  Only three fields are read: the
//  position pointer at +0x00 (dereferenced for three floats at
//  0x1000f813/0x1000f821/0x1000f82f and handed to sub_1001e950 at
//  0x1000f885) and the texture pair at +0x04/+0x08.  big64cc0.cpp and
//  big2ef34.cpp both measured the same 0x18 stride.
// ======================================================================= //
struct SrcVert
{
	LTVec* m_pPos;                                       // +0x00
	float  m_fU;                                         // +0x04
	float  m_fV;                                         // +0x08
	uint8  m_pad_0c[0x0c];                               // +0x0c .. +0x17
};


// ======================================================================= //
//  THE OBJECT AND ITS SURFACE.  pObj->m_p28->m_p30->m_p2c is the env-map
//  marker (0x1000f79e..0x1000f7ad) and pObj->m_p28->m_p34 the plane handed
//  to the reflection helper (0x1000f86b/0x1000f873).  Same two offsets
//  big64cc0.cpp reads off its SSurface and big124a4.cpp off its Ctx124a4.
// ======================================================================= //
struct STexture
{
	uint8  m_pad_00[0x2c];                               // +0x00 .. +0x2b
	void*  m_p2c;                                        // +0x2c
};

struct SSurface
{
	uint8      m_pad_00[0x30];                           // +0x00 .. +0x2f
	STexture*  m_p30;                                    // +0x30
	SPlane*    m_p34;                                    // +0x34
};

struct SGeoObj
{
	uint8      m_pad_00[0x28];                           // +0x00 .. +0x27
	SSurface*  m_p28;                                    // +0x28
};


// ======================================================================= //
//  THE BUCKET LIST -- big124a4.cpp's Elem124a4 / Bucket124a4, whose
//  registration side (0x1000eef0) d3dr3c.cpp decoded field for field.
//  This function reads +0x00 (the owner), +0x04 and +0x08 (where in the
//  vertex pool the lightmap pass left the object's screen vertices),
//  +0x10 (the clip flags) and +0x14 (the next link), and WRITES +0x00 when
//  it pushes the element back on the free list.
// ======================================================================= //
struct Elem0f6f0                                         // 0x18 bytes
{
	SGeoObj*    m_pOwner;                                // +0x00
	uint32      m_n04;                                   // +0x04
	uint32      m_n08;                                   // +0x08
	uint32      m_n0c;                                   // +0x0c
	uint32      m_n10;                                   // +0x10
	Elem0f6f0*  m_pNext;                                 // +0x14
};

struct Bucket0f6f0                                       // 0x0c bytes
{
	void*        m_pTable;                               // +0x00
	Elem0f6f0*   m_pElements;                            // +0x04
	Bucket0f6f0* m_pNextInList;                          // +0x08
};


// ======================================================================= //
//  THE TRANSFORM OBJECT.  Only the projective block at +0x1dc..+0x218 is
//  read by name here (the unclipped path); every other user of 0x1008b880
//  in the project takes it as an opaque `uint8[]`, so the two views are
//  kept apart and the block is reached through a cast exactly where the
//  bytes read it.  Field for field big124a4.cpp's XForm124a4.
// ======================================================================= //
struct XForm0f6f0
{
	uint8   m_pad_000[0x1dc];                            // +0x000 .. +0x1db
	float   m_f1dc;                                      // +0x1dc
	float   m_f1e0;                                      // +0x1e0
	float   m_f1e4;                                      // +0x1e4
	float   m_f1e8;                                      // +0x1e8
	float   m_f1ec;                                      // +0x1ec
	float   m_f1f0;                                      // +0x1f0
	float   m_f1f4;                                      // +0x1f4
	float   m_f1f8;                                      // +0x1f8
	float   m_f1fc;                                      // +0x1fc
	float   m_f200;                                      // +0x200
	float   m_f204;                                      // +0x204
	float   m_f208;                                      // +0x208
	float   m_f20c;                                      // +0x20c
	float   m_f210;                                      // +0x210
	float   m_f214;                                      // +0x214
	float   m_f218;                                      // +0x218
};


// ======================================================================= //
//  THE DIRECT3D 7 DEVICE AT 0x1008d988 -- big64cc0.cpp's slice, cut to the
//  one slot used.  +0x64 / 4 == 25 == DrawPrimitive.
// ======================================================================= //
#define D3DPT_TRIANGLEFAN                     6
// XYZRHW | DIFFUSE | SPECULAR | TEX2 -- 12+4+4+4+8+8 = 40 = 0x28.
#define D3DFVF_ENVVERTEX                  0x2c4


// ======================================================================= //
//  GLOBALS -- every one of them is an address this function touches.
// ======================================================================= //


// The clipper's state, all named already by d3dr3a / d3dr3c / big16324 /
// big64cc0.
extern uint32      g_1008b874;                           // clip flags
extern RVert*      g_1008b87c;                           // scratch cursor
extern uint32      g_1008be44;                           // clipped-poly count
extern float       g_1008b908;                           // near plane
extern float       g_1008b910;                           // far plane
extern uint8       g_1008b880[];                         // the transform object
extern uint32      g_1008d7a0;                           // UseD3DClip

// 0x1008ae00 -- big124a4.cpp / d3dr5c.cpp: the EnvMapWorld console
// variable, read as its leading dword.
extern uint32      g_1008ae00;                           // EnvMapWorld

// 0x1008bd34 -- the eye point.  big16324.cpp calls it g_v1008bd34; here it
// is only ever the first argument of sub_1001e950 (0x1000f86e).
extern LTVec       g_v1008bd34;                          // 0x1008bd34

// 0x1008d524 -- the base of the shared 32-byte screen-vertex pool.
// d3dr2c.cpp records 0x1000f270 handing out `g_1008d524 +
// (g_1008d520 << 5)`; the read at 0x10011237 is the other end of that.
#define g_1008d524_Pool   ((PoolVert*)g_1008d524)              // 0x1008d524

// 0x1008d4fc -- big124a4.cpp: the bucket-element free list, popped there
// through its +0x00 and pushed here through the same field.
extern Elem0f6f0*  g_1008d4fc;                           // 0x1008d4fc

// 0x1008e664 / 0x1008e668 -- ELEMENT ZERO of the per-stage inverse
// texture-size pair big6d910.cpp and big5a960.cpp both publish at
// 0x1008e664 (`0x1008e664(,%eax,8)`).  0x1008e66c / 0x1008e670 are
// element ONE of the same table, which is why the reflection pass scales
// the SECOND stage's coordinates with them.
extern float       g_1008e664;                           // stage 0 1/w
extern float       g_1008e668;                           // stage 0 1/h
extern float       g_1008e66c;                           // stage 1 1/w
extern float       g_1008e670;                           // stage 1 1/h

// 0x1008add4 / 0x1008add8 / 0x1008addc -- written by BOTH callers in the
// instructions immediately before the call (0x10011cd4, 0x10012022,
// 0x10012034..0x10012040, and the identity case 0x10012047/0x10012051
// which stores 1.0f and 0.0f).  A scale and a (cos,sin) pair.
extern float       g_1008add4;                           // env scale
extern float       g_1008add8;                           // env cos
extern float       g_1008addc;                           // env sin

// 0x1008ce8c -- the register-ABI indirect call at 0x1000f863.  d3dr3d.cpp
// records the three functions assigned to it (0x100234d0 / 0x100235d0 /
// 0x100234e0); big17c14.cpp, d3dr2d.cpp and big64cc0.cpp all pin the
// signature as a two-argument __fastcall taking %ecx then %edx, applied to
// (pVert, (uint8*)pVert + 0x14).
typedef void (__fastcall* PFN_1008ce8c)(void* pA, void* pB);
extern PFN_1008ce8c g_pfn_1008ce8c;                      // 0x1008ce8c


// ======================================================================= //
//  THE CALLEES -- six distinct direct, all placed by earlier work.
// ======================================================================= //

// 0x1000a700 -- d3dbatchd.cpp: "hands back where a piece of geometry's
// data is and how many".  __cdecl (geometry, &data, &count); the call site
// at 0x1000f7db pushes FOUR words and unwinds 0x10, the fourth being icl's
// dead slot (big124a4.cpp writes the same call with three arguments).
//
// NOTE FOR big2ef34.cpp, which left "what 0x1000a700's third out-parameter
// is counted in" open: HERE IT IS COUNTED IN 24-BYTE SOURCE VERTICES.  The
// build loop at 0x1000f80c runs exactly that many times (`test ; je` at
// 0x1000f7eb, `dec ; jne` at 0x1000f8e6) and advances the data pointer by
// 0x18 each time (0x1000f8c2), and the SAME count is then handed to
// DrawPrimitive as the draw-vertex count at 0x100112f2.
extern void   sub_1000a700(SGeoObj* pGeom, SrcVert** ppOutData,
                           int32* pnOutCount);

// 0x1001e950 -- d3dbatchc.cpp: 340 bytes of hand-scheduled x87 computing a
// reflection.  __cdecl, five arguments: (eye, position, plane, &out_u,
// &out_v).  big64cc0.cpp calls it with the identical shape.
extern void   sub_1001e950(const LTVec* pEye, const LTVec* pPos,
                           const SPlane* pPlane, float* pfU, float* pfV);

// 0x1000d9b0 -- d3dlow.cpp: transform a vector by the 3x4 at +0x15c of the
// second argument.  __cdecl.
extern void   sub_1000d9b0(LTVec* pVec, uint8* pXForm);

// 0x1000d940 -- the screen projection, 1/z through the +0x31c..+0x330
// block of the second argument; the sibling of d3dlow.cpp's 0x10014580
// with the same two __cdecl arguments, and the one that writes +0x0c.
extern void   sub_1000d940(RVert* pVert, uint8* pXForm);

// 0x10011420 -- d3dbatcha.cpp: 43 bytes, three arguments plus icl's dead
// fourth (the site at 0x100111ae reserves 0x10 and fills three slots).
extern LTBOOL sub_10011420(RVert** ppVerts, int32* pnVerts, void* pXForm);


// ----------------------------------------------------------------------- //
//  0x1000f6f0 .. 0x1001141d   (7,472 bytes including the two-byte pad).
//
//  The shipped entry takes its three arguments in %eax, %edx and %ecx
//  (icl /Qip); written __cdecl here, decoded as an ordinary call.
// ----------------------------------------------------------------------- //
// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line).  Inlined, it is the shipped `fstps t ;
// flds t ; fistpl n` at each of the thirty byte lerps.
// Measured: 9871 -> 7375 bytes (shipped 7472), 9106 -> 6634
// differing at /O2 (/Os 7409 / 6440).  Still not paired: the shipped body
// is a /Qip REGISTER-ONLY static -- it opens `push %ecx ; push %edx ;
// push %eax` around __chkstk and reloads them -- of its two callers'
// TU (0x10011cec and 0x1001206c, both in 0x10011980,
// w14_10011980_osoiqip.cpp), so it has to move into that file (not done
// here).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// ======================================================================= //
//  THE CLIPPER for the 40-byte vertex -- d3dr3b.cpp's driver 0x1000aa90
//  over the six plane bodies w1_10002310.cpp / w44_10002d40.cpp compile
//  byte-identical out of line, all INLINED into 0x1000f6f0
//  (0x1000fa9b .. 0x100110ab).  Named by plane, not by address: in this
//  /Qip TU icl also emits each helper out of line (unreferenced; the link
//  drops them), and an address in the name would make the byte comparison report the
//  copies as the shipped rows.  Planes in order: 0x10002310 near,
//  0x10002670 left, 0x100029e0 top, 0x10002d40 right, 0x100030a0 bottom,
//  0x10003410 far; ClipPolyA is 0x1000aa90.
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

struct SClipVertA                                       // 0x28 bytes
{
	LTVector v;                                         // +0x00  x, y, z
	float	m_0c;                                       // +0x0c
	uint8	m_10;                                       // +0x10
	uint8	m_11;                                       // +0x11
	uint8	m_12;                                       // +0x12
	uint8	m_13;                                       // +0x13
	uint8	m_14;                                       // +0x14
	uint8	m_15;                                       // +0x15
	uint8	m_16;                                       // +0x16
	uint8	m_17;                                       // +0x17
	float	m_18;                                       // +0x18
	float	m_1c;                                       // +0x1c
	float	m_20;                                       // +0x20
	float	m_24;                                       // +0x24
};

static inline void LerpClipVertA(SClipVertA* a, SClipVertA* b, SClipVertA* dst, float t)
{
	dst->m_18 = (b->m_18 - a->m_18) * t + a->m_18;
	dst->m_1c = (b->m_1c - a->m_1c) * t + a->m_1c;
	dst->m_20 = (b->m_20 - a->m_20) * t + a->m_20;
	dst->m_24 = (b->m_24 - a->m_24) * t + a->m_24;
	dst->m_12 = (uint8)RoundF((b->m_12 - a->m_12) * t + a->m_12);
	dst->m_11 = (uint8)RoundF((b->m_11 - a->m_11) * t + a->m_11);
	dst->m_10 = (uint8)RoundF((b->m_10 - a->m_10) * t + a->m_10);
	dst->m_13 = (uint8)RoundF((b->m_13 - a->m_13) * t + a->m_13);
	dst->m_17 = (uint8)RoundF((b->m_17 - a->m_17) * t + a->m_17);
}

static inline float ClipEdgeNearA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b908;

	return t;
}

static inline float ClipEdgeLeftA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}

static inline float ClipEdgeTopA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}

static inline float ClipEdgeRightA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}

static inline float ClipEdgeBottomA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}

static inline float ClipEdgeFarA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b910;

	return t;
}

// 0x10002310
static inline int32 ClipPlaneNearA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z >= g_1008b908);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeNearA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10002670
static inline int32 ClipPlaneLeftA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.x);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeLeftA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x100029e0
static inline int32 ClipPlaneTopA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.y < pV->v.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeTopA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10002d40
static inline int32 ClipPlaneRightA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.x < pV->v.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeRightA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x100030a0
static inline int32 ClipPlaneBottomA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.y);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeBottomA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10003410
static inline int32 ClipPlaneFarA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z <= g_1008b910);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeFarA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x1000aa90 -- d3dr3b.cpp's driver for this 40-byte vertex.
static inline LTBOOL ClipPolyA(uint32 nFlags, void** ppVerts, uint32* pnVerts)
{
	void*  pScratch;
	void*  pVerts;
	uint32 nVerts;
	uint32 nOut0;
	uint32 nOut1;
	uint32 nOut2;
	uint32 nOut3;
	uint32 nOut4;
	uint32 nOut5;

	if (g_1008d7a0)
	{
		nFlags &= 0x01;
		if (!nFlags)
			return LTTRUE;
	}

	pScratch = g_1008b87c;
	pVerts   = *ppVerts;
	nVerts   = *pnVerts;

	if (nFlags & 0x01)
	{
		if (!ClipPlaneNearA(&nOut0, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x04)
	{
		if (!ClipPlaneLeftA(&nOut1, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x08)
	{
		if (!ClipPlaneTopA(&nOut2, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x10)
	{
		if (!ClipPlaneRightA(&nOut3, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x20)
	{
		if (!ClipPlaneBottomA(&nOut4, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x02)
	{
		if (!ClipPlaneFarA(&nOut5, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}



static void sub_1000f6f0(Bucket0f6f0* pBucket, LTBOOL bRelease, uint32 nUnused)
{
	RVert       aVerts[80];          // -0x179c, 0xc80 bytes = 80 * 0x28
	RVert       aTri[32];            // -0x5e0 .. -0xe1, see note (3)
	LTVec       vScreen;             // -0xb14

	// -0x17b4 .. -0x17a0, in this order.  See the prologue note.
	float       fUScale, fVScale, fM00, fM01, fM10, fM11;

	// -0x18b0 .. -0x18a4, written as a group at 0x1000f907..0x1000f92b.
	RVert*      pClipVerts;          // -0x18b0
	int32       nClipVerts;          // -0x18ac
	void*       pXForm;              // -0x18a8 (copied to -0x68)
	uint32      nFVF;                // -0x18a4

	Elem0f6f0*  pElem;               // -0x6c
	Elem0f6f0*  pNext;               // -0x70
	SGeoObj*    pObj;                // -0x80
	SrcVert*    pSrcVerts;           // -0xb18
	int32       nSrcVerts;           // -0xb1c
	RVert*      pDest;
	RVert*      pWalk;
	RVert*      pVerts;              // -0x78
	RVert*      pOut;                // -0xb0
	int32       nVerts;              // -0xb8
	int32       nLeft;               // -0xc4
	uint32      nClipFlags;          // -0xc4
	PoolVert*   pPool;
	RVert*      pTriVerts;           // -0xe0
	int32       nTriVerts;           // -0xdc
	RVert*      pB;
	RVert*      pC;
	float       fOOW;                // -0xb8, spilled 64-bit

	// ------------------------------------------------------------------ //
	//  0x1000f719 .. 0x1000f782 -- twenty-two x87 instructions, no branch.
	//  Traced through the stack: the two `fstps` at 0x1000f72d/0x1000f741
	//  publish the scales, the four at 0x1000f770..0x1000f782 the matrix,
	//  and the SCHEDULE stores them ac, a0, a8, a4 -- so the source order
	//  below is the declaration order of the slots, not the store order.
	//  Each expression's LEFT operand is the one icl loads with `flds`.
	// ------------------------------------------------------------------ //
	fUScale = g_1008e66c * g_1008add4;
	fVScale = g_1008add4 * g_1008e670;

	fM00 = g_1008add8 * fUScale;
	fM01 = -g_1008addc * fVScale;
	fM10 = fUScale * g_1008addc;
	fM11 = g_1008add8 * fVScale;

	// 0x1000f788 -- `mov 0x4(%eax),%ebx ; test ; je <epilogue>`.
	pElem = pBucket->m_pElements;
	if (!pElem)
		return;

	do
	{
		// -------------------------------------------------------------- //
		//  0x1000f796 -- the loop head.
		// -------------------------------------------------------------- //
		pObj  = pElem->m_pOwner;
		pNext = pElem->m_pNext;

		// 0x1000f79e .. 0x1000f7bc.  Two separate `je 0x1001121f`, so two
		// source tests; the "no folding of redundant tests" rule.
		if (!pObj->m_p28->m_p30->m_p2c)
			goto L_Pooled;

		if (!g_1008ae00)
			goto L_Pooled;

		// -------------------------------------------------------------- //
		//  0x1000f7c2 -- the source geometry.  The call site pushes four
		//  words; the fourth is icl's dead slot.
		// -------------------------------------------------------------- //
		sub_1000a700(pObj, &pSrcVerts, &nSrcVerts);

		nLeft = nSrcVerts;
		pDest = aVerts;

		// 0x1000f7eb -- `test %eax,%eax ; je 0x1000f8f9`, the zero-trip
		// test of the `while (nLeft--)` below.
		while (nLeft--)
		{
			// 0x1000f80c .. 0x1000f835 -- three integer `mov`s through the
			// source's position POINTER, and the diffuse dword forced
			// white with the 0xffffffff hoisted out of the loop.
			pDest->m_vPos.x = pSrcVerts->m_pPos->x;
			pDest->m_vPos.y = pSrcVerts->m_pPos->y;
			pDest->m_vPos.z = pSrcVerts->m_pPos->z;
			*(uint32*)&pDest->m_b = 0xffffffff;

			// 0x1000f83e / 0x1000f852 -- texture pair TWO is the base
			// pair, scaled by stage one's inverse size.
			pDest->m_fU2 = g_1008e66c * pSrcVerts->m_fU;
			pDest->m_fV2 = g_1008e670 * pSrcVerts->m_fV;

			// 0x1000f85e -- `mov 0x1008ce8c,%eax ; call *%eax` with
			// %ecx = pDest (set at 0x1000f815) and %edx = pDest + 0x14
			// (the strength-reduced walker at 0x1000f803/0x1000f847).
			// The __fastcall big17c14.cpp pinned; it fills the specular.
			(*g_pfn_1008ce8c)(pDest, &pDest->m_sb);

			// 0x1000f865 .. 0x1000f897 -- `add $-0x18,%esp` then five
			// slots: the eye point, the source position, the surface
			// plane, and pointers to texture pair ONE.  The last two are
			// strength-reduced walkers in the image (-0x90 and %edi, both
			// `add $0x28`); the source takes the addresses.
			sub_1001e950(&g_v1008bd34, pSrcVerts->m_pPos, pObj->m_p28->m_p34,
			             &pDest->m_fU, &pDest->m_fV);

			// 0x1000f89c -- the reflection coordinates scaled.  The
			// emitted order is V then U and the operands are asymmetric;
			// see COIN-FLIP (2).
			pDest->m_fV = pDest->m_fV * g_1008add4;
			pDest->m_fU = g_1008add4 * pDest->m_fU;

			pSrcVerts++;
			pDest++;
		}

		// -------------------------------------------------------------- //
		//  0x1000f8f9 -- the four values the clipper and both draws work
		//  from, written as one group.
		// -------------------------------------------------------------- //
		pClipVerts = aVerts;
		nClipVerts = nSrcVerts;

		// 0x1000f904 -- this is the one member of the family that WRITES
		// the clip-flag global; big124a4.cpp and big64cc0.cpp only read it.
		g_1008b874 = pElem->m_n10;

		pXForm = g_1008b880;
		nFVF   = D3DFVF_ENVVERTEX;

		// 0x1000f918 .. 0x1000f940.  With D3D clipping off, or with the
		// near bit set (D3D cannot clip that), go per-triangle.  Byte for
		// byte big124a4.cpp's 0x10012a26.
		if (g_1008b874 && (!g_1008d7a0 || (g_1008b874 & 1)))
			goto L_PerTriangle;

		// 0x1000f94c -- the SECOND test of the same register.
		if (!g_1008b874)
		{
			// ---------------------------------------------------------- //
			//  0x1000f957 -- NO CLIPPING WANTED.  One projective transform
			//  per vertex through the block at +0x1dc..+0x218, `fdivrs
			//  1.0f` for the reciprocal, the three results assembled in a
			//  named three-float local (`rep movsl $3` at 0x1000fa44) and
			//  the reciprocal into +0x0c.  The identical instruction
			//  sequence, operand for operand, is big124a4.cpp's
			//  0x10012a65 and d3dbatchd.cpp's 0x1000a8d0.
			// ---------------------------------------------------------- //
			XForm0f6f0* pM = (XForm0f6f0*)pXForm;

			pWalk = pClipVerts;
			nLeft = nClipVerts;

			while (nLeft--)
			{
				fOOW = 1.0f / (pWalk->m_vPos.x * pM->m_f20c
				             + pWalk->m_vPos.y * pM->m_f210
				             + pWalk->m_vPos.z * pM->m_f214
				             + pM->m_f218);

				vScreen.x = (pWalk->m_vPos.x * pM->m_f1dc
				           + pWalk->m_vPos.y * pM->m_f1e0
				           + pWalk->m_vPos.z * pM->m_f1e4
				           + pM->m_f1e8) * fOOW;

				vScreen.y = (pWalk->m_vPos.x * pM->m_f1ec
				           + pWalk->m_vPos.y * pM->m_f1f0
				           + pWalk->m_vPos.z * pM->m_f1f4
				           + pM->m_f1f8) * fOOW;

				vScreen.z = (pWalk->m_vPos.x * pM->m_f1fc
				           + pWalk->m_vPos.y * pM->m_f200
				           + pWalk->m_vPos.z * pM->m_f204
				           + pM->m_f208) * fOOW;

				pWalk->m_vPos = vScreen;
				pWalk->m_fRHW = fOOW;
				pWalk++;
			}

			// 0x1000fa71 -- straight to the draw, no second projection.
			goto L_Draw;
		}

		// -------------------------------------------------------------- //
		//  0x1000fa76 -- to camera space, then 0x1000f4b0 INLINED.
		// -------------------------------------------------------------- //
		pWalk = pClipVerts;
		nLeft = nClipVerts;

		while (nLeft--)
		{
			sub_1000d9b0(&pWalk->m_vPos, g_1008b880);
			pWalk++;
		}

		// 0x1000fa9b -- 0x1000aa90 inlined: the flag word re-read from the
		// global, the six planes, and the clipped result written back.
		if (!ClipPolyA(g_1008b874, (void**)&pClipVerts, (uint32*)&nClipVerts))
			goto L_Next;

L_Project:
		// 0x100110c1 -- the screen projection, one call per vertex.  The
		// unclipped path does NOT come through here: it already did the
		// full projective transform inline.
		pWalk = pClipVerts;
		nLeft = nClipVerts;

		while (nLeft--)
		{
			sub_1000d940(pWalk, g_1008b880);
			pWalk++;
		}

L_Draw:
		// 0x100110e6 -- vtable +0x64, `add $-0x18,%esp` and six slots.
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLEFAN,
		                                    nFVF, pClipVerts, nClipVerts, 0);
		goto L_Next;

L_PerTriangle:
		// -------------------------------------------------------------- //
		//  0x10011129 -- the fan walked as (v0, vi, vi+1), each triangle
		//  copied into a three-vertex local with three `rep movsl $0xa`
		//  and handed to the out-of-line clipper.  nClipVerts itself is
		//  the counter: decremented and stored back at 0x10011204.
		// -------------------------------------------------------------- //
		if (nClipVerts > 2)
		{
			pB = pClipVerts + 1;
			pC = pClipVerts + 2;

			do
			{
				aTri[0] = pClipVerts[0];
				aTri[1] = *pB;
				aTri[2] = *pC;

				pTriVerts = aTri;
				nTriVerts = 3;

				// 0x100111ae -- three arguments in a 0x10 area, the
				// fourth slot dead.
				if (sub_10011420(&pTriVerts, &nTriVerts, pXForm))
					g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
						D3DPT_TRIANGLEFAN, nFVF, pTriVerts, nTriVerts, 0);

				pC++;
				pB++;
				nClipVerts--;
			}
			while (nClipVerts > 2);
		}
		goto L_Next;

L_Pooled:
		// -------------------------------------------------------------- //
		//  0x1001121f -- NO REFLECTION TEXTURE ON THIS SURFACE.  Nothing
		//  is rebuilt: the object's already-projected 32-byte vertices are
		//  read straight out of the shared pool, widened to 40 bytes, and
		//  redrawn with texture pair two derived from pair one through the
		//  2x2 the prologue built.
		// -------------------------------------------------------------- //
		pPool     = g_1008d524_Pool + pElem->m_n04;
		nSrcVerts = pElem->m_n08;

		nLeft = nSrcVerts;
		pDest = aVerts;

		while (nLeft--)
		{
			// 0x1001125b -- `rep movsl $8`, the 32-byte pooled vertex
			// into the low 32 bytes of the 40-byte draw vertex.
			*(PoolVert*)pDest = *pPool;

			// 0x1001126f.
			*(uint32*)&pDest->m_b = 0xffffffff;

			// 0x10011272 -- pair ONE rescaled by stage zero's inverse
			// size.  V is emitted first here too; see COIN-FLIP (2).
			pDest->m_fV = pPool->m_fV * g_1008e668;
			pDest->m_fU = pPool->m_fU * g_1008e664;

			// 0x1001128c / 0x100112ac -- pair TWO is pair one through the
			// 2x2.  `faddp` with the second term loaded FIRST is icl's
			// last-term-backwards association, so the source order is
			// (U term) + (V term).
			pDest->m_fU2 = pPool->m_fU * fM00 + pPool->m_fV * fM01;
			pDest->m_fV2 = pPool->m_fU * fM10 + pPool->m_fV * fM11;

			pPool++;
			pDest++;
		}

		// 0x100112cc -- the FVF is the LITERAL 0x2c4 here and the vertex
		// pointer is aVerts directly, not the nFVF/pClipVerts pair.
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLEFAN,
		                                    D3DFVF_ENVVERTEX, aVerts,
		                                    nSrcVerts, 0);

L_Next:
		// -------------------------------------------------------------- //
		//  0x10011309 -- the shared per-element tail.  The element's +0x00
		//  is REUSED as the free-list link; big124a4.cpp pops the same
		//  list with `g_1008d4fc = *(Elem124a4**)pFreeElem`.
		// -------------------------------------------------------------- //
		if (bRelease)
		{
			*(Elem0f6f0**)pElem = g_1008d4fc;
			g_1008d4fc = pElem;
		}

		pElem = pNext;
	}
	while (pElem);
}


// *********************************************************************** //
//  WHAT IS NOT PROVED, AND THE COIN-FLIPS
//
//  UNDECODED REGIONS: NONE.  Every byte of 0x1000f6f0..0x1001141d is
//  accounted for above, including the 235 bytes of sunk cold blocks (they
//  are the `else fT = 0.0f;` arms of the six epsilon tests and are written
//  as those arms, not as separate code).
//
//  CALL-FREE COVERAGE: 5,702 of 7,472 bytes
//  -- 76.3%, 0x1000fa92..0x100110d8 -- contain NO CALL and the call-sequence
//  comparison cannot see any of it.  That region is the six inlined clip planes.  It
//  rests on (a) cross-reference to four earlier independent decodes of the
//  same clipper (d3dr3a.cpp's out-of-line 0x10002d40 / 0x100030a0,
//  d3dr3c.cpp's driver 0x1000f4b0, big16324.cpp's and big124a4.cpp's
//  32-byte inlinings and big64cc0.cpp's 40-byte one), (b) the exact frame
//  decomposition set out in the header, and (c) the per-region instruction
//  census.  It does NOT rest on the call sequence, and it would be
//  dishonest to quote a call-sequence figure without this paragraph.
//
//  ONLY ONE REGION RESTS ON MY OWN READING ALONE: the env-map matrix at
//  0x1000f719..0x1000f782.  Twenty-two x87 instructions with no branch, no
//  call and no sibling anywhere in src/engine/.  The stack was traced
//  instruction by instruction (the trace is reproducible: six pushes, one
//  `fchs`, one `fmulp %st,%st(3)`, one `fxch %st(3)`, four `fstps`), and
//  the answer is confirmed at the far end of the function by 0x1001128c,
//  which uses the four results as a 2x2 in exactly the (00,01,10,11)
//  pattern -- but the ASSIGNMENT of each product to each slot is mine.
//
//  ------------------------------------------------------------------ //
//  (1) THE THIRD ARGUMENT.  %ecx is reloaded at 0x1000f700 and never read.
//      Written as `uint32 nUnused` because both call sites load a real
//      value into it (`mov -0x20(%ebp),%ecx`, itself 0x10011980's fourth
//      argument at 0x14(%ebp)).  The alternative is to drop it:
//          void sub_1000f6f0(Bucket0f6f0* pBucket, LTBOOL bRelease)
//      which costs one `mov` in the prologue.  big64cc0.cpp hit the same
//      thing and kept the parameter; kept here for the same reason.
//
//  (2) THE TEXTURE-SCALE MULTIPLY ORDER, twice.  At 0x1000f89c the image
//      loads m_fV then `fmuls g_1008add4`, loads g_1008add4 then
//      `fmuls m_fU`, `fxch`, and stores V then U -- opposite operand
//      order in the two statements and V first.  At 0x10011272 the same
//      shape with g_1008e668/g_1008e664, both with the vertex on the
//      left.  Written literally above (V's statement first).  The
//      alternatives, in order of plausibility:
//          pDest->m_fU *= g_1008add4;  pDest->m_fV *= g_1008add4;
//      and, for the pooled block,
//          pDest->m_fU = g_1008e664 * pPool->m_fU;
//          pDest->m_fV = g_1008e668 * pPool->m_fV;
//      big6d910.cpp records the identical asymmetry at 0x1006f2ff and
//      settled it the same way; one build over this file settles it here.
//
//  (3) THE 1,160-BYTE FRAME HOLE.  Everything in the frame decomposes
//      exactly EXCEPT -0x568..-0xe1, which is never addressed by any
//      instruction (checked: every ebp displacement in the function was
//      enumerated).  The one spelling that makes the frame close with no
//      remainder is
//          RVert aTri[32];       // -0x5e0 .. -0xe1, 32 * 0x28 = 0x500
//      -- the gap from aTri's base to the first scalar is EXACTLY 0x500 --
//      and only three elements are ever touched (the triangle handed to
//      0x10011420, whose clip can grow it).  BUILT: with
//      aTri[32] the frame is the shipped 0x18bc exactly (it was 0x1434)
//      and the function pairs on a 59-byte common opening:
//      attempted -> near.
//
//  (4) `float fT` vs `double fT`, and `float fOOW` vs `double fOOW`.  All
//      six planes spill the interpolant with `fstpl` and reload it with
//      `fmull` (0x1000fc0f/0x1000fc16 and five twins), and the unclipped
//      path spills the reciprocal with `fstl -0xb8` / `fldl -0xb8`.  Those
//      are 8-byte slots, which a `double` would give directly -- but so
//      does an icl x87 register spill of a `float` expression, and
//      big124a4.cpp, big16324.cpp, big5a960.cpp, big6d910.cpp and
//      big64cc0.cpp all wrote `float` over the same bytes.  Written
//      `float` here for consistency; `double fT; double fOOW;` is the
//      one-token alternative.
//
//  (5) THE 80-VERTEX BOUND.  aVerts[80] comes from the frame arithmetic
//      alone (0xc80 = 80 * 0x28 between -0x179c and -0xb1c) -- unlike
//      big124a4.cpp there is NO `sub_1000a740`-style bound check in this
//      function to confirm it, and nothing clamps nSrcVerts.  If the
//      declaration were `aVerts[81]` the frame would be 40 bytes larger,
//      so 80 is forced given the next local's address; but the next
//      local's address is only forced if nSrcVerts/pSrcVerts really are
//      the next two declarations, which is the assumption.
//
//  (6) WHICH TEXTURE PAIR IS "STAGE 0".  The reflection goes into +0x18/
//      +0x1c (pair one) and the base coordinates into +0x20/+0x24 (pair
//      two) -- the OPPOSITE of big64cc0.cpp, where 0x1001e950 writes
//      +0x20/+0x24.  That is read straight off the two pointers handed to
//      0x1001e950 at 0x1000f88f/0x1000f893 (%ecx = %ebx+0x18 and
//      %edi = %ebx+0x1c) and is not in doubt; it is flagged only because
//      a reader coming from big64cc0.cpp will expect the other way round.
//
//  ------------------------------------------------------------------ //
//  THE EXPECTED CALL-SEQUENCE RESIDUAL.  Ten shipped calls; the four indirect
//  ones (the __fastcall through 0x1008ce8c and the three DrawPrimitives)
//  carry no relocation, so the comparison matches them by POSITION only.  Two
//  of the three DrawPrimitives sit in blocks that /O2 may place
//  differently from the source order (L_PerTriangle and L_Pooled are both
//  laid out AFTER the main draw in the image and are written in that order
//  above, so they should line up).  `_chkstk` will not resolve unless the
//  local build emits it to the same symbol -- big124a4.cpp records the
//  same caveat for 0x100805a0.  There is no `sqrt` in this function, so
//  the known CRT-maths gap does not apply.
// *********************************************************************** //


// The bind, as w24_10059380_os.cpp's identical BindBlock59380 spells it
// (w3_10014580_os.cpp's 0x10015320 as an inline), over this file's types.
static inline Tex0ae20* FindRec11980(Tex0ae20* pHead, uint32 nStage)
{
	Tex0ae20* pCur;

	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
			return pCur;
	}

	return LTNULL;
}

static inline uint32 StageOf11980(Tex0ae20* pRec)
{
	return pRec->m_nStage42;
}

static inline void SetScale11980(TexScale0ae20* pScales, uint32 nStage, float fInvW, float fInvH)
{
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;
}

static inline void BindRec11980(Tex0ae20* pRec)
{
	TexBase0ae20* pTex = pRec;

	g_a1008e684[StageOf11980(pRec)] = (Tex0ae20*)pTex;

	if (pTex->m_nStamp14 != g_w1008bea8)
	{
		dl_Remove(&pRec->m_Link1c);
		dl_Insert(g_Link1008e634.m_pPrev, &pRec->m_Link1c);

		g_p1008cbcc->m_nBytes48 += pTex->m_nBytes10;

		pTex->m_nStamp14 = g_w1008bea8;
	}

	// state 0x18 = D3DRENDERSTATE_ALPHAREF.
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, (uint8)StageOf11980(pRec),
	                                 pTex->m_pSurface0c);

	if (pTex->m_nAlphaRef16)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18, pTex->m_nAlphaRef16);

	SetScale11980(g_a1008e664, StageOf11980(pRec), pTex->m_fInvW04, pTex->m_fInvH08);

	g_n1008bd80++;
}

static inline int32 BindBlock11980(TexBlock729c0* pBlock, uint32 nStage, uint32 nLOD)
{
	Tex0ae20* pRec;
	Tex0ae20* pHead;
	Tex0ae20* pNew;

	if (!pBlock)
		return 0;

	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pRecs0c;
	pRec  = LTNULL;

	if (pHead)
		pRec = FindRec11980(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	BindRec11980(pRec);

	if (pRec->m_nLOD44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nLOD44 = (uint16)nLOD;
	}

	return 1;

L_ElseLOD:
	if (pRec->m_nLOD44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nLOD44 = (uint16)nLOD;
	}

	return 1;

L_MakeRec:
	if (pBlock->m_pRecs0c)
	{
		pNew = sub_10039bd0(pBlock, nStage, 1);

		if (!pNew)
			return 0;

		pNew->m_pNext30  = pHead->m_pNext30;
		pHead->m_pNext30 = pNew;
		pRec = pNew;

		goto L_Bind;
	}

	pRec = sub_10039bd0(pBlock, nStage, 0);

	if (!pRec)
		return 0;

	goto L_Bind;
}


// ======================================================================= //
//  0x10011980 -- draw one bucket: (bucket, texture block, flag, mode).
//
//  The four binds are BindBlock11980 above, i.e.
//  w24_10059380_os.cpp's identical BindBlock59380 inline (FindRec,
//  BindRec with its TexBase copy, StageOf, SetScale, ltlink.h's
//  dl_Remove/dl_Insert) over this file's types, with a LOD of 0.  The
//  shipped body keeps every inline's locals in frame slots (`mov
//  %eax,-0x18 ; mov -0x18,%eax ; mov %eax,-0x8` at 0x10011a19 is the
//  FindRec result handed back); the hand-written BindBlock/FindRec it
//  replaces kept them in registers.  Measured: frame 0x28 -> the shipped
//  0x38, 2364 -> 2572 bytes (shipped 2560), 4% -> 57% of the bytes
//  agreeing at the own address: attempted -> near.  Residual: the eight
//  slots are a permutation of the shipped ones (pBucket at -0x8 for
//  -0x4); the inline's declaration order does not move them (all six
//  orders measured).
// ======================================================================= //
static void sub_10011980(Bucket729c0* pBucket, TexBlock729c0* pBlock, uint32 bFlag,
                  uint32 nMode)
{
	// A. the second block under the block, with its detail object.
	if (g_1008d950 && pBlock->m_pDetail2c && g_1008ae00
	 && BindBlock11980(pBlock->m_pBlock28, 0, 0))
	{
		BindBlock11980(pBlock, 1, 0);

		g_1008add4 = g_1008fe3c;
		sub_10011450((unsigned long)pBlock->m_pDetail2c);
		sub_1000f6f0((Bucket0f6f0*)pBucket, bFlag, nMode);
		sub_10011500();
		return;
	}

	// B. the block, and the second block over it.
	BindBlock11980(pBlock, 0, 0);

	if (g_1008d950 && g_1008aea0 && g_a1008e684[0] && pBlock->m_pBlock28
	 && !pBlock->m_pDetail2c && BindBlock11980(pBlock->m_pBlock28, 1, 0))
	{
		Tex0ae20* pTex = g_a1008e684[0];
		g_1008add4 = g_1008fe3c;
		if (pTex->Slot04() == 1)
		{
			g_1008add4 *= pTex->m_pXform18->m_fScale34;
			g_1008add8 = pTex->m_pXform18->m_fOffset38;
			g_1008addc = pTex->m_pXform18->m_fOffset3c;
		}
		else
		{
			g_1008add8 = 1.0f;
			g_1008addc = 0.0f;
		}

		sub_10011560();
		sub_1000f6f0((Bucket0f6f0*)pBucket, bFlag, nMode);
		sub_100115b0();
		return;
	}

	// C. straight from the pool.
	Elem729c0* pElem = pBucket->m_pElements;
	while (pElem)
	{
		PoolVert11980* pVerts =
			(PoolVert11980*)(g_1008d524 + pElem->m_iVert04 * sizeof(PoolVert11980));
		Elem729c0* pNext = pElem->m_pNext14;

		if (!nMode)
		{
			PoolVert11980* pVert = pVerts;
			for (uint32 n = pElem->m_nVerts08; n; n--, pVert++)
			{
				pVert->m_nColor10 = 0xffffffff;
				float fU = g_a1008e664[0].m_fInvW00 * pVert->m_fU18;
				float fV = g_a1008e664[0].m_fInvH04 * pVert->m_fV1c;
				pVert->m_fU18 = fU;
				pVert->m_fV1c = fV;
			}
		}

		// D3DPT_TRIANGLEFAN, FVF 0x1c4.
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6, 0x1c4, pVerts,
		                                    pElem->m_nVerts08, 0);

		if (bFlag)
		{
			pElem->m_pFreeNext = g_pFreeElems1008d4fc;
			g_pFreeElems1008d4fc = pElem;
		}

		pElem = pNext;
	}
}
