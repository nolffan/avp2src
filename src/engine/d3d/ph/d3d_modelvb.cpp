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
//  COMDAT .bss). Between them still unwritten: 0x10004660, 0x10005700
//  and 0x100062e0.
//
//  /O2 /Ob2: the global objects' initialisers are single functions (with
//  /Ob1 each is a `jmp` stub over a body, and the banks' a call pair).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <d3d.h>


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

	uint8       m_Cache[0x60 - 0x3c];                       // +0x3c
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

	// NEAR (0x10006180, 0x10006230): retail pushes ebp in the prologue,
	// cl 8966 here shrink-wraps it into the loop.
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
			if (bOK)
			{
				if (pD3D->CreateVertexBuffer(&desc, &pVB, 0) >= 0)
					m_pBuffers[i] = pVB;
				else
					bOK = LTFALSE;
			}
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
struct ModelVert
{
	uint8       m_pad_00[0x14];                             // +0x00
	LTVector    m_vNormal;                                  // +0x14
};

struct ModelVertSet;
struct ModelInstanceFlags
{
	uint8       m_pad_00[0x88];
	uint32      m_Flags;                                    // +0x88
};

class CD3DModelDraw;
typedef LTBOOL (CD3DModelDraw::*DrawVertsFn)(ModelVertSet* pSet);

extern uint32   g_ClipFlags;                                // 0x1005bd0c

class CD3DModelDraw
{
public:
	LTBOOL      DrawTLVertsReallyClose(ModelVertSet* pSet); // 0x10002050
	LTBOOL      DrawTLVertsClipped(ModelVertSet* pSet);     // 0x10002bc0
	LTBOOL      DrawTLVertsClipped2(ModelVertSet* pSet);    // 0x100036d0
	LTBOOL      DrawTLVerts(ModelVertSet* pSet);            // 0x10003b60
	LTBOOL      DrawTnLVerts(ModelVertSet* pSet);           // 0x10003e00
	LTBOOL      DrawCachedVerts(ModelVertSet* pSet);        // 0x10004230
	void        DrawVerts(DrawVertsFn fn, DrawVertsFn fn2,
	                      ModelVertSet* pSet);              // 0x10004270
	void        DrawPiece(ModelVertSet* pSet);              // 0x100045a0

	uint8       m_pad_000[0x4];                             // +0x000
	ModelInstanceFlags* m_pInstance;                        // +0x004
	uint8       m_pad_008[0x590 - 0x8];                     // +0x008
	LTMatrix    m_mEnvMap;                                  // +0x590
	uint8       m_pad_5d0[0x61c - 0x5d0];                   // +0x5d0
	float       m_fEnvOffsetU;                              // +0x61c
	float       m_fEnvOffsetV;                              // +0x620
	float       m_fEnvScaleU;                               // +0x624
	float       m_fEnvScaleV;                               // +0x628
	uint8       m_pad_62c[0x630 - 0x62c];                   // +0x62c
	float       m_fSpecularScale;                           // +0x630
	uint8       m_pad_634[0x874 - 0x634];                   // +0x634
	LTVector    m_vSpecularDir;                             // +0x874
	uint8       m_pad_880[0x8ac - 0x880];                   // +0x880
	uint32      m_nVertMode;                                // +0x8ac
	LTBOOL      m_bTnL;                                     // +0x8b0
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

// 0x100045a0 -- set up and draw one piece's vertices, by how they get to
// the screen.
void CD3DModelDraw::DrawPiece(ModelVertSet* pSet)
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
		fnClipped = &CD3DModelDraw::DrawTLVertsClipped2;
	}
	else
	{
		fnUnclipped = fnClipped = &CD3DModelDraw::DrawTLVerts;
	}

	DrawVerts(fnClipped, fnUnclipped, pSet);
}

// ======================================================================= //
//  The polygon clippers (the drawer's +0x440), 0x10001530..0x1000204a, and
//  their plane functions. ClipTop..ClipFar are templates over the vertex,
//  so they follow this file's last function (0x100063e0..0x10007924).
// ======================================================================= //

#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

// Round to nearest in the FPU's mode. Every colour byte the clippers
// interpolate is `fstp t ; fld t ; fistp n`, a float parameter stored and
// reloaded for an x87 store: the original's assembly helper, which no
// C spelling under cl 8966 gives (a cast calls _ftol).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

union VColor
{
	uint32      m_n;
	uint8       m_b[4];
};

// The 32-byte bank vertex (FVF 0x1c4) as the clippers see it.
struct ClipVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	VColor      m_Diffuse;                                  // +0x10
	VColor      m_Specular;                                 // +0x14
	float       m_fU;                                       // +0x18
	float       m_fV;                                       // +0x1c
};

// The 40-byte bank vertex (FVF 0x2c4): two texture coordinate pairs.
struct ClipVert2
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	VColor      m_Diffuse;                                  // +0x10
	VColor      m_Specular;                                 // +0x14
	float       m_fU;                                       // +0x18
	float       m_fV;                                       // +0x1c
	float       m_fU2;                                      // +0x20
	float       m_fV2;                                      // +0x24
};

extern uint32   g_nClipPasses;                              // 0x1005c124
extern int32    g_bUseD3DClip;                              // 0x100622b0
extern void*    g_pClipScratch;                             // 0x1005bd14                              // 0x1005c124
extern float    g_fNearZ;                                   // 0x1005b818
extern float    g_fFarZ;                                    // 0x1005b820

inline void LerpClipVert(ClipVert* a, ClipVert* b, ClipVert* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_Diffuse.m_b[2]  = (uint8)RoundF((b->m_Diffuse.m_b[2]  - a->m_Diffuse.m_b[2])  * t + a->m_Diffuse.m_b[2]);
	dst->m_Diffuse.m_b[1]  = (uint8)RoundF((b->m_Diffuse.m_b[1]  - a->m_Diffuse.m_b[1])  * t + a->m_Diffuse.m_b[1]);
	dst->m_Diffuse.m_b[0]  = (uint8)RoundF((b->m_Diffuse.m_b[0]  - a->m_Diffuse.m_b[0])  * t + a->m_Diffuse.m_b[0]);
	dst->m_Diffuse.m_b[3]  = (uint8)RoundF((b->m_Diffuse.m_b[3]  - a->m_Diffuse.m_b[3])  * t + a->m_Diffuse.m_b[3]);
	dst->m_Specular.m_b[3] = (uint8)RoundF((b->m_Specular.m_b[3] - a->m_Specular.m_b[3]) * t + a->m_Specular.m_b[3]);
}

inline void LerpClipVert(ClipVert2* a, ClipVert2* b, ClipVert2* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_fU2 = (b->m_fU2 - a->m_fU2) * t + a->m_fU2;
	dst->m_fV2 = (b->m_fV2 - a->m_fV2) * t + a->m_fV2;
	dst->m_Diffuse.m_b[2]  = (uint8)RoundF((b->m_Diffuse.m_b[2]  - a->m_Diffuse.m_b[2])  * t + a->m_Diffuse.m_b[2]);
	dst->m_Diffuse.m_b[1]  = (uint8)RoundF((b->m_Diffuse.m_b[1]  - a->m_Diffuse.m_b[1])  * t + a->m_Diffuse.m_b[1]);
	dst->m_Diffuse.m_b[0]  = (uint8)RoundF((b->m_Diffuse.m_b[0]  - a->m_Diffuse.m_b[0])  * t + a->m_Diffuse.m_b[0]);
	dst->m_Diffuse.m_b[3]  = (uint8)RoundF((b->m_Diffuse.m_b[3]  - a->m_Diffuse.m_b[3])  * t + a->m_Diffuse.m_b[3]);
	dst->m_Specular.m_b[3] = (uint8)RoundF((b->m_Specular.m_b[3] - a->m_Specular.m_b[3]) * t + a->m_Specular.m_b[3]);
}

inline float ClipEdgeNear(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_fNearZ - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_fNearZ;

	return t;
}

inline float ClipEdgeLeft(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeTop(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeRight(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeBottom(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeFar(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_fFarZ - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_fFarZ;

	return t;
}


#define CLIP_PLANE(NAME, INSIDE, EDGE) \
template<class V> \
LTBOOL NAME(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut) \
{ \
	static uint32 anInside[CLIP_MAX_VERTS]; \
	V *pV, *pEnd, *pPrev, *pCur, *pOutStart; \
	uint32* pFlag; \
	int32 nInside, i, iPrev; \
	float t; \
 \
	nInside = 0; \
	++g_nClipPasses; \
 \
	pFlag = anInside; \
	pV = *ppVerts; \
	pEnd = pV + *pnVerts; \
	for (; pV != pEnd; ++pV) \
	{ \
		uint32 bIn = INSIDE; \
 \
		*pFlag = bIn; \
		nInside += bIn; \
		pFlag++; \
	} \
 \
	if (nInside == 0) \
		return LTFALSE; \
 \
	if (nInside != *pnVerts) \
	{ \
		pOutStart = *ppOut; \
		iPrev = *pnVerts - 1; \
		pPrev = &(*ppVerts)[iPrev]; \
		for (i = 0; i < *pnVerts; i++) \
		{ \
			pCur = &(*ppVerts)[i]; \
 \
			if (anInside[iPrev]) \
			{ \
				**ppOut = *pPrev; \
				++(*ppOut); \
			} \
 \
			if (anInside[iPrev] != anInside[i]) \
			{ \
				t = EDGE(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos); \
				LerpClipVert(pPrev, pCur, *ppOut, t); \
				++(*ppOut); \
			} \
 \
			iPrev = i; \
			pPrev = pCur; \
		} \
 \
		*pnVerts = *ppOut - pOutStart; \
		*ppVerts = pOutStart; \
		*ppOut += *pnVerts; \
	} \
 \
	return LTTRUE; \
}

#define CLIP_PLANE_V(NAME, V, INSIDE, EDGE) \
inline LTBOOL NAME(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut) \
{ \
	static uint32 anInside[CLIP_MAX_VERTS]; \
	V *pV, *pEnd, *pPrev, *pCur, *pOutStart; \
	uint32* pFlag; \
	int32 nInside, i, iPrev; \
	float t; \
 \
	nInside = 0; \
	++g_nClipPasses; \
 \
	pFlag = anInside; \
	pV = *ppVerts; \
	pEnd = pV + *pnVerts; \
	for (; pV != pEnd; ++pV) \
	{ \
		uint32 bIn = INSIDE; \
 \
		*pFlag = bIn; \
		nInside += bIn; \
		pFlag++; \
	} \
 \
	if (nInside == 0) \
		return LTFALSE; \
 \
	if (nInside != *pnVerts) \
	{ \
		pOutStart = *ppOut; \
		iPrev = *pnVerts - 1; \
		pPrev = &(*ppVerts)[iPrev]; \
		for (i = 0; i < *pnVerts; i++) \
		{ \
			pCur = &(*ppVerts)[i]; \
 \
			if (anInside[iPrev]) \
			{ \
				**ppOut = *pPrev; \
				++(*ppOut); \
			} \
 \
			if (anInside[iPrev] != anInside[i]) \
			{ \
				t = EDGE(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos); \
				LerpClipVert(pPrev, pCur, *ppOut, t); \
				++(*ppOut); \
			} \
 \
			iPrev = i; \
			pPrev = pCur; \
		} \
 \
		*pnVerts = *ppOut - pOutStart; \
		*ppVerts = pOutStart; \
		*ppOut += *pnVerts; \
	} \
 \
	return LTTRUE; \
}

CLIP_PLANE_V(ClipNear, ClipVert, (pV->m_vPos.z >= g_fNearZ), ClipEdgeNear)
CLIP_PLANE_V(ClipLeft, ClipVert, (-pV->m_vPos.z < pV->m_vPos.x), ClipEdgeLeft)
CLIP_PLANE_V(ClipNear, ClipVert2, (pV->m_vPos.z >= g_fNearZ), ClipEdgeNear)
CLIP_PLANE_V(ClipLeft, ClipVert2, (-pV->m_vPos.z < pV->m_vPos.x), ClipEdgeLeft)
template<class V> LTBOOL ClipTop(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipRight(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipBottom(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipFar(uint32* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);


// Clip a polygon against the planes in nClipFlags, into the clip scratch
// buffer. FALSE when nothing is left.
#define CLIP_POLY(NAME, V) \
LTBOOL __fastcall NAME(uint32 nClipFlags, V** ppVerts, int32* pnVerts) \
{ \
	V* pOut; \
	V* pVerts; \
	int32 nVerts; \
	uint32 anUnused[6]; \
 \
	if (g_bUseD3DClip) \
	{ \
		nClipFlags &= 1; \
		if (!nClipFlags) \
			return LTTRUE; \
	} \
 \
	pOut = (V*)g_pClipScratch; \
	pVerts = *ppVerts; \
	nVerts = *pnVerts; \
 \
	if (nClipFlags & 0x01) \
	{ \
		if (!ClipNear(&anUnused[0], &pVerts, &nVerts, &pOut)) \
			return LTFALSE; \
	} \
	if (nClipFlags & 0x04) \
	{ \
		if (!ClipLeft(&anUnused[1], &pVerts, &nVerts, &pOut)) \
			return LTFALSE; \
	} \
	if (nClipFlags & 0x08) \
	{ \
		if (!ClipTop(&anUnused[2], &pVerts, &nVerts, &pOut)) \
			return LTFALSE; \
	} \
	if (nClipFlags & 0x10) \
	{ \
		if (!ClipRight(&anUnused[3], &pVerts, &nVerts, &pOut)) \
			return LTFALSE; \
	} \
	if (nClipFlags & 0x20) \
	{ \
		if (!ClipBottom(&anUnused[4], &pVerts, &nVerts, &pOut)) \
			return LTFALSE; \
	} \
	if (!(nClipFlags & 0x02) || \
	    ClipFar(&anUnused[5], &pVerts, &nVerts, &pOut)) \
	{ \
		*ppVerts = pVerts; \
		*pnVerts = nVerts; \
		return LTTRUE; \
	} \
	return LTFALSE; \
}

// 0x10001530 / 0x10001b30 -- NEAR: retail inlines ClipNear and ClipLeft
// and calls ClipTop; cl 8966 here inlines ClipTop as well (1335 bytes
// against 1032; 1321 against 1001).
CLIP_POLY(d3d_ClipPoly, ClipVert)
CLIP_POLY(d3d_ClipPoly2, ClipVert2)

CLIP_PLANE(ClipTop,    (pV->m_vPos.y < pV->m_vPos.z),  ClipEdgeTop)
CLIP_PLANE(ClipRight,  (pV->m_vPos.x < pV->m_vPos.z),  ClipEdgeRight)
CLIP_PLANE(ClipBottom, (-pV->m_vPos.z < pV->m_vPos.y), ClipEdgeBottom)
CLIP_PLANE(ClipFar,    (pV->m_vPos.z <= g_fFarZ),      ClipEdgeFar)

// ClipTop's own instances (0x100063e0, 0x10006e40), which the drivers
// above call in retail and inline here.
template LTBOOL ClipTop<ClipVert>(uint32*, ClipVert**, int32*, ClipVert**);
template LTBOOL ClipTop<ClipVert2>(uint32*, ClipVert2**, int32*, ClipVert2**);
