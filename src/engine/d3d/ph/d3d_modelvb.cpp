// *********************************************************************** //
//
//  ph/d3d_modelvb.cpp -- Primal Hunt's d3d.ren, TU01 (with what
//  docs/D3D_OBJECTS.md maps as TU02 and TU03): 0x10001000..0x10007924,
//  /O2.
//
//  The model vertex-buffer renderer: the model console variables, the
//  four vertex-buffer banks the model drawer fills (two vertex formats,
//  each pre-transformed and TnL), their (re)allocation, the texture
//  coordinate generators, the model polygon clippers and the drawer's
//  vertex-setup routines.
//
//  One object, not three: "TU02" (0x10004620, 0x1000463b) is the tail of
//  0x100045a0 (its `je 0x10004620` / `je 0x1000463b`), so 0x100045a0
//  runs to 0x10004656 and is padded with 0x90 to 0x10004660, where "TU03"
//  starts. "TU03" is this file's template instantiations, which cl 8966
//  emits after the last function: the banks' virtual functions and their
//  scalar deleting destructor (CModelVB is a class template, so its four
//  vtables -- two vertex formats, each pre-transformed and TnL -- share
//  the folded slots 0x10006140..0x10006230), and the clippers' plane
//  functions (0x100063e0..0x10007924, with function-static arrays in
//  COMDAT .bss). Between them the out-of-line copy of ProjectVertPushed
//  (0x100062e0), which DrawTLVertsReallyClose's clip loop calls. Still
//  unwritten: 0x10004660 (4241 bytes, the drawer's vertex transform and
//  lighting, called through 0x10005700) and 0x10005700 (2612 bytes).
//
//  /O2 /Ob2: the global objects' initialisers are single functions (with
//  /Ob1 each is a `jmp` stub over a body, and the banks' a call pair).
//  /Ob2's inlining budget is visible in the drawers: retail expands every
//  helper of DrawTLVertsClipped but calls DrawTLVertsReallyClose's last
//  ProjectVertPushed, so the projections and the classification are
//  shaped (inline function or macro) to land the same way.
//
//  Near, and why:
//    * 0x10001420 (d3d_GenUVEnvMap), 0x100036d0, 0x10002bc0: the order of
//      the terms in the transform sums, which cl 8966 picks itself (every
//      source order of the terms was measured, with no effect).
//    * 0x10002050 (DrawTLVertsReallyClose): retail calls only its clip
//      loop's ProjectVertPushed out of line; here the budget runs out
//      earlier (ProjFacing and the flush helpers too). 2655 against 2915.
//    * 0x10004270 (DrawVerts): the store of m_iCurTexture and the cache's
//      vertex size are scheduled differently (size-exact).
//    * 0x10006900, 0x100073b0, 0x10007670: ClipEdgeBottom's numerator
//      operand order inlined into ClipBottom, and LerpClipVert's last byte
//      lerp's load order, both cl 8966's choice (size-exact).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <d3d.h>
#include "../../LT2/lithshared/stdlith/dynarray.h"


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                                // 0x10011f36

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pszName;                                  // +0x14
	void*       m_hVar;                                     // +0x18
	CConVar*    m_pNext;                                    // +0x1c
};


// The vertex-buffer controller (TU43, 0x1003b3e0..): a ring of
// m_nBuffers Direct3D vertex buffers of m_nVerts vertices each.
class CVertexBufferController
{
public:
	CVertexBufferController();                              // 0x1003b3e0
	virtual ~CVertexBufferController();                     // 0x1003b436

	virtual long    DrawPrimitive(void* pDevice, uint32 nPrimType,
	                              uint32 nVerts);           // 0x1003b45b
	virtual long    DrawIndexedPrimitive(void* pDevice, uint32 nPrimType,
	                                     uint32 nVerts, uint16* pIndices,
	                                     uint32 nIndices);  // 0x1003b4dc
	virtual void    Term();                                 // 0x1003b565
	virtual LTBOOL  Init(IDirect3D7* pD3D, uint32 nVerts, uint32 nBuffers,
	                     LTBOOL bTnL, LTBOOL bSysMem);      // 0x1003b59d
	virtual void*   Lock();                                 // 0x1003b611
	virtual uint32  GetVertexSize() = 0;
	virtual LTBOOL  CreateBuffers(IDirect3D7* pD3D) = 0;

	// The last slot of the locked buffer.
	uint8* GetLastVert()
	{
		uint32 nLeft = m_nVerts - m_nUsed - 1;
		uint8* pBase = (uint8*)Lock();
		return pBase + nLeft * GetVertexSize();
	}

	// The vertices written between the locked buffer's start and pEnd.
	uint32 GetVertCount(uint8* pEnd)
	{
		return (pEnd - (uint8*)Lock()) / GetVertexSize();
	}

	void            NextBuffer();                           // 0x1003b4cc
	void            Flush();                                // 0x1003b557
	void            ReleaseBuffers();                       // 0x1003b66f

	void*       m_pBuffersVtbl;                             // +0x04 CMoArray
	LPDIRECT3DVERTEXBUFFER7* m_pBuffers;                    // +0x08
	uint8       m_pad_0c[0x18 - 0x0c];                      // +0x0c
	uint32      m_iBuffer;                                  // +0x18
	uint32      m_nUsed;                                    // +0x1c
	uint32      m_nVerts;                                   // +0x20
	uint32      m_nBuffers;                                 // +0x24
	LTBOOL      m_bTnL;                                     // +0x28
	LTBOOL      m_bSysMem;                                  // +0x2c
	LTBOOL      m_bInit;                                    // +0x30
	LTBOOL      m_bLocked;                                  // +0x34
	void*       m_pLocked;                                  // +0x38
};

// The model vertex cache (TU43's 0x100995a8): a controller that keeps
// whole models' vertices between frames.
class CVertexBufferCache : public CVertexBufferController
{
public:
	virtual uint32  GetVertexSize();
	virtual LTBOOL  CreateBuffers(IDirect3D7* pD3D);

	uint8       m_pad_3c[0x54 - 0x3c];                      // +0x3c
	uint32      m_nVertSize;                                // +0x54
	uint32      m_nFVF;                                     // +0x58
	uint8       m_pad_5c[0x60 - 0x5c];                      // +0x5c
	uint32      m_nCacheDelay;                              // +0x60
};

// The model banks: one and two texture stage vertices (the vertex format
// TVert names), each either pre-transformed or transformed by the device
// (bTnL). A class template, so its functions are instantiated after this
// file's last function.
struct ModelVBVert
{
	enum { FVF = D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 };
	uint32      m_Dwords[8];
};

struct ModelVBVert2
{
	enum { FVF = D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2 };
	uint32      m_Dwords[10];
};

template<class TVert, int bTnL>
class CModelVB : public CVertexBufferController
{
public:
	CModelVB() { m_bTnL = bTnL; }

	virtual uint32 GetVertexSize()
	{
		return m_bTnL ? sizeof(TVert) - sizeof(float) : sizeof(TVert);
	}

	// 0x10006180 (one texture stage), 0x10006230 (two): the pre-transformed
	// and TnL banks' copies fold.
	virtual LTBOOL CreateBuffers(IDirect3D7* pD3D)
	{
		D3DVERTEXBUFFERDESC desc;
		LPDIRECT3DVERTEXBUFFER7 pVB;
		uint32 i;
		LTBOOL bOK;

		memset(&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(desc);
		desc.dwCaps = D3DVBCAPS_WRITEONLY;
		desc.dwFVF = m_bTnL ? (D3DFVF_XYZ | TVert::FVF) : (D3DFVF_XYZRHW | TVert::FVF);
		if (!m_bSysMem)
			desc.dwCaps |= D3DVBCAPS_SYSTEMMEMORY;
		desc.dwNumVertices = m_nVerts;

		bOK = LTTRUE;
		for (i = 0; i < m_nBuffers; i++)
		{
			if (!bOK)
				break;

			if (pD3D->CreateVertexBuffer(&desc, &pVB, 0) >= 0)
				m_pBuffers[i] = pVB;
			else
				bOK = LTFALSE;
		}

		if (!bOK)
			ReleaseBuffers();

		return bOK;
	}
};


extern IDirect3D7*          g_pD3D;                         // 0x100638cc
extern long                 g_CV_TnLRast;                   // 0x1005df2c
extern CVertexBufferCache   g_ModelVBCache;                 // 0x100995a8

// The half-texel offsets the texture coordinate generators add.
extern float                g_fTexelOffsetU;                // 0x10052b70
extern float                g_fTexelOffsetV;                // 0x10052b74


// ----------------------------------------------------------------------- //
//  The file's globals, in construction order (0x10001030..0x10001229).
// ----------------------------------------------------------------------- //

CConVar         g_CV_ModelMinTri("ModelMinTri", 0.2f, LTNULL, LTNULL);             // 0x10051610
CModelVB<ModelVBVert, LTFALSE>  g_ModelVB;                                         // 0x10051650
CModelVB<ModelVBVert2, LTFALSE> g_ModelVB2;                                        // 0x10051ab0
CModelVB<ModelVBVert, LTTRUE>   g_ModelVBTnL;                                      // 0x10052b78
CModelVB<ModelVBVert2, LTTRUE>  g_ModelVB2TnL;                                     // 0x10051b10
CConVar         g_CV_ModelDetailTextureScale("ModelDetailTextureScale", 3.0f, LTNULL, LTNULL); // 0x100515f0
CConVar         g_CV_SpecularPowerTest("SpecularPowerTest", 0.0f, LTNULL, LTNULL); // 0x10052b50
CConVar         g_CV_SpecularScaleTest("SpecularScaleTest", 0.0f, LTNULL, LTNULL); // 0x10051af0
CConVar         g_CV_ModelVBSize("ModelVBSize", 126.0f, LTNULL, LTNULL);           // 0x10051630
CConVar         g_CV_ModelVBCount("ModelVBCount", 1.0f, LTNULL, LTNULL);           // 0x10052bb8
CConVar         g_CV_ModelVBCache("ModelVBCache", 128.0f, LTNULL, LTNULL);         // 0x10051690
CConVar         g_CV_ModelVBCacheDelay("ModelVBCacheDelay", 4.0f, LTNULL, LTNULL); // 0x100515d0


// 0x10001230 -- (re)allocate the model vertex buffers: at least one
// buffer of at least one whole triangle.
void d3d_InitModelVBs()
{
	g_CV_ModelVBSize.m_nValue = LTMAX(g_CV_ModelVBSize.m_nValue, 1);
	g_CV_ModelVBCount.m_nValue = LTMAX(g_CV_ModelVBCount.m_nValue, 1);
	g_CV_ModelVBSize.m_nValue = ((g_CV_ModelVBSize.m_nValue + 2) / 3) * 3;

	g_ModelVB.Init(g_pD3D, g_CV_ModelVBSize.m_nValue, g_CV_ModelVBCount.m_nValue,
		LTFALSE, g_CV_TnLRast);
	g_ModelVB2.Init(g_pD3D, g_CV_ModelVBSize.m_nValue, g_CV_ModelVBCount.m_nValue,
		LTFALSE, g_CV_TnLRast);
	g_ModelVBTnL.Init(g_pD3D, g_CV_ModelVBSize.m_nValue, g_CV_ModelVBCount.m_nValue,
		LTTRUE, g_CV_TnLRast);
	g_ModelVB2TnL.Init(g_pD3D, g_CV_ModelVBSize.m_nValue, g_CV_ModelVBCount.m_nValue,
		LTTRUE, g_CV_TnLRast);

	CVertexBufferController* pCache = &g_ModelVBCache;
	pCache->Init(g_pD3D, g_CV_ModelVBSize.m_nValue, g_CV_ModelVBCache.m_nValue,
		LTTRUE, g_CV_TnLRast);
	g_ModelVBCache.m_nCacheDelay = g_CV_ModelVBCacheDelay.m_nValue;
}

// 0x10001340 -- free them. Only the pre-transformed banks and the cache.
void d3d_TermModelVBs()
{
	g_ModelVB.Term();
	g_ModelVB2.Term();

	CVertexBufferController* pCache = &g_ModelVBCache;
	pCache->Term();
}


// ----------------------------------------------------------------------- //
//  Texture coordinate generators, called through the drawer's +0x434
//  (fastcall: the bank vertex, the model vertex, the triangle corner's uv).
// ----------------------------------------------------------------------- //

// A bank vertex's texture coordinates: one pair, or two.
struct UVVert
{
	uint8       m_pad_00[0x18];                             // +0x00
	float       m_fU;                                       // +0x18
	float       m_fV;                                       // +0x1c
	float       m_fU2;                                      // +0x20
	float       m_fV2;                                      // +0x24

	void SetUV(float fU, float fV) { m_fU = fU; m_fV = fV; }
};

// 0x10001370 -- the corner's uv.
void __fastcall d3d_GenUV(UVVert* pOut, UVVert* pSrc, float* pUV)
{
	pOut->SetUV(pUV[0] + g_fTexelOffsetU, pUV[1] + g_fTexelOffsetV);
}

// 0x10001390 -- the corner's uv, and the detail texture's: the same
// scaled by ModelDetailTextureScale.
void __fastcall d3d_GenUVDetail(UVVert* pOut, UVVert* pSrc, float* pUV)
{
	pOut->m_fU = pUV[0] + g_fTexelOffsetU;
	pOut->m_fV = pUV[1] + g_fTexelOffsetV;
	pOut->m_fU2 = g_CV_ModelDetailTextureScale.m_fValue * pOut->m_fU;
	pOut->m_fV2 = g_CV_ModelDetailTextureScale.m_fValue * pOut->m_fV;
}

// 0x100013d0 -- the model vertex's uv.
void __fastcall d3d_GenUVVert(UVVert* pOut, UVVert* pSrc, float* pUV)
{
	pOut->SetUV(pSrc->m_fU, pSrc->m_fV);
}

// 0x100013e0 -- the corner's uv, and the model vertex's as the second.
void __fastcall d3d_GenUVVert2(UVVert* pOut, UVVert* pSrc, float* pUV)
{
	pOut->m_fU = pUV[0] + g_fTexelOffsetU;
	pOut->m_fV = pUV[1] + g_fTexelOffsetV;
	pOut->m_fU2 = pSrc->m_fU;
	pOut->m_fV2 = pSrc->m_fV;
}


// The model drawer, CD3DModelDraw: only what this file reads.

// A transformed model vertex (0x20 bytes): the drawer's work array.
struct ModelVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	uint32      m_nColor;                                   // +0x10
	LTVector    m_vNormal;                                  // +0x14
};

// One model triangle (0x20 bytes): three vertex indices and a uv pair
// per corner.
struct ModelTri
{
	uint16      m_Indices[3];                               // +0x00
	uint16      m_pad_06;                                   // +0x06
	float       m_UV[3][2];                                 // +0x08
};

// One vertex run of a piece.
struct ModelVertSet
{
	uint8       m_pad_00[0x08];                             // +0x00
	uint32      m_nVerts;                                   // +0x08
	uint8       m_pad_0c[0x14 - 0x0c];                      // +0x0c
	ModelTri*   m_pTris;                                    // +0x14
	uint32      m_nTris;                                    // +0x18
	uint8       m_pad_1c[0x24 - 0x1c];                      // +0x1c
};

// A bank vertex, pre-transformed (FVF 0x1c4 / 0x2c4): the first 0x18
// bytes; the uv generator writes the rest.
struct TLVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	uint32      m_nDiffuse;                                 // +0x10
	uint32      m_nSpecular;                                // +0x14
};

// A bank vertex the device transforms (FVF 0x1c2 / 0x2c2): no rhw.
struct LVert
{
	LTVector    m_vPos;                                     // +0x00
	uint32      m_nDiffuse;                                 // +0x0c
	uint32      m_nSpecular;                                // +0x10
};

// A model piece: its full-detail vertex run, then its lower LODs.
struct ModelPiece : public ModelVertSet
{
	uint32      m_iTexture;                                 // +0x24
	uint8       m_pad_28[0x2c - 0x28];                      // +0x28
	float       m_fSpecularPower;                           // +0x2c
	float       m_fSpecularScale;                           // +0x30
	uint8       m_pad_34[0x38 - 0x34];                      // +0x34
	ModelVertSet* m_LODs;                                   // +0x38
	uint32      m_nLODs;                                    // +0x3c

	ModelVertSet* GetLOD(uint32 iLOD)
	{
		if (iLOD == 0)
			return this;
		iLOD--;
		if (iLOD >= m_nLODs)
			return LTNULL;
		return &m_LODs[iLOD];
	}
};

struct Model
{
	uint8       m_pad_00[0x34];                             // +0x00
	CMoArray<ModelPiece*> m_Pieces;                         // +0x34

	uint32      NumPieces()             { return m_Pieces.GetSize(); }
	ModelPiece* GetPiece(uint32 i)      { return m_Pieces[i]; }
};

// The render-state list a skin carries (TU21, d3d_statelist.cpp).
class CRenderStateList
{
public:
	void        Restore();                                  // 0x10022a86
	void        Apply(CRenderStateList* pList, uint32 iStage); // 0x10022a97
};

extern CRenderStateList g_SavedStates;                      // 0x10069728

struct RenderTexture
{
	uint8       m_pad_00[0x34];                             // +0x00
	CRenderStateList* m_pRenderStates;                      // +0x34
};

struct ModelInstance
{
	uint8       m_pad_000[0x88];                            // +0x000
	uint32      m_Flags;                                    // +0x088
	uint8       m_pad_08c[0x1c0 - 0x08c];                   // +0x08c
	RenderTexture* m_pSkins[0x28];                          // +0x1c0
	uint32      m_HiddenPieces;                             // +0x260
};

void d3d_SetGammaTexture(float gamma);                      // 0x1001f5f0

class CD3DModelDraw;
typedef LTBOOL (CD3DModelDraw::*DrawVertsFn)(ModelVertSet* pSet, ModelVert* pVerts);

// The drawer's per-vertex hooks: the uv generator (the bank vertex, the
// model vertex, the corner's uv pair), the bank-vertex copy and the
// polygon clipper.
typedef void (__fastcall *GenUVFn)(void* pOut, ModelVert* pVert, float* pUV);
typedef void (__fastcall *CopyVertFn)(void* pDst, void* pSrc);
typedef LTBOOL (__fastcall *ClipPolyFn)(uint32 nClipFlags, void** ppVerts, int32* pnVerts);
typedef void (__fastcall *GenSpecularFn)(CD3DModelDraw* pDraw, ModelVert* pVert, struct UVVert* pOut);

extern uint32       g_ClipFlags;                            // 0x1005bd0c
extern int32        g_nModelTriangles;                      // 0x1005bd04
extern LTBOOL       g_bMirrorView;                          // 0x1005bc5c
extern LPDIRECT3DDEVICE7 g_pD3DDevice;                      // 0x100638c8

class CD3DModelDraw
{
public:
	LTBOOL      DrawTLVertsReallyClose(ModelVertSet* pSet, ModelVert* pVerts); // 0x10002050
	LTBOOL      DrawTLVertsClipped(ModelVertSet* pSet, ModelVert* pVerts);     // 0x10002bc0
	LTBOOL      DrawTLVertsNoClip(ModelVertSet* pSet, ModelVert* pVerts);    // 0x100036d0
	LTBOOL      DrawTLVerts(ModelVertSet* pSet, ModelVert* pVerts);            // 0x10003b60
	LTBOOL      DrawTnLVerts(ModelVertSet* pSet, ModelVert* pVerts);           // 0x10003e00
	LTBOOL      DrawCachedVerts(ModelVertSet* pSet, ModelVert* pVerts);        // 0x10004230
	void        DrawVerts(DrawVertsFn fn, DrawVertsFn fn2,
	                      uint32 nValue);                   // 0x10004270
	void        DrawPiece(uint32 nValue);                   // 0x100045a0
	void        SetupTextureStages(uint32* pnValue);        // 0x10025193

	// Send the vertices through the model cache, which keeps them in the
	// bank's format, less the rhw: the device transforms them.
	void        UseModelVBCache(uint32 nFVF, uint32 nVertSize)
	{
		m_pVB = &g_ModelVBCache;
		g_ModelVBCache.m_nFVF = (nFVF & ~D3DFVF_XYZRHW) | D3DFVF_XYZ;
		g_ModelVBCache.m_nVertSize = nVertSize - sizeof(float);
	}
	void        SetupTexture(uint32 iTexture);              // 0x10025269

	// Draw the bank's vertices up to pOut.
	void        FlushVerts(uint8* pOut)
	{
		uint32 nVerts = m_pVB->GetVertCount(pOut);
		g_nModelTriangles += nVerts / 3;
		m_pVB->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLELIST, nVerts);
	}

	// Draw the full bank (the next triangle does not fit).
	void        FlushFullVB()
	{
		g_nModelTriangles += (m_pVB->m_nVerts - m_pVB->m_nUsed) / 3;
		m_pVB->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLELIST,
			m_pVB->m_nVerts - m_pVB->m_nUsed);
	}

	Model*      m_pModel;                                   // +0x000
	ModelInstance* m_pInstance;                             // +0x004
	uint8       m_pad_008[0x34 - 0x8];                      // +0x008
	uint32      m_iStage;                                   // +0x034
	uint8       m_pad_038[0x2c4 - 0x38];                    // +0x038
	uint8       m_bPieceHidden[0x100];                      // +0x2c4
	uint8       m_bPieceClipped[0x100];                     // +0x3c4
	uint8       m_pad_4c4[0x4cc - 0x4c4];                   // +0x4c4
	uint32      m_iCurTexture;                              // +0x4cc
	uint8       m_pad_4d0[0x590 - 0x4d0];                   // +0x4d0
	LTMatrix    m_mEnvMap;                                  // +0x590
	uint8       m_pad_5d0[0x5e8 - 0x5d0];                   // +0x5d0
	uint32      m_nVertFormat;                              // +0x5e8
	GenSpecularFn m_pfnGenSpecular;                         // +0x5ec
	uint32      m_nValue;                                   // +0x5f0
	GenUVFn     m_pfnGenUV;                                 // +0x5f4
	uint32      m_nVertSize;                                // +0x5f8
	CopyVertFn  m_pfnCopyVert;                              // +0x5fc
	ClipPolyFn  m_pfnClipPoly;                              // +0x600
	uint32      m_nFVF;                                     // +0x604
	CVertexBufferController* m_pVB;                         // +0x608
	uint32      m_iLOD;                                     // +0x60c
	uint8       m_pad_610[0x61c - 0x610];                   // +0x610
	float       m_fEnvOffsetU;                              // +0x61c
	float       m_fEnvOffsetV;                              // +0x620
	float       m_fEnvScaleU;                               // +0x624
	float       m_fEnvScaleV;                               // +0x628
	float       m_fSpecularPower;                           // +0x62c
	float       m_fSpecularScale;                           // +0x630
	uint8       m_pad_634[0x640 - 0x634];                   // +0x634
	uint32      m_nSpecular;                                // +0x640
	uint8       m_pad_644[0x82c - 0x644];                   // +0x644
	ModelVert*  m_pVerts;                                   // +0x82c
	uint8       m_pad_830[0x874 - 0x830];                   // +0x830
	LTVector    m_vSpecularDir;                             // +0x874
	uint8       m_pad_880[0x8ac - 0x880];                   // +0x880
	uint32      m_nVertMode;                                // +0x8ac
	LTBOOL      m_bTnL;                                     // +0x8b0
	LTBOOL      m_bBackfaceCull;                            // +0x8b4
};

// 0x10001410 -- no texture coordinates.
void __fastcall d3d_GenUVNone(CD3DModelDraw* pDraw, ModelVert* pVert,
                              UVVert* pOut)
{
}

// 0x10001420 -- environment map: the normal through the environment
// matrix, scaled and offset. NEAR: the terms of each row's sum (retail
// v: y, x, z; u: y, z, x), which no spelling tried (direct, Dot,
// Apply3x3, MatVMul_3x3, temporaries) reproduces.
void __fastcall d3d_GenUVEnvMap(CD3DModelDraw* pDraw, ModelVert* pVert,
                                UVVert* pOut)
{
	pOut->SetUV(
		(pVert->m_vNormal.x * pDraw->m_mEnvMap.m[0][0]
		 + pVert->m_vNormal.y * pDraw->m_mEnvMap.m[0][1]
		 + pVert->m_vNormal.z * pDraw->m_mEnvMap.m[0][2])
			* pDraw->m_fEnvScaleU + pDraw->m_fEnvOffsetU,
		(pVert->m_vNormal.x * pDraw->m_mEnvMap.m[1][0]
		 + pVert->m_vNormal.y * pDraw->m_mEnvMap.m[1][1]
		 + pVert->m_vNormal.z * pDraw->m_mEnvMap.m[1][2])
			* pDraw->m_fEnvScaleV + pDraw->m_fEnvOffsetV);
}

// 0x10001490 -- specular: the normal against the specular direction.
void __fastcall d3d_GenUVSpecular(CD3DModelDraw* pDraw, ModelVert* pVert,
                                  UVVert* pOut)
{
	LTVector vDir = pDraw->m_vSpecularDir;
	float fDot = pVert->m_vNormal.Dot(vDir);
	if (fDot > 0.0f)
		pOut->m_fU = pOut->m_fV = fDot * pDraw->m_fSpecularScale;
	else
		pOut->m_fU = pOut->m_fV = 0.0f;
}


// The bank vertices: one texture stage (FVF 0x1c4), two (0x2c4).
struct BankVert
{
	uint32      m_Dwords[8];
};

struct BankVert2
{
	uint32      m_Dwords[10];
};

// 0x10001510 -- copy a bank vertex (the drawer's +0x43c).
void __fastcall d3d_CopyVert(BankVert* pDst, BankVert* pSrc)
{
	*pDst = *pSrc;
}

// 0x10001520 -- the same, two texture stages.
void __fastcall d3d_CopyVert2(BankVert2* pDst, BankVert2* pSrc)
{
	*pDst = *pSrc;
}

// ======================================================================= //
//  The polygon clippers (the drawer's +0x440), 0x10001530..0x1000204a, and
//  their plane functions (ph/d3d_clip.h). ClipTop..ClipFar are templates
//  over the vertex, so they follow this file's last function
//  (0x100063e0..0x10007924).
// ======================================================================= //

#include "d3d_clip.h"

extern LTMatrix g_mModelTransform;                          // 0x1005b92c
extern float    g_fReallyClosePushZ;                        // 0x1005b74c

// 0x10001530 -- the drawer's clipper for the one-stage vertex. The driver
// is an inline under it: written in the function itself, cl 8966 inlines
// ClipTop as well (one inline level more), where retail calls it.
LTBOOL __fastcall d3d_ClipPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts)
{
	return ClipPoly(nClipFlags, ppVerts, pnVerts);
}

// 0x10001b30 -- the same, two texture stages.
LTBOOL __fastcall d3d_ClipPoly2(uint32 nClipFlags, ClipVert2** ppVerts, int32* pnVerts)
{
	return ClipPoly(nClipFlags, ppVerts, pnVerts);
}

// The screen-space facing of a triangle, sign-flipped for a mirrored view.
inline float TriFacing(ModelVert* pV0, ModelVert* pV1, ModelVert* pV2)
{
	float fCross = (pV1->m_vPos.x - pV0->m_vPos.x) * (pV2->m_vPos.y - pV0->m_vPos.y)
	             - (pV2->m_vPos.x - pV0->m_vPos.x) * (pV1->m_vPos.y - pV0->m_vPos.y);
	if (g_bMirrorView)
		fCross = -fCross;
	return fCross;
}

// The planes ClassifyTri tests, in its order, and the planes still to
// clip against once a triangle straddles one of them.
// The most vertices a clipped model triangle is fanned from.
#define MODEL_CLIP_VERTS    32

#define CPLANE_NEAR     0x01
#define CPLANE_FAR      0x02
#define CPLANE_LEFT     0x04
#define CPLANE_TOP      0x08
#define CPLANE_RIGHT    0x10
#define CPLANE_BOTTOM   0x20

#define CLASSIFY_PLANE(PLANE, INSIDE, REST) \
	if (nFlags & PLANE) \
	{ \
		n = INSIDE(pV0) + INSIDE(pV1) + INSIDE(pV2); \
		if (n == 0) \
			return 1; \
		if (n != 3) \
		{ \
			*pnClip = nFlags & (REST); \
			return 2; \
		} \
	}

#define INSIDE_LEFT(v)      (-(v)->m_vPos.z < (v)->m_vPos.x)
#define INSIDE_RIGHT(v)     ((v)->m_vPos.x < (v)->m_vPos.z)
#define INSIDE_TOP(v)       ((v)->m_vPos.y < (v)->m_vPos.z)
#define INSIDE_BOTTOM(v)    (-(v)->m_vPos.z < (v)->m_vPos.y)
#define INSIDE_NEAR(v)      ((v)->m_vPos.z >= g_fNearZ)
#define INSIDE_FAR(v)       ((v)->m_vPos.z <= g_fFarZ)

// Classify a view-space triangle against the planes in nFlags: 0 wholly
// inside, 1 wholly outside one of them, 2 straddling one, *pnClip then
// holding that plane and those still to test.
inline uint32 ClassifyTri(ModelVert* pV0, ModelVert* pV1, ModelVert* pV2,
                          uint32 nFlags, uint32* pnClip)
{
	uint32 n;

	CLASSIFY_PLANE(CPLANE_LEFT, INSIDE_LEFT,
		CPLANE_LEFT | CPLANE_RIGHT | CPLANE_TOP | CPLANE_BOTTOM | CPLANE_NEAR | CPLANE_FAR)
	CLASSIFY_PLANE(CPLANE_RIGHT, INSIDE_RIGHT,
		CPLANE_RIGHT | CPLANE_TOP | CPLANE_BOTTOM | CPLANE_NEAR | CPLANE_FAR)
	CLASSIFY_PLANE(CPLANE_TOP, INSIDE_TOP,
		CPLANE_TOP | CPLANE_BOTTOM | CPLANE_NEAR | CPLANE_FAR)
	CLASSIFY_PLANE(CPLANE_BOTTOM, INSIDE_BOTTOM,
		CPLANE_BOTTOM | CPLANE_NEAR | CPLANE_FAR)
	CLASSIFY_PLANE(CPLANE_NEAR, INSIDE_NEAR,
		CPLANE_NEAR | CPLANE_FAR)
	CLASSIFY_PLANE(CPLANE_FAR, INSIDE_FAR,
		CPLANE_FAR)

	return 0;
}

// The facing of a triangle after the divide by z, sign-flipped for a
// mirrored view.
inline float ProjFacing(TLVert* pV0, TLVert* pV1, TLVert* pV2)
{
	float fRHW[3];
	fRHW[0] = 1.0f / pV0->m_vPos.z;
	fRHW[1] = 1.0f / pV1->m_vPos.z;
	fRHW[2] = 1.0f / pV2->m_vPos.z;

	LTVector v0, v1, v2;
	v0.x = pV0->m_vPos.x * fRHW[0];
	v0.y = pV0->m_vPos.y * fRHW[0];
	v1.x = pV1->m_vPos.x * fRHW[1] - v0.x;
	v1.y = pV1->m_vPos.y * fRHW[1] - v0.y;
	v2.x = pV2->m_vPos.x * fRHW[2] - v0.x;
	v2.y = pV2->m_vPos.y * fRHW[2] - v0.y;

	float fCross = v2.x * v1.y - v2.y * v1.x;
	if (g_bMirrorView)
		fCross = -fCross;
	return fCross;
}

// Project a view-space vertex into a bank vertex.
#define PROJECT_VERT(pOut, pIn) \
{ \
	float fRHW = 1.0f / (g_mModelTransform.m[3][0] * (pIn)->m_vPos.x \
	                   + g_mModelTransform.m[3][1] * (pIn)->m_vPos.y \
	                   + g_mModelTransform.m[3][2] * (pIn)->m_vPos.z \
	                   + g_mModelTransform.m[3][3]); \
	(pOut)->m_vPos.x = (g_mModelTransform.m[0][0] * (pIn)->m_vPos.x \
	                  + g_mModelTransform.m[0][1] * (pIn)->m_vPos.y \
	                  + g_mModelTransform.m[0][2] * (pIn)->m_vPos.z \
	                  + g_mModelTransform.m[0][3]) * fRHW; \
	(pOut)->m_vPos.y = (g_mModelTransform.m[1][0] * (pIn)->m_vPos.x \
	                  + g_mModelTransform.m[1][1] * (pIn)->m_vPos.y \
	                  + g_mModelTransform.m[1][2] * (pIn)->m_vPos.z \
	                  + g_mModelTransform.m[1][3]) * fRHW; \
	(pOut)->m_vPos.z = (g_mModelTransform.m[2][0] * (pIn)->m_vPos.x \
	                  + g_mModelTransform.m[2][1] * (pIn)->m_vPos.y \
	                  + g_mModelTransform.m[2][2] * (pIn)->m_vPos.z \
	                  + g_mModelTransform.m[2][3]) * fRHW; \
	(pOut)->m_fRHW = fRHW; \
}

// Project a clipped vertex where it is.
inline void ProjectVertInPlace(TLVert* pVert)
{
	LTVector vPos;
	float fRHW;

	fRHW = 1.0f / (g_mModelTransform.m[3][0] * pVert->m_vPos.x
	             + g_mModelTransform.m[3][1] * pVert->m_vPos.y
	             + g_mModelTransform.m[3][2] * pVert->m_vPos.z
	             + g_mModelTransform.m[3][3]);
	vPos.x = (g_mModelTransform.m[0][0] * pVert->m_vPos.x
	        + g_mModelTransform.m[0][1] * pVert->m_vPos.y
	        + g_mModelTransform.m[0][2] * pVert->m_vPos.z
	        + g_mModelTransform.m[0][3]) * fRHW;
	vPos.y = (g_mModelTransform.m[1][0] * pVert->m_vPos.x
	        + g_mModelTransform.m[1][1] * pVert->m_vPos.y
	        + g_mModelTransform.m[1][2] * pVert->m_vPos.z
	        + g_mModelTransform.m[1][3]) * fRHW;
	vPos.z = (g_mModelTransform.m[2][0] * pVert->m_vPos.x
	        + g_mModelTransform.m[2][1] * pVert->m_vPos.y
	        + g_mModelTransform.m[2][2] * pVert->m_vPos.z
	        + g_mModelTransform.m[2][3]) * fRHW;
	pVert->m_fRHW = fRHW;
	pVert->m_vPos = vPos;
}

// Project a view-space position, its depth pushed back by fPushZ: x and y
// are projected from where it is, z and rhw from fPushZ further away.
inline void ProjectVertPushed(TLVert* pOut, LTVector* pIn, float fPushZ)
{
	LTVector vPos;
	float fRHW;
	float fZ;

	fRHW = 1.0f / (g_mModelTransform.m[3][0] * pIn->x
	             + g_mModelTransform.m[3][1] * pIn->y
	             + g_mModelTransform.m[3][2] * pIn->z
	             + g_mModelTransform.m[3][3]);
	vPos.x = (g_mModelTransform.m[0][0] * pIn->x
	        + g_mModelTransform.m[0][1] * pIn->y
	        + g_mModelTransform.m[0][2] * pIn->z
	        + g_mModelTransform.m[0][3]) * fRHW;
	vPos.y = (g_mModelTransform.m[1][0] * pIn->x
	        + g_mModelTransform.m[1][1] * pIn->y
	        + g_mModelTransform.m[1][2] * pIn->z
	        + g_mModelTransform.m[1][3]) * fRHW;
	fZ = fPushZ + pIn->z;
	fRHW = 1.0f / (g_mModelTransform.m[3][0] * pIn->x
	             + g_mModelTransform.m[3][1] * pIn->y
	             + g_mModelTransform.m[3][2] * fZ
	             + g_mModelTransform.m[3][3]);
	vPos.z = (g_mModelTransform.m[2][0] * pIn->x
	        + g_mModelTransform.m[2][1] * pIn->y
	        + g_mModelTransform.m[2][2] * fZ
	        + g_mModelTransform.m[2][3]) * fRHW;
	pOut->m_fRHW = fRHW;
	pOut->m_vPos = vPos;
}

// One corner into a bank vertex: its colour, the drawer's specular and
// the uv generator's coordinates.
#define SETUP_TL_VERT(p, v, i) \
	((TLVert*)(p))->m_nDiffuse = (v)->m_nColor; \
	((TLVert*)(p))->m_nSpecular = m_nSpecular; \
	m_pfnGenUV((p), (v), pTri->m_UV[i]);

// 0x10002050 -- as DrawTLVertsClipped, for a model drawn really close to
// the camera (the weapon in view): its depth is projected pushed back by
// g_fReallyClosePushZ, so it is not cut by the near plane.
LTBOOL CD3DModelDraw::DrawTLVertsReallyClose(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint8* pOut = (uint8*)m_pVB->Lock();
	uint8* pLast = m_pVB->GetLastVert();
	ModelTri* pTri = pSet->m_pTris;
	int32 nTris = pSet->m_nTris;

	for (; nTris; nTris--, pTri++)
	{
		ModelVert* pV0 = &pVerts[pTri->m_Indices[0]];
		ModelVert* pV1 = &pVerts[pTri->m_Indices[1]];
		ModelVert* pV2 = &pVerts[pTri->m_Indices[2]];
		uint32 nClip;
		uint32 nClass;

		nClass = ClassifyTri(pV0, pV1, pV2, g_ClipFlags, &nClip);
		if (nClass == 0)
		{
			if (ProjFacing((TLVert*)pV0, (TLVert*)pV1, (TLVert*)pV2) <= 0.0f)
				continue;

			ProjectVertPushed((TLVert*)pOut, &pV0->m_vPos, g_fReallyClosePushZ);
			SETUP_TL_VERT(pOut, pV0, 0)
			pOut += m_nVertSize;
			ProjectVertPushed((TLVert*)pOut, &pV1->m_vPos, g_fReallyClosePushZ);
			SETUP_TL_VERT(pOut, pV1, 1)
			pOut += m_nVertSize;
			ProjectVertPushed((TLVert*)pOut, &pV2->m_vPos, g_fReallyClosePushZ);
			SETUP_TL_VERT(pOut, pV2, 2)
			pOut += m_nVertSize;

			if (pOut > pLast && nTris > 1)
			{
				FlushFullVB();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}
		}
		else if (nClass == 2)
		{
			uint8 aClip[MODEL_CLIP_VERTS * sizeof(ClipVert2)];
			uint8* pClip = aClip;
			int32 nVerts = 3;
			uint8* p;
			int32 i;

			p = aClip;
			((TLVert*)p)->m_vPos = pV0->m_vPos;
			SETUP_TL_VERT(p, pV0, 0)
			p += m_nVertSize;
			((TLVert*)p)->m_vPos = pV1->m_vPos;
			SETUP_TL_VERT(p, pV1, 1)
			p += m_nVertSize;
			((TLVert*)p)->m_vPos = pV2->m_vPos;
			SETUP_TL_VERT(p, pV2, 2)

			if (!m_pfnClipPoly(nClip, (void**)&pClip, &nVerts))
				continue;

			if (ProjFacing((TLVert*)pClip, (TLVert*)(pClip + m_nVertSize),
			               (TLVert*)(pClip + m_nVertSize * 2)) <= 0.0f)
				continue;

			uint8* pEnd = pClip + nVerts * m_nVertSize;
			for (p = pClip; p < pEnd; p += m_nVertSize)
			{
				LTVector vPos = ((TLVert*)p)->m_vPos;
				ProjectVertPushed((TLVert*)p, &vPos, g_fReallyClosePushZ);
			}

			// Not enough room for the fan: draw what there is and go on
			// in the next buffer.
			if (pOut + (nVerts - 2) * 3 * m_nVertSize > pLast)
			{
				FlushVerts(pOut);
				m_pVB->Flush();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}

			nVerts--;
			for (i = 1; i < nVerts; )
			{
				m_pfnCopyVert(pOut, pClip);
				pOut += m_nVertSize;
				m_pfnCopyVert(pOut, pClip + i * m_nVertSize);
				pOut += m_nVertSize;
				i++;
				m_pfnCopyVert(pOut, pClip + i * m_nVertSize);
				pOut += m_nVertSize;
			}
		}
	}

	FlushVerts(pOut);
	return LTTRUE;
}

// 0x10002bc0 -- view-space triangles against the clip planes: inside,
// projected straight into the bank; straddling, clipped as a polygon,
// projected and fanned into the bank.
LTBOOL CD3DModelDraw::DrawTLVertsClipped(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint8* pOut = (uint8*)m_pVB->Lock();
	uint8* pLast = m_pVB->GetLastVert();
	ModelTri* pTri = pSet->m_pTris;
	int32 nTris = pSet->m_nTris;

	for (; nTris; nTris--, pTri++)
	{
		ModelVert* pV0 = &pVerts[pTri->m_Indices[0]];
		ModelVert* pV1 = &pVerts[pTri->m_Indices[1]];
		ModelVert* pV2 = &pVerts[pTri->m_Indices[2]];
		uint32 nClip;
		uint32 nClass;

		nClass = ClassifyTri(pV0, pV1, pV2, g_ClipFlags, &nClip);
		if (nClass == 0)
		{
			if (ProjFacing((TLVert*)pV0, (TLVert*)pV1, (TLVert*)pV2) <= 0.0f)
				continue;

			PROJECT_VERT((TLVert*)pOut, pV0)
			SETUP_TL_VERT(pOut, pV0, 0)
			pOut += m_nVertSize;
			PROJECT_VERT((TLVert*)pOut, pV1)
			SETUP_TL_VERT(pOut, pV1, 1)
			pOut += m_nVertSize;
			PROJECT_VERT((TLVert*)pOut, pV2)
			SETUP_TL_VERT(pOut, pV2, 2)
			pOut += m_nVertSize;

			if (pOut > pLast && nTris > 1)
			{
				FlushFullVB();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}
		}
		else if (nClass == 2)
		{
			uint8 aClip[MODEL_CLIP_VERTS * sizeof(ClipVert2)];
			uint8* pClip = aClip;
			int32 nVerts = 3;
			uint8* p;
			int32 i;

			p = aClip;
			((TLVert*)p)->m_vPos = pV0->m_vPos;
			SETUP_TL_VERT(p, pV0, 0)
			p += m_nVertSize;
			((TLVert*)p)->m_vPos = pV1->m_vPos;
			SETUP_TL_VERT(p, pV1, 1)
			p += m_nVertSize;
			((TLVert*)p)->m_vPos = pV2->m_vPos;
			SETUP_TL_VERT(p, pV2, 2)

			if (!m_pfnClipPoly(nClip, (void**)&pClip, &nVerts))
				continue;

			if (ProjFacing((TLVert*)pClip, (TLVert*)(pClip + m_nVertSize),
			               (TLVert*)(pClip + m_nVertSize * 2)) <= 0.0f)
				continue;

			uint8* pEnd = pClip + nVerts * m_nVertSize;
			for (p = pClip; p < pEnd; p += m_nVertSize)
				ProjectVertInPlace((TLVert*)p);

			// Not enough room for the fan: draw what there is and go on
			// in the next buffer.
			if (pOut + (nVerts - 2) * 3 * m_nVertSize > pLast)
			{
				FlushVerts(pOut);
				m_pVB->Flush();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}

			nVerts--;
			for (i = 1; i < nVerts; )
			{
				m_pfnCopyVert(pOut, pClip);
				pOut += m_nVertSize;
				m_pfnCopyVert(pOut, pClip + i * m_nVertSize);
				pOut += m_nVertSize;
				i++;
				m_pfnCopyVert(pOut, pClip + i * m_nVertSize);
				pOut += m_nVertSize;
			}
		}
	}

	FlushVerts(pOut);
	return LTTRUE;
}


// 0x100036d0 -- view-space triangles of a piece wholly inside the clip
// planes: projected straight into the bank.
LTBOOL CD3DModelDraw::DrawTLVertsNoClip(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint8* pOut = (uint8*)m_pVB->Lock();
	uint8* pLast = m_pVB->GetLastVert();
	ModelTri* pTri = pSet->m_pTris;
	int32 nTris = pSet->m_nTris;

	for (; nTris; nTris--, pTri++)
	{
		ModelVert* pV0 = &pVerts[pTri->m_Indices[0]];
		ModelVert* pV1 = &pVerts[pTri->m_Indices[1]];
		ModelVert* pV2 = &pVerts[pTri->m_Indices[2]];

		if (m_bBackfaceCull &&
		    ProjFacing((TLVert*)pV0, (TLVert*)pV1, (TLVert*)pV2) <= 0.0f)
			continue;

		PROJECT_VERT((TLVert*)pOut, pV0)
		SETUP_TL_VERT(pOut, pV0, 0)
		pOut += m_nVertSize;
		PROJECT_VERT((TLVert*)pOut, pV1)
		SETUP_TL_VERT(pOut, pV1, 1)
		pOut += m_nVertSize;
		PROJECT_VERT((TLVert*)pOut, pV2)
		SETUP_TL_VERT(pOut, pV2, 2)
		pOut += m_nVertSize;

		if (pOut > pLast && nTris > 1)
		{
			FlushFullVB();
			pOut = (uint8*)m_pVB->Lock();
			pLast = m_pVB->GetLastVert();
		}
	}

	FlushVerts(pOut);
	return LTTRUE;
}

// 0x10003b60 -- pre-transformed (XYZRHW) triangles, culled when they face
// away or are smaller than ModelMinTri.
LTBOOL CD3DModelDraw::DrawTLVerts(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint8* pOut = (uint8*)m_pVB->Lock();
	uint8* pLast = m_pVB->GetLastVert();

	ModelTri* pTri = pSet->m_pTris;
	int32 nTris = pSet->m_nTris;
	for (; nTris; nTris--, pTri++)
	{
		ModelVert* pV[3];
		pV[0] = &pVerts[pTri->m_Indices[0]];
		pV[1] = &pVerts[pTri->m_Indices[1]];
		pV[2] = &pVerts[pTri->m_Indices[2]];

		if (m_bBackfaceCull && TriFacing(pV[0], pV[1], pV[2]) <= g_CV_ModelMinTri.m_fValue)
			continue;

		TLVert* pT;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[0]->m_vPos;
		pT->m_fRHW = pV[0]->m_fRHW;
		pT->m_nDiffuse = pV[0]->m_nColor;
		pT->m_nSpecular = m_nSpecular;
		m_pfnGenUV(pOut, pV[0], pTri->m_UV[0]);
		pOut += m_nVertSize;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[1]->m_vPos;
		pT->m_fRHW = pV[1]->m_fRHW;
		pT->m_nDiffuse = pV[1]->m_nColor;
		pT->m_nSpecular = m_nSpecular;
		m_pfnGenUV(pOut, pV[1], pTri->m_UV[1]);
		pOut += m_nVertSize;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[2]->m_vPos;
		pT->m_fRHW = pV[2]->m_fRHW;
		pT->m_nDiffuse = pV[2]->m_nColor;
		pT->m_nSpecular = m_nSpecular;
		m_pfnGenUV(pOut, pV[2], pTri->m_UV[2]);
		pOut += m_nVertSize;

		if (pOut > pLast && nTris > 1)
		{
			FlushFullVB();
			pOut = (uint8*)m_pVB->Lock();
			pLast = m_pVB->GetLastVert();
		}
	}

	FlushVerts(pOut);
	return LTTRUE;
}

// One corner into a bank vertex the device transforms: it has no rhw, so
// the rest of it, and the uv generator, which write the pre-transformed
// layout, are handed the vertex four bytes early.
#define SETUP_TNL_VERT(i) \
	((LVert*)pOut)->m_vPos = pV[i]->m_vPos; \
	pOut -= sizeof(float); \
	((TLVert*)pOut)->m_nDiffuse = pV[i]->m_nColor; \
	((TLVert*)pOut)->m_nSpecular = m_nSpecular; \
	m_pfnGenUV(pOut, pV[i], pTri->m_UV[i]); \
	pOut += m_nVertSize;

// 0x10003e00 -- triangles the device transforms (TnL).
LTBOOL CD3DModelDraw::DrawTnLVerts(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint8* pOut = (uint8*)m_pVB->Lock();
	uint8* pLast = m_pVB->GetLastVert();
	ModelTri* pTri = pSet->m_pTris;
	int32 nTris = pSet->m_nTris;
	ModelVert* pV[3];

	if (m_bBackfaceCull)
	{
		for (; nTris; nTris--, pTri++)
		{
			pV[0] = &pVerts[pTri->m_Indices[0]];
			pV[1] = &pVerts[pTri->m_Indices[1]];
			pV[2] = &pVerts[pTri->m_Indices[2]];

			if (TriFacing(pV[0], pV[1], pV[2]) <= g_CV_ModelMinTri.m_fValue)
				continue;

			SETUP_TNL_VERT(0)
			SETUP_TNL_VERT(1)
			SETUP_TNL_VERT(2)

			if (pOut > pLast && nTris > 1)
			{
				FlushFullVB();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}
		}
	}
	else
	{
		for (; nTris; nTris--, pTri++)
		{
			pV[0] = &pVerts[pTri->m_Indices[0]];
			pV[1] = &pVerts[pTri->m_Indices[1]];
			pV[2] = &pVerts[pTri->m_Indices[2]];

			SETUP_TNL_VERT(0)
			SETUP_TNL_VERT(1)
			SETUP_TNL_VERT(2)

			if (pOut > pLast && nTris > 1)
			{
				FlushFullVB();
				pOut = (uint8*)m_pVB->Lock();
				pLast = m_pVB->GetLastVert();
			}
		}
	}

	FlushVerts(pOut);
	return LTTRUE;
}

// 0x10004230 -- the cached path: the piece's vertices are already in the
// bank, so only the draw.
LTBOOL CD3DModelDraw::DrawCachedVerts(ModelVertSet* pSet, ModelVert* pVerts)
{
	uint32 nVerts = pSet->m_nTris * 3;
	g_nModelTriangles += nVerts / 3;
	m_pVB->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLELIST, nVerts);
	return LTTRUE;
}

// 0x10004270 -- set the drawer up for the bank's vertex format and draw
// each visible piece with fn, or fnClip where the piece needs clipping.
// NEAR (808 bytes, size-exact): cl 8966 here sinks the store of
// m_iCurTexture below the loop set-up and schedules the cache's vertex
// size as `add -4` before the store of m_pVB.
void CD3DModelDraw::DrawVerts(DrawVertsFn fn, DrawVertsFn fnClip, uint32 nValue)
{
	m_nValue = nValue;

	if (m_nVertFormat == 1)
	{
		m_nVertSize = sizeof(ClipVert);
		m_pfnCopyVert = (CopyVertFn)d3d_CopyVert;
		m_pfnClipPoly = (ClipPolyFn)d3d_ClipPoly;
		m_nFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1;
		if (m_bTnL)
			m_pVB = &g_ModelVBTnL;
		else
			m_pVB = &g_ModelVB;
	}
	else
	{
		m_nVertSize = sizeof(ClipVert2);
		m_pfnCopyVert = (CopyVertFn)d3d_CopyVert2;
		m_pfnClipPoly = (ClipPolyFn)d3d_ClipPoly2;
		m_nFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2;
		if (m_bTnL)
			m_pVB = &g_ModelVB2TnL;
		else
			m_pVB = &g_ModelVB2;
	}

	// The console changed the bank sizes: reallocate.
	if (m_pVB->m_nVerts != (uint32)g_CV_ModelVBSize.m_nValue ||
	    m_pVB->m_nBuffers != (uint32)g_CV_ModelVBCount.m_nValue)
	{
		d3d_TermModelVBs();
		d3d_InitModelVBs();
	}

	if (m_bTnL)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);

	// Cached: the vertices go to (or already are in) the model cache.
	if (m_nVertMode)
	{
		UseModelVBCache(m_nFVF, m_nVertSize);
	}

	m_bBackfaceCull = (m_nVertMode != 1 && !m_bTnL);
	m_iCurTexture = (uint32)-1;

	ModelVert* pVerts = m_pVerts;
	for (uint32 i = 0; i < m_pModel->NumPieces(); i++)
	{
		ModelPiece* pPiece = m_pModel->GetPiece(i);
		ModelVertSet* pSet = pPiece->GetLOD(m_iLOD);
		if (!pSet)
			continue;

		if ((1 << i) & m_pInstance->m_HiddenPieces)
			continue;

		if (m_bPieceHidden[i])
			continue;

		if (pPiece->m_iTexture != m_iCurTexture)
		{
			SetupTexture(pPiece->m_iTexture);
			SetupTextureStages(&m_nValue);

			RenderTexture* pTexture = m_pInstance->m_pSkins[pPiece->m_iTexture];
			if (pTexture)
			{
				CRenderStateList* pStates = pTexture->m_pRenderStates;
				if (pStates)
				{
					g_SavedStates.Restore();
					g_SavedStates.Apply(pStates, m_iStage);
				}
			}
		}

		m_fSpecularPower = pPiece->m_fSpecularPower;
		m_fSpecularScale = pPiece->m_fSpecularScale;
		if (g_CV_SpecularPowerTest.m_fValue != 0.0f)
			m_fSpecularPower = g_CV_SpecularPowerTest.m_fValue;
		if (g_CV_SpecularScaleTest.m_fValue != 0.0f)
			m_fSpecularScale = g_CV_SpecularScaleTest.m_fValue;

		if (m_fSpecularScale != 0.0f && m_pfnGenSpecular == (GenSpecularFn)d3d_GenUVSpecular)
			d3d_SetGammaTexture(m_fSpecularPower);

		if (m_bPieceClipped[i])
			(this->*fnClip)(pSet, pVerts);
		else
			(this->*fn)(pSet, pVerts);

		pVerts += pSet->m_nVerts;
	}

	if (m_bTnL)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

	g_SavedStates.Restore();
}

//@@DRAWFUNCS@@

// 0x100045a0 -- set up and draw one piece's vertices, by how they get to
// the screen.
void CD3DModelDraw::DrawPiece(uint32 nValue)
{
	DrawVertsFn fnClipped, fnUnclipped;

	if (m_nVertMode == 2)
	{
		fnUnclipped = fnClipped = &CD3DModelDraw::DrawCachedVerts;
	}
	else if (m_nVertMode == 1 || m_bTnL)
	{
		fnUnclipped = fnClipped = &CD3DModelDraw::DrawTnLVerts;
	}
	else if (m_pInstance->m_Flags & 0x40)
	{
		fnUnclipped = fnClipped = &CD3DModelDraw::DrawTLVertsReallyClose;
	}
	else if (g_ClipFlags & 0x3f)
	{
		fnUnclipped = &CD3DModelDraw::DrawTLVertsClipped;
		fnClipped = &CD3DModelDraw::DrawTLVertsNoClip;
	}
	else
	{
		fnUnclipped = fnClipped = &CD3DModelDraw::DrawTLVerts;
	}

	DrawVerts(fnClipped, fnUnclipped, nValue);
}
