// *********************************************************************** //
//
//  w19_1003b6f0_osoiqip.cpp -- the render-state object whose vtable is
//  0x10089530 (slots 0x1003bb10, 0x1003d350, 0x1003d5c0, 0x1003d5f0,
//  0x1003d8f0; constructor 0x1003b924 in d3dbatchd.cpp, which clears the
//  eight dwords at +0x2c), and the blend-mode map its state setter calls:
//
//    0x1003b6f0 (4 + 64)  blend mode -> D3DBLEND, a /Qip two-entry pair
//                         (stack entry 0x1003b6f0, register entry
//                         0x1003b6f4 with the argument in %eax);
//    0x1003d350 (624)     slot +0x04  SetState(nState, nValue);
//    0x1003d5c0 (40)      slot +0x08  GetState(nState, &nValue);
//    0x1003d5f0 (758)     slot +0x0c  SetTexture(pTexture).
//
//  FLAGS.  0x1003d3ed/0x1003d41c call 0x1003b6f4 -- the register entry --
//  with %eax loaded, which only a caller in 0x1003b6f0's own /Qip TU does,
//  so the map and the setter share this file.  Every body pads the /Os way
//  (to 4 with a lea/mov filler, then 0x90).  No CRT call anywhere, so /Oi-
//  is inert; /Os /Oi- /Qip is the _osoiqip convention.
//
//  The enumeration split 0x1003d350 at its jump-table targets (rows
//  0x1003d37d, 0x1003d39e, 0x1003d3bf, 0x1003d3e0, 0x1003d40f, 0x1003d43e,
//  0x1003d483, 0x1003d4ee are the eight `case` bodies, reached only through
//  the table at 0x100a99e8); the function is 0x1003d350..0x1003d5c0.
//
// *********************************************************************** //

typedef unsigned long  uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;


// ======================================================================= //
//  THE DEVICE AND THE TEXTURE RECORDS
// ======================================================================= //

struct DDSurface1003d5f0;
struct DDSurface1003d5f0Vtbl
{
	void*	m_pad_00[47];                                       // +0x00..+0xb8
	long	(__stdcall* SetLOD)(DDSurface1003d5f0*, uint32);    // +0xbc
};
struct DDSurface1003d5f0
{
	DDSurface1003d5f0Vtbl* lpVtbl;
};

// IDirect3DDevice7: +0x50 SetRenderState, +0x8c SetTexture, +0x94
// SetTextureStageState.  Declared as the C++ interface: the state setter
// evaluates the device pointer BEFORE the argument that calls 0x1003b6f0
// (0x1003d3e0 loads and spills it first), which is `this` going first.
class D3DDevice1003d350
{
public:
	virtual long __stdcall _v00() = 0;
	virtual long __stdcall _v04() = 0;
	virtual long __stdcall _v08() = 0;
	virtual long __stdcall _v0c() = 0;
	virtual long __stdcall _v10() = 0;
	virtual long __stdcall _v14() = 0;
	virtual long __stdcall _v18() = 0;
	virtual long __stdcall _v1c() = 0;
	virtual long __stdcall _v20() = 0;
	virtual long __stdcall _v24() = 0;
	virtual long __stdcall _v28() = 0;
	virtual long __stdcall _v2c() = 0;
	virtual long __stdcall _v30() = 0;
	virtual long __stdcall _v34() = 0;
	virtual long __stdcall _v38() = 0;
	virtual long __stdcall _v3c() = 0;
	virtual long __stdcall _v40() = 0;
	virtual long __stdcall _v44() = 0;
	virtual long __stdcall _v48() = 0;
	virtual long __stdcall _v4c() = 0;
	virtual long __stdcall SetRenderState(uint32 nState, uint32 nValue) = 0;        // +0x50
	virtual long __stdcall _v54() = 0;
	virtual long __stdcall _v58() = 0;
	virtual long __stdcall _v5c() = 0;
	virtual long __stdcall _v60() = 0;
	virtual long __stdcall _v64() = 0;
	virtual long __stdcall _v68() = 0;
	virtual long __stdcall _v6c() = 0;
	virtual long __stdcall _v70() = 0;
	virtual long __stdcall _v74() = 0;
	virtual long __stdcall _v78() = 0;
	virtual long __stdcall _v7c() = 0;
	virtual long __stdcall _v80() = 0;
	virtual long __stdcall _v84() = 0;
	virtual long __stdcall _v88() = 0;
	virtual long __stdcall SetTexture(uint32 nStage, DDSurface1003d5f0* pSurf) = 0; // +0x8c
	virtual long __stdcall _v90() = 0;
	virtual long __stdcall SetTextureStageState(uint32 nStage, uint32 nType,
	                                            uint32 nValue) = 0;                 // +0x94
};

extern D3DDevice1003d350* g_pD3DDevice;                 // 0x1008d988

// The texture record (big0ae20.cpp's Tex0ae20, same offsets).
// ltlink.h's CheapLTLink with its inline members (w24_10059380_os.cpp's
// Link0ae20), so the relink is the header's dl_Remove / dl_Insert.
struct Link1003d5f0
{
	Link1003d5f0*	m_pPrev;                            // +0x00
	Link1003d5f0*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08

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

	inline void AddAfter(Link1003d5f0* pLink)
	{
		pLink->m_pPrev = this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
	}
};

inline void dl_Insert(Link1003d5f0* pAfter, Link1003d5f0* pLink)
{
	pAfter->AddAfter(pLink);
}

// Flat here, not ltlink.h's `pLink->Remove()`: under /Qip an inline that
// calls an inline makes icl emit an out-of-line copy of the callee (the
// LTMatrix::Inverse finding), and the image holds no such copy in this TU
// (its only dl_Remove is d3dneutral.cpp's 0x1000a6e0).  Same bytes in
// 0x1003d5f0 either way.
inline void dl_Remove(Link1003d5f0* pLink)
{
	pLink->m_pPrev->m_pNext = pLink->m_pNext;
	pLink->m_pNext->m_pPrev = pLink->m_pPrev;
	pLink->m_pPrev = pLink->m_pNext = pLink;
}

// The record's texture half as a BASE class (w24_10059380_os.cpp's
// TexBase0ae20): the bind reaches the texture fields through a base
// pointer converted from the record, which icl keeps as a second copy.
struct TexBase1003d5f0
{
	void*				m_pVtbl00;                      // +0x00
	float				m_fInvW04;                      // +0x04
	float				m_fInvH08;                      // +0x08
	DDSurface1003d5f0*	m_pSurface0c;                   // +0x0c
	uint32				m_nBytes10;                     // +0x10
	uint16				m_nStamp14;                     // +0x14
	uint16				m_nAlphaRef16;                  // +0x16
	uint8				m_pad_18[0x1c - 0x18];
	Link1003d5f0		m_Link1c;                       // +0x1c
	uint8				m_pad_28[0x30 - 0x28];
};

struct Tex1003d5f0 : public TexBase1003d5f0
{
	Tex1003d5f0*		m_pNext30;                      // +0x30
	uint8				m_pad_34[0x42 - 0x34];
	uint16				m_nStage42;                     // +0x42
	uint16				m_nDirty44;                     // +0x44
};

struct TexBlock1003d5f0
{
	uint8			m_pad_00[0x0c];
	Tex1003d5f0*	m_pHead0c;                          // +0x0c
	uint8			m_pad_10[0x30 - 0x10];
	uint16			m_nStamp30;                         // +0x30
};

struct TexScale1003d5f0
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};

// 0x1008cbcc: +0x04 is called with the engine texture and answers its
// renderer block (`call *0x4(%edx)`, cdecl, one argument); +0x48 is the
// texture-memory counter big0ae20.cpp and big2ef34.cpp record.
struct RS1003d5f0
{
	void*				m_p00;                                      // +0x00
	TexBlock1003d5f0*	(*m_pfnGetBlock04)(void* pTexture);         // +0x04
	uint8				m_pad_08[0x48 - 0x08];
	uint32				m_nBytes48;                                 // +0x48
};

extern RS1003d5f0*		g_p1008cbcc;                    // 0x1008cbcc
extern uint16			g_w1008b870;                    // 0x1008b870
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern uint32			g_n1008bd80;                    // 0x1008bd80
extern uint32			g_n1008d7d0;                    // 0x1008d7d0  stage
extern Link1003d5f0		g_Link1008e634;                 // 0x1008e634
extern TexScale1003d5f0	g_a1008e664[];                  // 0x1008e664
extern Tex1003d5f0*		g_a1008e684[];                  // 0x1008e684

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry: (block, stage, bLink).
extern "C" Tex1003d5f0* sub_10039bd0(TexBlock1003d5f0* pBlock, uint32 nStage,
                                     uint32 bLink);

// 0x1003b690 -- d3dmid.cpp: renderer blend op -> D3DTEXTUREOP, falling
// back to the second argument.  It recurses on its default arm.
uint32 sub_1003b690(uint32 nOp, uint32 nFallbackOp);


// ======================================================================= //
//  0x1003b6f0 -- renderer blend mode -> D3DBLEND.  5, 6, 3, 4, 1 map to
//  themselves, tested in that order, anything else to 2 (D3DBLEND_ONE);
//  %eax is preloaded with 2 before the first test.
// ======================================================================= //
uint32 sub_1003b6f0(uint32 nMode)
{
	uint32 nBlend = 2;

	if (nMode == 5)
		nBlend = 5;
	else if (nMode == 6)
		nBlend = 6;
	else if (nMode == 3)
		nBlend = 3;
	else if (nMode == 4)
		nBlend = 4;
	else if (nMode == 1)
		nBlend = 1;

	return nBlend;
}


// ======================================================================= //
//  THE OBJECT
// ======================================================================= //
class C10089530
{
public:
	uint32	sub_1003d350(uint32 nState, uint32 nValue);     // +0x04
	uint32	sub_1003d5c0(uint32 nState, uint32* pValue);    // +0x08
	uint32	sub_1003d5f0(void* pTexture);                   // +0x0c

	void*	m_pVtbl;                                        // +0x00
	uint32	m_pad_04[10];                                   // +0x04..+0x28
	uint32	m_States[8];                                    // +0x2c
};


// 0x1003d4bf / 0x1003d524 are 0x1003b690 inlined ONE level: its switch
// runs in place and only the default arm calls out, as
// sub_1003b690(nFallback, nFallback).
inline uint32 MapTextureOp(uint32 nOp, uint32 nFallbackOp)
{
	switch (nOp)
	{
		case 1: return 2;
		case 2: return 3;
		case 3: return 4;
		case 4: return 7;
		case 5: return 8;
		default: return sub_1003b690(nFallbackOp, nFallbackOp);
	}
}


// Case 5's wrap/clamp choice, expanded in place.
inline uint32 MapAddress(uint32 nValue)
{
	uint32 nAddress;

	if (nValue == 1)
		nAddress = 1;
	else if (nValue == 2)
		nAddress = 3;
	else
		nAddress = 1;

	return nAddress;
}


// ----------------------------------------------------------------------- //
//  0x1003d350 -- thiscall, `ret $0x8`.  A state out of range answers 1;
//  otherwise the device is told and the value remembered at +0x2c.
//    0  D3DRENDERSTATE_ALPHABLENDENABLE (0x1b)   the value
//    1  D3DRENDERSTATE_ZENABLE          (0x07)   the value
//    2  D3DRENDERSTATE_ZWRITEENABLE     (0x0e)   the value
//    3  D3DRENDERSTATE_SRCBLEND         (0x13)   0x1003b6f0(value)
//    4  D3DRENDERSTATE_DESTBLEND        (0x14)   0x1003b6f0(value)
//    5  stage 0 D3DTSS_ADDRESS          (0x0c)   2 -> CLAMP (3), else WRAP (1)
//    6  stage 0 D3DTSS_COLOROP          (0x01)   the op map, MODULATE fallback
//    7  stage 0 D3DTSS_ALPHAOP          (0x04)   the op map, SELECTARG1 fallback
// ----------------------------------------------------------------------- //
uint32 C10089530::sub_1003d350(uint32 nState, uint32 nValue)
{
	if (nState >= 8)
		return 1;

	switch (nState)
	{
		case 0:
			g_pD3DDevice->SetRenderState(0x1b, nValue);
			break;

		case 1:
			g_pD3DDevice->SetRenderState(0x07, nValue);
			break;

		case 2:
			g_pD3DDevice->SetRenderState(0x0e, nValue);
			break;

		case 3:
			g_pD3DDevice->SetRenderState(0x13, sub_1003b6f0(nValue));
			break;

		case 4:
			g_pD3DDevice->SetRenderState(0x14, sub_1003b6f0(nValue));
			break;

		case 5:
			g_pD3DDevice->SetTextureStageState(0, 0x0c, MapAddress(nValue));
			break;

		case 6:
			g_pD3DDevice->SetTextureStageState(0, 1, MapTextureOp(nValue, 3));
			break;

		case 7:
			g_pD3DDevice->SetTextureStageState(0, 4, MapTextureOp(nValue, 1));
			break;
	}

	m_States[nState] = nValue;
	return 0;
}


// ----------------------------------------------------------------------- //
//  0x1003d5c0 -- thiscall, `ret $0x8`.  The remembered value, or 1 for a
//  state out of range.
// ----------------------------------------------------------------------- //
uint32 C10089530::sub_1003d5c0(uint32 nState, uint32* pValue)
{
	if (nState >= 8)
		return 1;

	*pValue = m_States[nState];
	return 0;
}


// The two pieces of 0x1003d5f0 that big0ae20.cpp, big2ef34.cpp and
// big6d910.cpp also carry expanded: clearing the current stage, and making
// a record current (relinking it at the back of the list when it has not
// been touched this frame, then telling the device).
inline void UnbindStage()
{
	uint32 nStage = g_n1008d7d0;

	if (g_a1008e684[nStage])
	{
		g_pD3DDevice->SetTexture(nStage, 0);
		g_a1008e684[nStage] = 0;
	}
}

inline void ResetLOD(Tex1003d5f0* pRec)
{
	if (pRec->m_nDirty44)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
		pRec->m_nDirty44 = 0;
	}
}

static inline Tex1003d5f0* FindRec1003d5f0(Tex1003d5f0* pHead, uint32 nStage)
{
	Tex1003d5f0* pCur;

	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
			return pCur;
	}

	return 0;
}

static inline uint32 StageOf1003d5f0(Tex1003d5f0* pRec)
{
	return pRec->m_nStage42;
}

static inline void SetScale1003d5f0(TexScale1003d5f0* pScales, uint32 nStage, float fInvW, float fInvH)
{
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;
}

inline void BindRec(Tex1003d5f0* pRec)
{
	TexBase1003d5f0* pTex = pRec;

	g_a1008e684[StageOf1003d5f0(pRec)] = (Tex1003d5f0*)pTex;

	if (pTex->m_nStamp14 != g_w1008bea8)
	{
		dl_Remove(&pRec->m_Link1c);
		dl_Insert(g_Link1008e634.m_pPrev, &pRec->m_Link1c);

		g_p1008cbcc->m_nBytes48 += pTex->m_nBytes10;

		pTex->m_nStamp14 = g_w1008bea8;
	}

	g_pD3DDevice->SetTexture((uint8)StageOf1003d5f0(pRec), pTex->m_pSurface0c);

	if (pTex->m_nAlphaRef16)
		g_pD3DDevice->SetRenderState(0x18, pTex->m_nAlphaRef16);

	SetScale1003d5f0(g_a1008e664, StageOf1003d5f0(pRec), pTex->m_fInvW04, pTex->m_fInvH08);

	g_n1008bd80++;
}


// ----------------------------------------------------------------------- //
//  0x1003d5f0 -- thiscall, `ret $0x4`.  Bind a texture to the current
//  stage: find the block's record for this stage (or make one), make it
//  current, reset its LOD.  No texture unbinds the stage and answers 0;
//  no block, or a record that cannot be made, unbinds it and answers 1.
//
//  IDENTICAL (was 560 of 758 bytes differing), written as
//  w24_10059380_os.cpp's identical copy of the same bind block:
//    * the texture half of the record is a BASE class (TexBase1003d5f0)
//      and the bind works through a base pointer converted from the
//      record -- the shipped second copy of the record pointer in %esi;
//      the stage through an inline accessor, the 1/w,1/h pair through an
//      inline setter handed the array, the relink as dl_Remove/dl_Insert;
//    * the search is an inline returning the found record, and the flow
//      is the w24 one with labels: not found -> L_MakeRec, already bound
//      -> L_ElseLOD, else L_Bind;
//    * L_MakeRec re-reads `pBlock->m_pHead0c` (the shipped not-found jump
//      goes to that test rather than being threaded past it);
//    * the linked make-record result is a second local (pNew) copied into
//      pRec on success before `goto L_Bind` -- the shipped copy sits after
//      the null test -- and both failures share one unbind;
//    * L_ElseLOD's `return 0` is a `goto` to the label on the bind path's
//      return: its LOD test then jumps straight to that block (with its
//      own `return 0` the test lands on a trailing `jmp`, 1 byte off).
// ----------------------------------------------------------------------- //
uint32 C10089530::sub_1003d5f0(void* pTexture)
{
	TexBlock1003d5f0*	pBlock;
	Tex1003d5f0*		pHead;
	Tex1003d5f0*		pRec;
	Tex1003d5f0*		pNew;
	uint32				nStage;

	if (!pTexture)
	{
		UnbindStage();
		return 0;
	}

	pBlock = g_p1008cbcc->m_pfnGetBlock04(pTexture);
	if (!pBlock)
	{
		UnbindStage();
		return 1;
	}

	nStage = g_n1008d7d0;
	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pHead0c;
	pRec  = 0;

	if (pHead)
		pRec = FindRec1003d5f0(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	BindRec(pRec);
	ResetLOD(pRec);
L_Ret0:
	return 0;

L_ElseLOD:
	ResetLOD(pRec);
	goto L_Ret0;

L_MakeRec:
	if (pBlock->m_pHead0c)
	{
		pNew = sub_10039bd0(pBlock, nStage, 1);
		if (pNew)
		{
			pNew->m_pNext30  = pHead->m_pNext30;
			pHead->m_pNext30 = pNew;
			pRec = pNew;
			goto L_Bind;
		}
	}
	else
	{
		pRec = sub_10039bd0(pBlock, nStage, 0);
	}

	if (!pRec)
	{
		UnbindStage();
		return 1;
	}

	goto L_Bind;
}
