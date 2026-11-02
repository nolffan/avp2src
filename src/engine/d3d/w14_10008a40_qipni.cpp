// *********************************************************************** //
//
//  w14_10008a40_qipni.cpp -- CD3DModelDraw::sub_10008a40 and
//  ::sub_10008da0 (written for the d3d.ren link; 0x10008da0
//  moved here from w2_10008da0.cpp).
//
//  Draw every visible piece of the current model through one of the five
//  vertex-setup routines (w14_10006840.cpp), which 0x10008da0 picks and
//  hands over as the first argument:
//
//    * choose the output format from +0x428: 1 is the one-texture
//      32-byte vertex (FVF 0x1c4, the copy 0x10003bb0 and the clipper
//      0x10003c40), anything else the two-texture 40-byte one (FVF
//      0x2c4, 0x10003bf0 and 0x100051a0); +0x6b0 then picks the bank
//      (0x1008acd4 / 0x1008ad4c, 0x1008ac98 / 0x1008ad10);
//    * rebuild the banks (0x100039d0, 0x10009ae0) when ModelVBSize or
//      ModelVBCount no longer match the chosen bank;
//    * +0x6ac nonzero draws through the model cache at 0x10091d80
//      instead, untransformed (the rhw dropped from FVF and stride);
//    * per piece: skip hidden ones (+0x204 of the object), rebind the
//      texture when it changes (0x10047230, 0x10047b60), take the uv
//      scale from the piece or its console override, and call the setup
//      routine on the piece's vertex run and its slice of +0x62c.
//
//  /O2 /Qip /Qip_no_inlining: the shipped body is the two-entry /Qip pair
//  -- a stack entry `mov 0x4(%esp),%eax` at 0x10008a40 and the register
//  entry 0x10008a44 -- and its only caller, 0x10008da0, calls 0x10008a44
//  with the setup routine in %eax as well as on the stack: the two shared
//  a /Qip TU.  Both are __thiscall.  /Qip alone inlines 0x10008a40 into
//  its single caller, which the shipped build did not.
//
//  0x10008a40 (the stub) and 0x10008da0 are byte-identical.  0x10008a44
//  is NEAR by one allocation choice: the shipped loop re-reads the piece
//  count from the model (`mov 0x3c(%edx),%eax`) at every test, ours keeps
//  it in a stack slot refreshed after the setup call; loop spellings
//  (index/while/reversed compare/int, merged or nested skips) all build
//  the same, and a volatile read of the count is worse.  Also: the
//  tex-scale members as dword stores (a union) change nothing; GetSet
//  writing through an out-pointer (pSet address-taken) makes the body
//  size-exact (849) but 328 off, the count still spilled.
//  IDENTICAL -- the loop reads the pieces through the
//  model's inline NumPieces() / GetPiece(i) (see D3DModel8a40).
//
//  0x10008c12 (the enumeration's 398-byte "gap" row) is NOT a function:
//  it is the inside of 0x10008a44's piece loop, the texture-rebind block
//  (`push %edi ; mov %ebx,%ecx ; call 0x10047230 ... call 0x10047b60`)
//  that the `je 0x10008c2b` at 0x10008c10 falls into.  Nothing calls or
//  points at it, and the body above covers it.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct ModelVertSet;
struct RVert;
struct Vert3c40;
struct Vert051a0;

// The Direct3D device (w2_10011620_os.cpp's view): SetRenderState only.
struct D3DDevice;
struct D3DDeviceVtbl
{
	uint8   m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);   // +0x50
};
struct D3DDevice
{
	D3DDeviceVtbl* lpVtbl;
};
extern D3DDevice*   g_pD3DDevice;                           // 0x1008d988

// A model vertex-buffer bank (w2_10009ae0.cpp's I100039d0).  The model
// cache at 0x10091d80 shares the interface and keeps its vertex format
// and stride at +0x58 / +0x54.
struct I100039d0
{
	virtual void    Slot00();                               // +0x00

	uint8           m_pad_04[0x20 - 0x04];                  // +0x04
	uint32          m_nSize20;                              // +0x20
	uint32          m_nBuffers24;                           // +0x24
	uint8           m_pad_28[0x54 - 0x28];                  // +0x28
	uint32          m_nStride54;                            // +0x54
	uint32          m_nFVF58;                               // +0x58
};

extern I100039d0    g_1008ac98;                             // 0x1008ac98
extern I100039d0    g_1008acd4;                             // 0x1008acd4
extern I100039d0    g_1008ad10;                             // 0x1008ad10
extern I100039d0    g_1008ad4c;                             // 0x1008ad4c
extern I100039d0    g_10091d80;                             // 0x10091d80

// d3ddata.cpp's tag for a console variable (w2_10009ae0.cpp's spelling).
struct CConVar
{
	int32       m_nValue00;                                 // +0x00
	float       m_fValue04;                                 // +0x04
	uint8       m_pad_08[0x20 - 0x08];                      // +0x08
};
extern CConVar      g_1008ac10;                             // 0x1008ac10  ModelVBSize
extern CConVar      g_1008abf0;                             // 0x1008abf0  ModelVBCount

// 0x1008b874 -- a model-draw mode word (big17c14.cpp's spelling).
extern unsigned long g_1008b874;                            // 0x1008b874
// 0x1008ca38 -- ModelProfile console variable.
extern long         g_CV_ModelProfile;                      // 0x1008ca38
// 0x1008ac54 / 0x1008ac34 -- u/v overrides for the model texture scale.
extern float        g_f1008ac54;                            // 0x1008ac54
extern float        g_f1008ac34;                            // 0x1008ac34

// 0x100039d0 / 0x10009ae0 -- release and rebuild the model vertex banks.
void sub_100039d0();
void sub_10009ae0();
// 0x10039420 -- d3dqip2.cpp's spelling.
void sub_10039420(float fValue);
// 0x10021d10 -- the console printf.
extern "C" void sub_10021d10(const char* pFmt, ...);

// The bank-vertex copies and the two clippers, installed by address.
extern "C" void __fastcall sub_10003bb0(void* pDst, void* pSrc);
extern "C" void __fastcall sub_10003bf0(void* pDst, void* pSrc);
LTBOOL __fastcall sub_10003c40(uint32 nClip, Vert3c40** ppVerts,
                               int32* pnVerts);
LTBOOL __fastcall sub_100051a0(uint32 nClip, Vert051a0** ppVerts,
                               int32* pnVerts);
// The environment-map uv generator (w2_10003a10.cpp).
extern "C" void __fastcall sub_10003b50(void* pDraw, void* pVert,
                                        void* pOut);


// One model piece (w2_10008da0.cpp's ModelPiece8e10): its first 0x24
// bytes are its base vertex run, +0x38/+0x3c the further runs.
struct ModelVertSet8a40
{
	uint32      m_n00;                                      // +0x00
	void*       m_pVerts04;                                 // +0x04
	uint32      m_nVerts08;                                 // +0x08
	uint8       m_pad_0c[0x24 - 0x0c];                      // +0x0c
};

struct ModelPiece8a40 : public ModelVertSet8a40
{
	uint32      m_nTexture24;                               // +0x24
	uint8       m_pad_28[0x2c - 0x28];                      // +0x28
	float       m_fTexScaleU2c;                             // +0x2c
	float       m_fTexScaleV30;                             // +0x30
	uint8       m_pad_34[0x38 - 0x34];                      // +0x34
	ModelVertSet8a40* m_pSets38;                            // +0x38
	uint32      m_nSets3c;                                  // +0x3c

	ModelVertSet8a40* GetSet(uint32 i)
	{
		ModelVertSet8a40* pSet;
		if (i == 0)
			pSet = this;
		else if (i - 1 >= m_nSets3c)
			pSet = 0;
		else
			pSet = &m_pSets38[i - 1];
		return pSet;
	}
};

// The pieces are a CMoArray<ModelPiece*> (vptr +0x34, m_pArray +0x38,
// m_nElements +0x3c).
struct PieceArray8a40
{
	void*				m_pVtbl;
	ModelPiece8a40**	m_pArray;
	uint32				m_nElements;
};
// The model's pieces reached through the model's own
// inline accessors NumPieces() / GetPiece(i), as LithTech's Model class
// spells them (the array's own GetSize()/operator[] inside them build the
// same bytes, but icl then also emits those two out of line).  With
// them 0x10008a44 is byte-identical (134 -> 0): icl then keeps the model
// pointer in %edx across the loop and re-reads the count at every test,
// as shipped; the array members read directly (or the array's accessors
// called on m_pModel00->m_Pieces) spill one or the other (24..421 off).
struct D3DModel8a40
{
	uint8       m_pad_00[0x34];                             // +0x00
	PieceArray8a40 m_Pieces;                                // +0x34

	uint32          NumPieces()           { return m_Pieces.m_nElements; }
	ModelPiece8a40* GetPiece(uint32 i)    { return m_Pieces.m_pArray[i]; }
};

struct D3DObject8a40
{
	uint8       m_pad_000[0x88];                            // +0x000
	uint32      m_nFlags88;                                 // +0x088
	uint8       m_pad_08c[0x204 - 0x08c];                   // +0x08c
	uint32      m_nHiddenPieces204;                         // +0x204
};

// The transformed model vertex (0x20 bytes).
struct RVert8a40
{
	uint8       m_Bytes[0x20];
};


class CD3DModelDraw
{
public:
	typedef LTBOOL (CD3DModelDraw::*PFNSETUP)(ModelVertSet* pSet,
	                                          RVert* pVerts);

	LTBOOL      sub_10006840(ModelVertSet* pSet, RVert* pVerts); // 0x10006840
	LTBOOL      sub_10007620(ModelVertSet* pSet, RVert* pVerts); // 0x10007620
	LTBOOL      sub_10008340(ModelVertSet* pSet, RVert* pVerts); // 0x10008340
	LTBOOL      sub_10008680(ModelVertSet* pSet, RVert* pVerts); // 0x10008680
	LTBOOL      sub_100086f0(ModelVertSet* pSet, RVert* pVerts); // 0x100086f0
	void        sub_10008a40(PFNSETUP pfnSetup, unsigned long nValue); // 0x10008a40
	void        sub_10008da0(unsigned long nValue);         // 0x10008da0
	void        sub_10047230(uint32 nIndex);                // 0x10047230
	void        sub_10047b60(uint32* pnValue);              // 0x10047b60

	D3DModel8a40*  m_pModel00;                              // +0x000
	D3DObject8a40* m_pObject04;                             // +0x004
	uint8       m_pad_008[0x30c - 0x008];                   // +0x008
	uint32      m_nTexture30c;                              // +0x30c
	uint8       m_pad_310[0x428 - 0x310];                   // +0x310
	uint32      m_nFormat428;                               // +0x428
	void*       m_pfnTexCoord42c;                           // +0x42c
	uint32      m_n430;                                     // +0x430
	void*       m_pfnUV434;                                 // +0x434
	uint32      m_nStride438;                               // +0x438
	void*       m_pfnCopy43c;                               // +0x43c
	void*       m_pfnClip440;                               // +0x440
	uint32      m_nFVF444;                                  // +0x444
	I100039d0*  m_pBank448;                                 // +0x448
	uint32      m_n44c;                                     // +0x44c
	uint8       m_pad_450[0x46c - 0x450];                   // +0x450
	float       m_fTexScaleU46c;                            // +0x46c
	float       m_fTexScaleV470;                            // +0x470
	uint8       m_pad_474[0x62c - 0x474];                   // +0x474
	RVert8a40*  m_pVerts62c;                                // +0x62c
	uint8       m_pad_630[0x6ac - 0x630];                   // +0x630
	uint32      m_n6ac;                                     // +0x6ac
	uint32      m_n6b0;                                     // +0x6b0
	uint32      m_bCull6b4;                                 // +0x6b4
};


// The model-cache override (+0x6ac): draw through the cache at 0x10091d80,
// untransformed -- the rhw bit (4) of the format swapped for XYZ (2) and
// four bytes off the stride.  The name matters to icl: a call whose callee
// is named with Err/Fail/Invalid/Warn/... is predicted cold and its block
// laid out past the `ret`, as the shipped 0x10008d50 is, even inlined.
inline void UseCacheOverride(CD3DModelDraw* pDraw, uint32 nFVF,
                             uint32 nStride)
{
	pDraw->m_pBank448 = &g_10091d80;
	g_10091d80.m_nFVF58 = (nFVF & ~4) | 2;
	g_10091d80.m_nStride54 = nStride - 4;
}

// The chosen bank no longer has ModelVBCount buffers (its second test,
// laid out cold at 0x10008d7f for the same reason).
inline int BuffersInvalid(I100039d0* pBank)
{
	return pBank->m_nBuffers24 != g_1008abf0.m_nValue00;
}


// ======================================================================= //
//  0x10008a40 -- draw every visible piece through pfnSetup.
// ======================================================================= //
void CD3DModelDraw::sub_10008a40(PFNSETUP pfnSetup, unsigned long nValue)
{
	m_n430 = nValue;

	if (m_nFormat428 == 1)
	{
		m_nStride438 = 0x20;
		m_pfnCopy43c = (void*)sub_10003bb0;
		m_pfnClip440 = (void*)sub_10003c40;
		m_nFVF444 = 0x1c4;
		if (m_n6b0)
			m_pBank448 = &g_1008acd4;
		else
			m_pBank448 = &g_1008ad4c;
	}
	else
	{
		m_nStride438 = 0x28;
		m_pfnCopy43c = (void*)sub_10003bf0;
		m_pfnClip440 = (void*)sub_100051a0;
		m_nFVF444 = 0x2c4;
		if (m_n6b0)
			m_pBank448 = &g_1008ac98;
		else
			m_pBank448 = &g_1008ad10;
	}

	if (m_pBank448->m_nSize20 != g_1008ac10.m_nValue00
	 || BuffersInvalid(m_pBank448))
	{
		sub_100039d0();
		sub_10009ae0();
	}

	if (m_n6b0)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x16, 3);

	if (m_n6ac)
		UseCacheOverride(this, m_nFVF444, m_nStride438);

	m_bCull6b4 = (m_n6ac != 1 && !m_n6b0);
	m_nTexture30c = (uint32)-1;

	RVert8a40* pOut = m_pVerts62c;
	for (uint32 i = 0; i < m_pModel00->NumPieces(); i++)
	{
		ModelPiece8a40* pPiece = m_pModel00->GetPiece(i);
		ModelVertSet8a40* pSet = pPiece->GetSet(m_n44c);
		if (!pSet)
			continue;
		if ((1 << i) & m_pObject04->m_nHiddenPieces204)
			continue;

		if (pPiece->m_nTexture24 != m_nTexture30c)
		{
			sub_10047230(pPiece->m_nTexture24);
			sub_10047b60(&m_n430);
		}

		m_fTexScaleU46c = pPiece->m_fTexScaleU2c;
		m_fTexScaleV470 = pPiece->m_fTexScaleV30;
		if (g_f1008ac54 != 0.0f)
			m_fTexScaleU46c = g_f1008ac54;
		if (g_f1008ac34 != 0.0f)
			m_fTexScaleV470 = g_f1008ac34;

		if (m_fTexScaleV470 != 0.0f && m_pfnTexCoord42c == (void*)sub_10003b50)
			sub_10039420(m_fTexScaleU46c);

		(this->*pfnSetup)((ModelVertSet*)pSet, (RVert*)pOut);
		pOut += pSet->m_nVerts08;
	}

	if (m_n6b0)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x16, 1);

	if (g_CV_ModelProfile)
		sub_10021d10("DrawTris: %d", 0);
}


// Mode 2 (+0x6ac) draws the model straight out of the cache override, so
// its setup routine only draws.  Named for icl as UseCacheOverride is: the
// shipped 0x10008da0 lays this arm out past its `ret` (0x10008e08).
inline CD3DModelDraw::PFNSETUP CacheOverrideSetup()
{
	return &CD3DModelDraw::sub_10008680;
}


// ======================================================================= //
//  0x10008da0 -- pick the vertex-setup routine and run it.
// ======================================================================= //
void CD3DModelDraw::sub_10008da0(unsigned long nValue)
{
	PFNSETUP pfnSetup;

	if (m_n6ac == 2)
		pfnSetup = CacheOverrideSetup();
	else if (m_n6ac == 1 || m_n6b0)
		pfnSetup = &CD3DModelDraw::sub_100086f0;
	else if (m_pObject04->m_nFlags88 & 0x40)
		pfnSetup = &CD3DModelDraw::sub_10006840;
	else if (g_1008b874 & 0x3f)
		pfnSetup = &CD3DModelDraw::sub_10007620;
	else
		pfnSetup = &CD3DModelDraw::sub_10008340;

	sub_10008a40(pfnSetup, nValue);
}
