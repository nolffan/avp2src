// *********************************************************************** //
//  ph/drawpolymgr.cpp -- Primal Hunt's d3d.ren, TU30
//  (0x1002a70f..0x1002be3c), /O1.
//
//  The multipass world-polygon manager (DrawPolyMgr, the global at
//  0x100746d8): the DetailTextureScale and DrawPolyMgr console variables,
//  and the small per-stage members its tables in .data
//  (0x1004dbe8..0x1004dc84) point at -- texture coordinate generators,
//  texture binders and colour sources.
//
//  Not yet written here: the global's construction and destruction
//  (0x1002a751..0x1002a931) and the larger members (0x1002aa7b,
//  0x1002ab18, 0x1002acef, 0x1002adba).
// *********************************************************************** //

#include <string.h>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include "../../LT2/lithshared/stdlith/object_bank.h"


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};


struct SharedTexture
{
	uint8			m_pad_00[0x28];
	SharedTexture*	m_pDetail;                          // +0x28
};

struct TextureData
{
	uint8			m_pad_00[0x28];
	SharedTexture*	m_pDetail;                          // +0x28
	SharedTexture*	m_pShared;                          // +0x2c
};

struct Surface
{
	uint8			m_pad_00[0x2c];
	SharedTexture*	m_pTexture;                         // +0x2c
	uint8			m_pad_30[0x48 - 0x30];
	uint32			m_Flags;                            // +0x48
};

struct WorldPoly
{
	uint8			m_pad_00[0x2c];
	Surface*		m_pSurface;                         // +0x2c
};

struct PolyVert
{
	LTVector*		m_pVec;                             // +0x00
	float			m_U;                                // +0x04
	float			m_V;                                // +0x08
	float			m_LMU;                              // +0x0c
	float			m_LMV;                              // +0x10
	uint8			m_Color[3];                         // +0x14
};

struct TexPair
{
	float	u, v;
};

// ---------------------------------------------------------------------- //
//  The material DrawPolyMgr draws with, and its per-pass device state.
// ---------------------------------------------------------------------- //
struct StageMap                                         // 0x24 bytes
{
	StageMap()	{ m_18 = 0; m_1c = 0; m_20 = 0; }

	uint32		m_Data[6];                              // +0x00
	uint32		m_18;                                   // +0x18
	uint32		m_1c;                                   // +0x1c
	uint32		m_20;                                   // +0x20
};

struct PassDesc                                         // 0x8c bytes
{
	PassDesc();

	uint32		m_00;                                   // +0x00
	uint32		m_04;                                   // +0x04
	uint32		m_08;                                   // +0x08
	uint32		m_0c;                                   // +0x0c
	uint32		m_10;                                   // +0x10
	uint32		m_14;                                   // +0x14
	uint32		m_18;                                   // +0x18
	uint32		m_1c;                                   // +0x1c
	StageMap	m_Stages[3];                            // +0x20
};

struct MaterialInst;

struct Material                                         // 0x1cc bytes
{
	Material();

	char			m_Name[0x20];                       // +0x000
	uint32			m_nPasses;                          // +0x020
	PassDesc		m_Passes[3];                        // +0x024
	MaterialInst*	m_pInst;                            // +0x1c8
};

struct TexStage                                         // 0x18 bytes
{
	TexStage()	{ m_ColorOp = 1; m_AlphaOp = 1; }

	uint32		m_ColorOp;                              // +0x00
	uint32		m_ColorArg1;                            // +0x04
	uint32		m_ColorArg2;                            // +0x08
	uint32		m_AlphaOp;                              // +0x0c
	uint32		m_AlphaArg1;                            // +0x10
	uint32		m_AlphaArg2;                            // +0x14
};

struct PassState                                        // 0x1dc bytes
{
	PassState();

	LTLink		m_Lists[0x20];                          // +0x000
	uint32		m_180;                                  // +0x180
	uint32		m_184;                                  // +0x184
	LTLink		m_Pending;                              // +0x188
	TexStage	m_Stages[3];                            // +0x194
};

struct MaterialInst                                     // 0x59c bytes
{
	MaterialInst();

	Material*	m_pMaterial;                            // +0x000
	uint32		m_04;                                   // +0x004
	PassState	m_Passes[3];                            // +0x008
};

struct PolyBatch16
{
	uint32		m_Data[4];
};

struct PolyNode                                         // 0x28 bytes
{
	uint32		m_Data[10];
};

class DrawPolyMgr;
typedef void (DrawPolyMgr::*TexCoordFn)(PolyVert* pSrc, TexPair* pDest, uint32 iStage);

class DrawPolyMgr
{
public:
	void		GenTexCoords_Base(PolyVert* pSrc, TexPair* pDest, uint32 iStage);
	void		GenTexCoords_Lightmap(PolyVert* pSrc, TexPair* pDest, uint32 iStage);
	void		GenTexCoords_Detail(PolyVert* pSrc, TexPair* pDest, uint32 iStage);
	void		GenTexCoords_Scaled(PolyVert* pSrc, TexPair* pDest, uint32 iStage);
	void		GenTexCoords2(void* pPoly, TexPair* pDest, void* pVert);
	void		GenTexCoords3(void* pPoly, TexPair* pDest, void* pVert);
	uint32		SetTexture_Base(WorldPoly* pPoly, uint32 iStage);
	uint32		SetTexture_None(WorldPoly* pPoly, uint32 iStage);
	uint32		SetTexture_DetailScale(WorldPoly* pPoly, uint32 iStage);
	uint32		SetTexture_Disable(WorldPoly* pPoly, uint32 iStage);
	uint32		SetTexture_Shared(WorldPoly* pPoly, uint32 iStage, uint32 flags);
	uint32		SetTexture_Lightmap(WorldPoly* pPoly, uint32 iStage, uint32 flags);
	uint32		SetTexture_Detail(WorldPoly* pPoly, uint32 iStage, uint32 flags);
	uint32		SetTexture_None2(WorldPoly* pPoly, uint32 iStage, uint32 flags);
	void		SetColor_None(WorldPoly* pPoly, uint32* pColor);
	void		SetColor_Lightmap(PolyVert* pSrc, uint8* pDest);
	void		SetColor_Scaled(PolyVert* pSrc, uint8* pDest);

	DrawPolyMgr();
	~DrawPolyMgr();

	void		Init();

	Material		m_Material;                         // +0x000
	MaterialInst	m_Inst;                             // +0x1cc
	uint16			m_768;                              // +0x768
	MaterialInst*	m_pInst;                            // +0x76c
	TexPair		m_TexScale[3];                          // +0x770
	float		m_DetailScaleU;                         // +0x788
	float		m_DetailScaleV;                         // +0x78c
	float		m_DetailOffsetU;                        // +0x790
	float		m_DetailOffsetV;                        // +0x794
	float		m_DetailSizeU;                          // +0x798
	float		m_DetailSizeV;                          // +0x79c
	float		m_ColorScale[3];                        // +0x7a0
	TexCoordFn	m_GenTexCoords[3];                      // +0x7ac
	uint32		m_7b8;                                  // +0x7b8
	CMoArray<PolyBatch16>			m_Batches;          // +0x7bc
	uint32		m_7d0;                                  // +0x7d0
	ObjectBank<PolyNode, NullCS>	m_NodeBank;         // +0x7d4
};

extern uint32	d3d_GetTextureScale(SharedTexture* pTexture, uint32 iStage, float* pU, float* pV);  // 0x10022760
extern void		d3d_DisableTexture(uint32 iStage);      // 0x1000a22b
extern TexPair	g_TexelSize[];                          // 0x100672a8
extern uint8	g_LightTableR[256];                     // 0x1005fa9c
extern uint8	g_LightTableG[256];                     // 0x1005fb9c
extern uint8	g_LightTableB[256];                     // 0x1005fc9c
extern uint8	g_VertexAlpha;                          // 0x1005d20c
extern int		d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
extern uint32	d3d_SetLightmapTexture(WorldPoly* pPoly, uint32 iStage);  // 0x10009e55


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_DetailTextureScale("DetailTextureScale", 0.2f, LTNULL, LTNULL);  // 0x100742b8  $E 0x1002a70f/0x1002a714
CD3DConVar	g_CV_DrawPolyMgr("DrawPolyMgr", 0.0f, LTNULL, LTNULL);                // 0x10074258  $E 0x1002a732/0x1002a737

DrawPolyMgr	g_DrawPolyMgr;                              // 0x100746d8  $E 0x1002a751


// ---------------------------------------------------------------------- //
//  0x1002a767 .. 0x1002a8f0 -- construction.
// ---------------------------------------------------------------------- //
DrawPolyMgr::DrawPolyMgr() : m_NodeBank(32)
{
	m_pInst = LTNULL;
	Init();
}

Material::Material()
{
	m_Name[0] = 0;
	m_nPasses = 0;
	m_pInst = LTNULL;
}

PassDesc::PassDesc()
{
	m_00 = 0;
	m_08 = 0;
	m_04 = 0;
	m_0c = 0;
	m_10 = 0;
	m_14 = 0;
	m_18 = 0;
	m_1c = 0;
}

MaterialInst::MaterialInst()
{
	m_pMaterial = LTNULL;
	m_04 = 0;
}

PassState::PassState()
{
	uint32 i;

	for (i=0; i < 0x20; i++)
		dl_TieOff(&m_Lists[i]);

	dl_TieOff(&m_Pending);

	m_Stages[0].m_ColorOp = 4;
	m_Stages[0].m_ColorArg1 = 2;
	m_Stages[0].m_ColorArg2 = 0;
	m_Stages[0].m_AlphaOp = 4;
	m_Stages[0].m_AlphaArg1 = 2;
	m_Stages[0].m_AlphaArg2 = 0;
}

void DrawPolyMgr::Init()
{
	strcpy(m_Material.m_Name, "TEST_GOURAUD");
	m_Material.m_pInst = &m_Inst;
	m_Material.m_nPasses = 1;
	m_Material.m_Passes[0].m_00 = 0;
	m_Material.m_Passes[0].m_10 = 1;
	m_Material.m_Passes[0].m_14 = 0;
	m_Material.m_Passes[0].m_1c = 1;
	m_Material.m_Passes[0].m_Stages[0].m_18 = 0;
	m_Material.m_Passes[0].m_Stages[0].m_1c = 0;
	m_Inst.m_pMaterial = &m_Material;
	m_Inst.m_04 = 0;
	m_pInst = &m_Inst;
}

DrawPolyMgr::~DrawPolyMgr()
{
}



// ---------------------------------------------------------------------- //
//  The texture coordinate generators (0x1004dbf0).
// ---------------------------------------------------------------------- //

// 0x1002a932
void DrawPolyMgr::GenTexCoords_Base(PolyVert* pSrc, TexPair* pDest, uint32 iStage)
{
	pDest->u = m_TexScale[iStage].u * pSrc->m_U;
	pDest->v = m_TexScale[iStage].v * pSrc->m_V;
}

// 0x1002a95c
void DrawPolyMgr::GenTexCoords_Lightmap(PolyVert* pSrc, TexPair* pDest, uint32 iStage)
{
	pDest->u = pSrc->m_LMU;
	pDest->v = pSrc->m_LMV;
}

// 0x1002a972
void DrawPolyMgr::GenTexCoords_Detail(PolyVert* pSrc, TexPair* pDest, uint32 iStage)
{
	pDest->u = m_DetailSizeU * m_DetailOffsetU + pSrc->m_pVec->x;
	pDest->v = m_DetailSizeV * m_DetailOffsetV + pSrc->m_pVec->z;
	pDest->u = m_TexScale[iStage].u * pDest->u;
	pDest->v = m_TexScale[iStage].v * pDest->v;
}

// 0x1002a9bf
void DrawPolyMgr::GenTexCoords_Scaled(PolyVert* pSrc, TexPair* pDest, uint32 iStage)
{
	pDest->u = m_DetailScaleU * pSrc->m_U;
	pDest->v = m_DetailScaleV * pSrc->m_V;
}


// ---------------------------------------------------------------------- //
//  0x1002a9e1 / 0x1002aa0d -- two and three stages of coordinates.
// ---------------------------------------------------------------------- //
void DrawPolyMgr::GenTexCoords2(void* pPoly, TexPair* pDest, void* pVert)
{
	(this->*m_GenTexCoords[0])((PolyVert*)pPoly, pDest, 0);
	(this->*m_GenTexCoords[1])((PolyVert*)pPoly, pDest + 1, 1);
}

void DrawPolyMgr::GenTexCoords3(void* pPoly, TexPair* pDest, void* pVert)
{
	(this->*m_GenTexCoords[0])((PolyVert*)pPoly, pDest, 0);
	(this->*m_GenTexCoords[1])((PolyVert*)pPoly, pDest + 1, 1);
	(this->*m_GenTexCoords[2])((PolyVert*)pPoly, pDest + 2, 2);
}


// ---------------------------------------------------------------------- //
//  The texture binders (0x1004dc10).
// ---------------------------------------------------------------------- //

// 0x1002aa4b
uint32 DrawPolyMgr::SetTexture_Base(WorldPoly* pPoly, uint32 iStage)
{
	return d3d_GetTextureScale(pPoly->m_pSurface->m_pTexture, iStage,
		&m_TexScale[iStage].u, &m_TexScale[iStage].v);
}

// 0x1002aa75
uint32 DrawPolyMgr::SetTexture_None(WorldPoly* pPoly, uint32 iStage)
{
	return 1;
}

// 0x1002aa7b
uint32 DrawPolyMgr::SetTexture_DetailScale(WorldPoly* pPoly, uint32 iStage)
{
	SharedTexture* pTexture = pPoly->m_pSurface->m_pTexture;

	if (pTexture && pTexture->m_pDetail)
	{
		if (d3d_GetTextureScale(pTexture->m_pDetail, iStage, &m_TexScale[iStage].u, &m_TexScale[iStage].v))
		{
			m_DetailScaleU = g_CV_DetailTextureScale.m_fValue * m_TexScale[iStage].u;
			m_DetailScaleV = g_CV_DetailTextureScale.m_fValue * m_TexScale[iStage].v;
		}
	}

	return 0;
}

// 0x1002aadb
uint32 DrawPolyMgr::SetTexture_Disable(WorldPoly* pPoly, uint32 iStage)
{
	d3d_DisableTexture(iStage);
	return 1;
}

// 0x1002aaeb
uint32 DrawPolyMgr::SetTexture_Shared(WorldPoly* pPoly, uint32 iStage, uint32 flags)
{
	return d3d_SetTexture(pPoly->m_pSurface->m_pTexture, iStage, 0);
}

// 0x1002ab06
uint32 DrawPolyMgr::SetTexture_Lightmap(WorldPoly* pPoly, uint32 iStage, uint32 flags)
{
	return d3d_SetLightmapTexture(pPoly, iStage);
}

// 0x1002ab18
uint32 DrawPolyMgr::SetTexture_Detail(WorldPoly* pPoly, uint32 iStage, uint32 flags)
{
	SharedTexture* pTexture = pPoly->m_pSurface->m_pTexture;

	if (pTexture && pTexture->m_pDetail)
	{
		if (d3d_SetTexture(pTexture->m_pDetail, iStage, 0))
		{
			m_DetailScaleU = g_CV_DetailTextureScale.m_fValue * g_TexelSize[iStage].u;
			m_DetailScaleV = g_CV_DetailTextureScale.m_fValue * g_TexelSize[iStage].v;
			return 1;
		}
	}

	return 0;
}

// 0x1002ab7a
uint32 DrawPolyMgr::SetTexture_None2(WorldPoly* pPoly, uint32 iStage, uint32 flags)
{
	return 1;
}

// 0x1002ab80
void DrawPolyMgr::SetColor_None(WorldPoly* pPoly, uint32* pColor)
{
	pColor[4] = 0xFFFFFFFF;
}

// 0x1002ab8b
void DrawPolyMgr::SetColor_Lightmap(PolyVert* pSrc, uint8* pDest)
{
	pDest[0x12] = g_LightTableR[pSrc->m_Color[2]];
	pDest[0x11] = g_LightTableG[pSrc->m_Color[1]];
	pDest[0x10] = g_LightTableB[pSrc->m_Color[0]];
	pDest[0x13] = g_VertexAlpha;
}

// The renderer's float-to-int (fixedpoint.h), rounding in the FPU's mode.
inline int RoundFloatToInt(float f)
{
	int nResult;

	__asm
	{
		fld f
		fistp nResult
	}

	return nResult;
}

// 0x1002abc6
void DrawPolyMgr::SetColor_Scaled(PolyVert* pSrc, uint8* pDest)
{
	pDest[0x12] = (uint8)RoundFloatToInt((float)pSrc->m_Color[2] * m_ColorScale[0]);
	pDest[0x11] = (uint8)RoundFloatToInt((float)pSrc->m_Color[1] * m_ColorScale[1]);
	pDest[0x10] = (uint8)RoundFloatToInt((float)pSrc->m_Color[0] * m_ColorScale[2]);
	pDest[0x13] = g_VertexAlpha;
}


// ---------------------------------------------------------------------- //
//  0x1002ac3c / 0x1002ac59 -- a surface's texture flags.
// ---------------------------------------------------------------------- //
void d3d_GetTextureFlags(WorldPoly* pPoly, uint32* pType, uint32* pFlags)
{
	*pFlags = (uint32)pPoly->m_pSurface->m_pTexture;
	*pType = (*pFlags >> 2) & 0x1f;
}

void d3d_GetSurfaceFlags(Surface* pSurface, uint32* pType, uint32* pFlags)
{
	*pFlags = pSurface->m_Flags;
	*pType = (*pFlags >> 2) & 0x1f;
}


// ---------------------------------------------------------------------- //
//  The clippers (0x1004dc58..0x1004dc84).
// ---------------------------------------------------------------------- //
struct SceneContext;
extern SceneContext	g_SceneContext;                     // 0x1005b790
extern uint32		g_ClipFlags;                        // 0x1005bd0c

extern int	d3d_ClipTLVerts(void* ppVerts, void* pnVerts, SceneContext* pContext, uint32 flags);     // 0x1000aec6
extern int	d3d_ClipLMVerts(void* ppVerts, void* pnVerts, SceneContext* pContext, uint32 flags);     // 0x100085f2
extern int	d3d_ClipTLVertsRC(uint32 clipFlags, void* ppVerts, void* pnVerts);    // 0x1000af61
extern int	d3d_ClipLMVertsRC(uint32 clipFlags, void* ppVerts, void* pnVerts);    // 0x10008779
extern void	d3d_DrawPolyList(void* pList, void* pVerts, uint32 nVerts);           // 0x100083ec

// 0x1002ac73
int __fastcall d3d_ClipTL(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	g_ClipFlags = clipFlags;
	return d3d_ClipTLVerts(ppVerts, pnVerts, &g_SceneContext, 0);
}

// 0x1002ac90
int __fastcall d3d_ClipLM(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	g_ClipFlags = clipFlags;
	return d3d_ClipLMVerts(ppVerts, pnVerts, &g_SceneContext, 0);
}

// 0x1002acad
int __fastcall d3d_ClipTLRC(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return d3d_ClipTLVertsRC(clipFlags, ppVerts, pnVerts);
}

// 0x1002acbe
int __fastcall d3d_ClipLMRC(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return d3d_ClipLMVertsRC(clipFlags, ppVerts, pnVerts);
}

// 0x1002accf
int __fastcall d3d_ClipNone(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return 0;
}

// 0x1002acd4
void __fastcall d3d_DrawNone(uint32 a, void* b, void* pList)
{
}

struct PolyBatch
{
	void*		m_pList;                                // +0x00
	uint8		m_pad_04[0x10 - 0x04];
	uint32		m_nVerts;                               // +0x10
	uint8		m_pad_14[0x18 - 0x14];
	uint8		m_Verts[1];                             // +0x18
};

// 0x1002acd7
void __fastcall d3d_DrawBatch(uint32 a, void* b, PolyBatch* pBatch)
{
	d3d_DrawPolyList(pBatch->m_pList, pBatch->m_Verts, pBatch->m_nVerts);
}


// ---------------------------------------------------------------------- //
//  The rest of the object's console variables, defined further down the
//  file (their initialisers, from 0x1002aea4, follow the code above).
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_TestGouraud("TestGouraud", 0.0f, LTNULL, LTNULL);	// 0x10074298  $E 0x1002aea4/0x1002aea9
CD3DConVar	g_CV_TestLightmap("TestLightmap", 1.0f, LTNULL, LTNULL);	// 0x10074278  $E 0x1002aec3/0x1002aec8
