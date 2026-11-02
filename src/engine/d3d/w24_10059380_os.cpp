// *********************************************************************** //
//
//  w24_10059380_os.cpp -- written for the d3d.ren link.
//
//  0x10059380: the pointer table at 0x10089728's slot 0 (the table's
//  0x10089710..0x10089740 run holds 0x100592a0, 0x10059300, 0x1005a050,
//  0x1005a040, 0x10059fc0, 0x10059f70, THIS, 0x10059590, 0x10059620,
//  0x10059870, 0x10059880, 0x100598a0, 0x100598f0).
//
//  Flag set /Os: its table-mate 0x10059590 (d3dr5b.cpp, /Os) opens with the
//  same `55 8b ec / 83 ec xx` frame, and the pushed dead reserve before the
//  0x10039bd0 calls (`push %edi` then three arguments, `add $0x10`) is the
//  /Os shape w3_10014580_os.cpp's 0x10015320 records for the same calls.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ltlink.h's CheapLTLink, spelled with the tag big0ae20.cpp gives the
// used-list head at 0x1008e634, with its inline members.
struct Link0ae20
{
	Link0ae20*  m_pPrev;                                // +0x00
	Link0ae20*  m_pNext;                                // +0x04
	void*       m_pData;                                // +0x08

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

struct DDSurface;
struct DDSurface59380;

struct DDSurface59380Vtbl
{
	uint8   m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface59380* pThis, uint32 dwLOD);  // +0xbc
};

struct DDSurface59380
{
	DDSurface59380Vtbl* lpVtbl;
};

// IDirect3DDevice7 as the C++ interface (w19_1003b6f0_osoiqip.cpp's
// spelling): SetTexture's stage is evaluated BEFORE the device pointer is
// loaded (`movzwl 0x42(%ebx),%eax` then `mov 0x1008d988,%edx`), the order
// a C++ method call gives and a C `lpVtbl` call does not.
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
	virtual long __stdcall SetTexture(uint32 nStage, DDSurface59380* pSurf) = 0;    // +0x8c
};

struct TexScale0ae20
{
	float       m_fInvW00;                              // +0x00
	float       m_fInvH04;                              // +0x04
};

// The per-stage texture record (big0ae20.cpp's layout).
struct TexBase0ae20
{
	void*           m_pVtbl00;                          // +0x00
	float           m_fInvW04;                          // +0x04
	float           m_fInvH08;                          // +0x08
	DDSurface59380* m_pSurface0c;                       // +0x0c
	uint32          m_nBytes10;                         // +0x10
	uint16          m_nStamp14;                         // +0x14
	uint16          m_nAlphaRef16;                      // +0x16
	uint8           m_pad_18[0x1c - 0x18];
	Link0ae20       m_Link1c;                           // +0x1c
	uint8           m_pad_28[0x30 - 0x28];
};

struct Tex0ae20 : public TexBase0ae20
{
	Tex0ae20*       m_pNext30;                          // +0x30
	uint8           m_pad_34[0x42 - 0x34];
	uint16          m_nStage42;                         // +0x42
	uint16          m_nDirty44;                         // +0x44
};

struct TexBlock0ae20
{
	uint8       m_pad_00[0x0c];
	Tex0ae20*   m_pHead0c;                              // +0x0c
	uint8       m_pad_10[0x30 - 0x10];
	uint16      m_nStamp30;                             // +0x30
};

struct Stats0ae20
{
	uint8       m_pad_00[0x48];
	uint32      m_nBytes48;                             // +0x48
};

extern D3DDevice1003d350* g_pD3DDevice;                 // 0x1008d988
extern Tex0ae20*        g_a1008e684[];                  // 0x1008e684  bound per stage
extern TexScale0ae20    g_a1008e664[];                  // 0x1008e664  1/w, 1/h per stage
extern Link0ae20        g_Link1008e634;                 // 0x1008e634  the used list head
extern uint16           g_w1008bea8;                    // 0x1008bea8  frame stamp
extern uint16           g_w1008b870;                    // 0x1008b870  block stamp
extern Stats0ae20*      g_p1008cbcc;                    // 0x1008cbcc
extern uint32           g_n1008bd80;                    // 0x1008bd80  texture changes

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry (block, stage, bLink).
extern "C" Tex0ae20* sub_10039bd0(TexBlock0ae20* pBlock, uint32 nStage,
                                  uint32 bLink);


// ======================================================================= //
//  0x10059380 -- 528 bytes.  Bind the owner's texture for one stage.
//
//  w3_10014580_os.cpp's 0x10015320 (find or make the stage's record, bind
//  it, set its LOD) inlined with a LOD of 0, the block reached through
//  pOwner->+0x28->+0x30 (the chain big6d910.cpp's copy walks); its answer
//  (1 bound, 0 no block / no record) is the inline's return value, built
//  in %edx and moved to %eax at the one exit.  IDENTICAL.
//
//  __thiscall, `ret $0xc`: three stack arguments, the third never read,
//  `this` never read (the dead `push %edi` slot beside `sub $0x18`).
//  The block pointer is tested with `and $0xffffffff,%ecx`, the no-op
//  `& ~0` mark w19_10073c00_osoiqip.cpp / w20_100754d0_os.cpp record;
//  only an explicit `& 0xffffffff` reproduces it.
// ======================================================================= //

struct Mesh59380
{
	uint8           m_pad_00[0x30];
	TexBlock0ae20*  m_pBlock30;                         // +0x30
};

struct Owner59380
{
	uint8       m_pad_00[0x28];
	Mesh59380*  m_p28;                                  // +0x28
};

struct C10059380
{
	uint32 sub_10059380(Owner59380* pOwner, uint32 nStage, uint32 nUnused);
};

// The record walk, inlined as in w3_10014580_os.cpp's 0x10015320: the
// found record comes back in one register and is copied.
static inline Tex0ae20* FindRec59380(Tex0ae20* pHead, uint32 nStage)
{
	Tex0ae20* pCur;

	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
			return pCur;
	}

	return LTNULL;
}

// 0x1000a7d0's body (w3_1000a7d0_os.cpp), inlined, with ltlink.h's
// dl_Remove/dl_Insert inlined too.  WHAT PINNED IT (both callers below
// went from ~160 differing bytes to identical on these three levers):
//   * `mov %ebx,%esi` at entry, then the stage and the list links read
//     through %ebx and every texture field through %esi: the texture
//     fields are reached through a BASE-class pointer converted from the
//     record (TexBase0ae20* = Tex0ae20*), which icl keeps as a copy; the
//     same-typed copy is coalesced away (measured);
//   * the stage is read BEFORE the device pointer for SetTexture, and
//     re-read from the record at every use: an inline accessor (icl
//     evaluates inlined calls ahead of the other operands);
//   * the 1/w, 1/h pair is loaded into two registers and stored through
//     `mov $0x1008e664,%edx ; (%edx,%eax,8)`: an inline setter handed
//     the array as a pointer argument.
static inline uint32 StageOf59380(Tex0ae20* pRec)
{
	return pRec->m_nStage42;
}

static inline void SetScale59380(TexScale0ae20* pScales, uint32 nStage, float fInvW, float fInvH)
{
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;
}

static inline void BindRec59380(Tex0ae20* pRec)
{
	TexBase0ae20* pTex = pRec;

	g_a1008e684[StageOf59380(pRec)] = (Tex0ae20*)pTex;

	if (pTex->m_nStamp14 != g_w1008bea8)
	{
		dl_Remove(&pRec->m_Link1c);
		dl_Insert(g_Link1008e634.m_pPrev, &pRec->m_Link1c);

		g_p1008cbcc->m_nBytes48 += pTex->m_nBytes10;

		pTex->m_nStamp14 = g_w1008bea8;
	}

	// vtable +0x8c SetTexture, the stage narrowed with `movzbl %al,%eax`.
	g_pD3DDevice->SetTexture((uint8)StageOf59380(pRec), pTex->m_pSurface0c);

	if (pTex->m_nAlphaRef16)
	{
		// vtable +0x50, state 0x18 = D3DRENDERSTATE_ALPHAREF.
		g_pD3DDevice->SetRenderState(0x18, pTex->m_nAlphaRef16);
	}

	SetScale59380(g_a1008e664, StageOf59380(pRec), pTex->m_fInvW04, pTex->m_fInvH08);

	g_n1008bd80++;
}

// w3_10014580_os.cpp's 0x10015320 (find or make the stage's record, bind
// it, set its LOD), as an inline function.
static inline int32 BindBlock59380(TexBlock0ae20* pBlock, uint32 nStage, uint32 nLOD)
{
	Tex0ae20* pRec;
	Tex0ae20* pHead;
	Tex0ae20* pNew;

	if (!pBlock)
		return 0;

	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pHead0c;
	pRec  = LTNULL;

	if (pHead)
		pRec = FindRec59380(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	BindRec59380(pRec);

	if (pRec->m_nDirty44 != nLOD)
	{
		// vtable +0xbc.
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nDirty44 = (uint16)nLOD;
	}

	return 1;

L_ElseLOD:
	// Already bound to this stage: only the LOD update survives.
	if (pRec->m_nDirty44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nDirty44 = (uint16)nLOD;
	}

	return 1;

L_MakeRec:
	if (pBlock->m_pHead0c)
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

uint32 C10059380::sub_10059380(Owner59380* pOwner, uint32 nStage, uint32 nUnused)
{
	return BindBlock59380((TexBlock0ae20*)((uint32)pOwner->m_p28->m_pBlock30 & 0xffffffff),
	                      nStage, 0);
}


// ======================================================================= //
//  0x10059620 -- 579 bytes.  The same bind, one level further in, then
//  publish the stage's 1/w, 1/h pair scaled into this+0x788.
//
//  __thiscall on d3dr5b.cpp's CTexStage (`this` spilled to -0x10(%ebp)
//  and written at +0x788 / +0x78c, the pair 0x10059fc0 also refreshes),
//  `ret $0xc`, the third argument never read.  The block is reached
//  through pOwner->+0x28->+0x30->+0x28, the chain 0x10059fc0 walks, with
//  the same two null guards; the answer is returned directly (`xor
//  %eax,%eax ; inc %eax` beside the scale, one shared `xor %eax,%eax`
//  for every failure), so the bind's answer is tested, not returned.
//  The two `je` in a row after `test %ecx,%ecx` are the guard's test and
//  the inline's own `if (!pBlock)`.  IDENTICAL.
// ======================================================================= //

extern float g_1008fe3c;                                // 0x1008fe3c

struct Node59620
{
	uint8           m_pad_00[0x28];
	TexBlock0ae20*  m_pBlock28;                         // +0x28
};

struct Mesh59620
{
	uint8       m_pad_00[0x30];
	Node59620*  m_p30;                                  // +0x30
};

struct Owner59620
{
	uint8       m_pad_00[0x28];
	Mesh59620*  m_p28;                                  // +0x28
};

struct TexPair
{
	float u;                                            // +0x00
	float v;                                            // +0x04
};

struct CTexStage
{
	uint8       m_pad_000[0x788];                       // +0x000
	TexPair     m_Cur788;                               // +0x788

	uint32 sub_10059620(Owner59620* pOwner, uint32 nStage, uint32 nUnused);
};

uint32 CTexStage::sub_10059620(Owner59620* pOwner, uint32 nStage, uint32 nUnused)
{
	Node59620* pNode;

	pNode = pOwner->m_p28->m_p30;

	if (pNode && pNode->m_pBlock28)
	{
		if (BindBlock59380(pNode->m_pBlock28, nStage, 0))
		{
			m_Cur788.u = g_a1008e664[nStage].m_fInvW00 * g_1008fe3c;
			m_Cur788.v = g_a1008e664[nStage].m_fInvH04 * g_1008fe3c;

			return 1;
		}
	}

	return 0;
}


// ======================================================================= //
//  0x10059870 -- 12 bytes: `push %esi ; xor %eax,%eax ; inc %eax ;
//  add $0x4,%esp ; ret $0xc`.
//
//  The slot after 0x10059620 in the same table: a three-argument
//  __thiscall member that ignores `this` (the dead four-byte slot) and
//  every argument and answers 1 -- the accepting twin of d3dhigh.cpp's
//  refusing 0x10056aa0.  The argument types are unrecoverable; they are
//  spelled as 0x10059620's.  Padding after it is `8b f6` then 0x90: /Os.
// ======================================================================= //

struct C10059870
{
	uint32 sub_10059870(Owner59620* pOwner, uint32 nStage, uint32 nUnused);
};

uint32 C10059870::sub_10059870(Owner59620* pOwner, uint32 nStage, uint32 nUnused)
{
	return 1;
}


// ======================================================================= //
//  0x10059880 -- 20 bytes.  The "opaque white" case of d3dhigh.cpp's
//  0x100598a0: the same two-argument __thiscall shape (`ret $0x8`, `this`
//  ignored), the source never read, and the destination's packed colour
//  at +0x10 set to 0xffffffff in one dword store.
// ======================================================================= //

struct ColorDest59880
{
	uint8   m_pad_00[0x10];                             // +0x00
	uint32  m_nColor10;                                 // +0x10
};

struct C10059880
{
	void sub_10059880(const void* pSrc, ColorDest59880* pDest);
};

void C10059880::sub_10059880(const void* pSrc, ColorDest59880* pDest)
{
	pDest->m_nColor10 = 0xffffffff;
}
