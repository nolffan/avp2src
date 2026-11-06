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
//  Still near: the two pass draws (0x1002b22f, 0x1002b4b3: register allocation) and
//  DrawNodeFlat (0x1002b6dc: loop shape).
// *********************************************************************** //

#include <string.h>
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include "../../LT2/lithshared/stdlith/object_bank.h"
#include "d3d_clip.h"


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

struct PolyVert;

struct WorldPoly
{
	CheapLTLink		m_Link;                             // +0x00
	uint32			m_BatchInfo;                        // +0x08 (first stored vertex << 16 | count)
	uint8			m_pad_0c[0x14 - 0x0c];
	uint16			m_ClipFlags;                        // +0x14
	uint8			m_pad_16[0x2c - 0x16];
	Surface*		m_pSurface;                         // +0x2c
	void*			m_pLights;                          // +0x30
	uint8			m_pad_34[0x50 - 0x34];
	PolyVert*		m_pTJuncVerts;                      // +0x50
	uint16			m_nVerts;                           // +0x54
	uint16			m_nTJuncVerts;                      // +0x56
	uint8			m_Verts[1];                         // +0x58 (PolyVert)
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

	void		Set(uint32 colorOp, uint32 colorArg1, uint32 colorArg2,
					uint32 alphaOp, uint32 alphaArg1, uint32 alphaArg2)
	{
		m_ColorOp = colorOp;
		m_ColorArg1 = colorArg1;
		m_ColorArg2 = colorArg2;
		m_AlphaOp = alphaOp;
		m_AlphaArg1 = alphaArg1;
		m_AlphaArg2 = alphaArg2;
	}

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
	MaterialInst*	m_pNext;                            // +0x004
	PassState	m_Passes[3];                            // +0x008
};

// The element type has a constructor of its own (an empty one: BaseNew,
// 0x1002be26, constructs nothing). That is what makes CMoArray's members
// (0x1002ba2f..0x1002be26) come out as retail's at /O1: BaseNew's
// construction loop costs the inliner before it is optimised away, so
// SetSize2 expands _AllocateTArray and calls BaseNew, where a plain-data
// element gets the reverse (TU07's 16-byte CMoArray, 0x1000e742, is one).
struct PolyBatch16
{
	PolyBatch16() {}

	uint32		m_Data[4];
};

// The bank's element: a list head and two links back to the node. Its
// constructor (0x1002bb68) and destructor (0x1002bbad) are what the bank's
// Allocate and Free (0x1002bb4c, 0x1002bb86) call.
struct PolyNode                                         // 0x28 bytes
{
	PolyNode()
	{
		dl_TieOff(&m_Head);
		dl_TieOff(&m_Link1);
		m_Link1.m_pData = this;
		dl_TieOff(&m_Link2);
		m_Link2.m_pData = this;
	}

	~PolyNode()
	{
		dl_Remove(&m_Link1);
		dl_Remove(&m_Link2);
	}

	LTLink		m_Head;                                 // +0x00
	uint32		m_0c;                                   // +0x0c
	LTLink		m_Link1;                                // +0x10
	LTLink		m_Link2;                                // +0x1c
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

	void		AddPoly(WorldPoly* pPoly, uint32 iPass);
	void		SetPassStates(PassDesc* pDesc, PassState* pState);
	void		Flush();
	void		DrawPolyFirstPass(WorldPoly* pPoly, PassDesc* pDesc, uint32 iNextPass);
	void		DrawPolyAdditionalPass(WorldPoly* pPoly, PassDesc* pDesc, uint32 iNextPass);
	void		DrawNodeFlat(PolyNode* pNode);
	void		DrawNone(struct PolyBatch* pBatch);
	void		DrawBatch(struct PolyBatch* pBatch);

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
	TexCoordFn	m_GenTexCoordsAll;                      // +0x7b8
	CMoArray<PolyBatch16>			m_Batches;          // +0x7bc
	uint32		m_7d0;                                  // +0x7d0
	ObjectBank<PolyNode, NullCS>	m_NodeBank;         // +0x7d4
};

extern LTBOOL	d3d_GetTextureScale(SharedTexture* pTexture, uint32 iStage, float* pU, float* pV);  // 0x10022760
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
	m_pNext = LTNULL;
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
	m_Inst.m_pNext = LTNULL;
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
struct ViewParams;
extern ViewParams	g_ViewParams;                     // 0x1005b790
extern uint32		g_ClipFlags;                        // 0x1005bd0c

extern int	d3d_ClipTLVerts(void* ppVerts, void* pnVerts, ViewParams* pContext, uint32 flags);     // 0x1000aec6
struct RenderPoly;
extern LTBOOL	d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pContext, uint32 flags);  // 0x100085f2 (drawmodel_clip.cpp)
extern int	d3d_ClipTLVertsRC(uint32 clipFlags, void* ppVerts, void* pnVerts);    // 0x1000af61
extern void	d3d_LightLMPoly(RenderPoly* pPoly, ClipVert2* pVerts, uint32 nVerts);  // 0x100083ec (drawmodel_clip.cpp)

// 0x1002ac73
int __fastcall d3d_ClipTL(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	g_ClipFlags = clipFlags;
	return d3d_ClipTLVerts(ppVerts, pnVerts, &g_ViewParams, 0);
}

// 0x1002ac90
int __fastcall d3d_ClipLM(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	g_ClipFlags = clipFlags;
	return d3d_ClipLMVerts((ClipVert2**)ppVerts, (int32*)pnVerts, &g_ViewParams, 0);
}

// 0x1002acad
int __fastcall d3d_ClipTLRC(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return d3d_ClipTLVertsRC(clipFlags, ppVerts, pnVerts);
}

// 0x1002acbe -- ClipPoly<ClipVert2> (d3d_clip.h; its copy is TU04's
// 0x10008779).
int __fastcall d3d_ClipLMRC(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return ClipPoly(clipFlags, (ClipVert2**)ppVerts, (int32*)pnVerts);
}

// 0x1002accf
int __fastcall d3d_ClipNone(uint32 clipFlags, void* ppVerts, void* pnVerts)
{
	return 0;
}

// A polygon's vertices as a pass draws them, and what its stages bound.
#define MAX_PASS_VERTS		128

struct PolyBatch
{
	WorldPoly*	m_pList;                                // +0x00
	uint32		m_TexResult[3];                         // +0x04
	int			m_nVerts;                               // +0x10
	uint32		m_VertSize;                             // +0x14
	uint8		m_Verts[MAX_PASS_VERTS * 0x28];         // +0x18
};

// 0x1002acd4
void DrawPolyMgr::DrawNone(PolyBatch* pBatch)
{
}

// 0x1002acd7
void DrawPolyMgr::DrawBatch(PolyBatch* pBatch)
{
	d3d_LightLMPoly((RenderPoly*)pBatch->m_pList, (ClipVert2*)pBatch->m_Verts, pBatch->m_nVerts);
}


// ---------------------------------------------------------------------- //
//  0x1002acef -- put a polygon on a pass's list: on the node for its
//  texture, made when there is none yet.
// ---------------------------------------------------------------------- //
typedef void (*GetFlagsFn)(WorldPoly* pPoly, uint32* pType, uint32* pFlags);
extern GetFlagsFn g_GetFlagsFns[];

void DrawPolyMgr::AddPoly(WorldPoly* pPoly, uint32 iPass)
{
	MaterialInst* pInst;
	LTLink* pList;
	LTLink* pCur;
	PolyNode* pNode;
	uint32 type, flags;

	pInst = m_Material.m_pInst;
	g_GetFlagsFns[m_Material.m_Passes[iPass].m_18](pPoly, &type, &flags);

	pList = &pInst->m_Passes[iPass].m_Lists[type];
	for (pCur = pList->m_pNext; pCur != pList; pCur = pCur->m_pNext)
	{
		pNode = (PolyNode*)pCur->m_pData;
		if (pNode->m_0c == flags)
		{
			dl_Insert(&pNode->m_Head, &pPoly->m_Link);
			return;
		}
	}

	pNode = m_NodeBank.Allocate();
	if (!pNode)
		return;

	pNode->m_0c = flags;
	dl_Insert(pList, &pNode->m_Link1);
	dl_Insert(&pInst->m_Passes[iPass].m_Pending, &pNode->m_Link2);
	dl_Insert(&pNode->m_Head, &pPoly->m_Link);
}


// ---------------------------------------------------------------------- //
//  0x1002adba -- a pass's blend and texture stage states.
// ---------------------------------------------------------------------- //
extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8

void DrawPolyMgr::SetPassStates(PassDesc* pDesc, PassState* pState)
{
	TexStage* pStage;
	uint32 i;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, pDesc->m_00);
	if (pDesc->m_00)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, pState->m_180);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, pState->m_184);
	}

	for (i=0; i < pDesc->m_1c; i++)
	{
		pStage = &pState->m_Stages[i];

		g_pD3DDevice->SetTextureStageState(i, D3DTSS_COLOROP, pStage->m_ColorOp);
		if (pStage->m_ColorOp != D3DTOP_DISABLE)
		{
			g_pD3DDevice->SetTextureStageState(i, D3DTSS_COLORARG1, pStage->m_ColorArg1);
			g_pD3DDevice->SetTextureStageState(i, D3DTSS_COLORARG2, pStage->m_ColorArg2);
		}

		g_pD3DDevice->SetTextureStageState(i, D3DTSS_ALPHAOP, pStage->m_AlphaOp);
		if (pStage->m_AlphaOp != D3DTOP_DISABLE)
		{
			g_pD3DDevice->SetTextureStageState(i, D3DTSS_ALPHAARG1, pStage->m_AlphaArg1);
			g_pD3DDevice->SetTextureStageState(i, D3DTSS_ALPHAARG2, pStage->m_AlphaArg2);
		}
	}
}


// ---------------------------------------------------------------------- //
//  The rest of the object's console variables, defined further down the
//  file (their initialisers, from 0x1002aea4, follow the code above).
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_TestGouraud("TestGouraud", 0.0f, LTNULL, LTNULL);	// 0x10074298  $E 0x1002aea4/0x1002aea9
CD3DConVar	g_CV_TestLightmap("TestLightmap", 1.0f, LTNULL, LTNULL);	// 0x10074278  $E 0x1002aec3/0x1002aec8


// ---------------------------------------------------------------------- //
//  The per-pass function tables (0x1004dbe8..0x1004dc90).
// ---------------------------------------------------------------------- //
struct TLVertex;
typedef uint32 (DrawPolyMgr::*SetTextureFn)(WorldPoly* pPoly, uint32 iStage);
typedef uint32 (DrawPolyMgr::*SetTextureFn2)(WorldPoly* pPoly, uint32 iStage, uint32 flags);
typedef void (DrawPolyMgr::*SetColorFn)(PolyVert* pSrc, uint8* pDest);
typedef void (__fastcall *SpecularFn)(TLVertex* pVert, uint32* pSpecular);
typedef int (__fastcall *ClipFn)(uint32 clipFlags, void* ppVerts, void* pnVerts);
typedef void (DrawPolyMgr::*DrawBatchFn)(PolyBatch* pBatch);

extern SpecularFn	g_pfnSetupSpecular;                 // 0x1005e1c4
extern SpecularFn	g_pfnSetupFogSpecular;              // 0x1005e6d8

GetFlagsFn g_GetFlagsFns[2] =                           // 0x1004dbe8
{
	d3d_GetTextureFlags,
	(GetFlagsFn)d3d_GetSurfaceFlags,
};

TexCoordFn g_TexCoordFns[4] =                           // 0x1004dbf0
{
	&DrawPolyMgr::GenTexCoords_Base,
	&DrawPolyMgr::GenTexCoords_Lightmap,
	&DrawPolyMgr::GenTexCoords_Detail,
	&DrawPolyMgr::GenTexCoords_Scaled,
};

TexCoordFn g_MultiTexCoordFns[4] =                      // 0x1004dc00
{
	LTNULL,
	LTNULL,
	(TexCoordFn)&DrawPolyMgr::GenTexCoords2,
	(TexCoordFn)&DrawPolyMgr::GenTexCoords3,
};

SetTextureFn g_SetTextureFns[4] =                       // 0x1004dc10
{
	&DrawPolyMgr::SetTexture_Base,
	&DrawPolyMgr::SetTexture_None,
	&DrawPolyMgr::SetTexture_DetailScale,
	&DrawPolyMgr::SetTexture_Disable,
};

SetTextureFn2 g_SetTextureFns2[4] =                     // 0x1004dc20
{
	&DrawPolyMgr::SetTexture_Shared,
	&DrawPolyMgr::SetTexture_Lightmap,
	&DrawPolyMgr::SetTexture_Detail,
	&DrawPolyMgr::SetTexture_None2,
};

SetColorFn g_SetColorFns[3] =                           // 0x1004dc30
{
	(SetColorFn)&DrawPolyMgr::SetColor_None,
	&DrawPolyMgr::SetColor_Lightmap,
	&DrawPolyMgr::SetColor_Scaled,
};

SpecularFn* g_SpecularFns[3] =                          // 0x1004dc3c
{
	&g_pfnSetupSpecular,
	&g_pfnSetupFogSpecular,
};

// The rest by stage count (1..3).
uint32 g_PassVertSizes[4] = { 0x20, 0x28, 0x30 };       // 0x1004dc48

ClipFn g_ClipFns[4] =                                   // 0x1004dc58
{
	d3d_ClipTL,
	d3d_ClipLM,
	d3d_ClipNone,
};

ClipFn g_ClipFnsRC[4] =                                 // 0x1004dc68
{
	d3d_ClipTLRC,
	d3d_ClipLMRC,
	d3d_ClipNone,
};

DrawBatchFn g_DrawBatchFns[4] =                         // 0x1004dc78
{
	&DrawPolyMgr::DrawNone,
	&DrawPolyMgr::DrawBatch,
	&DrawPolyMgr::DrawNone,
};

uint32 g_PassFVFs[3] = { 0x1c4, 0x2c4, 0x3c4 };         // 0x1004dc88


// ---------------------------------------------------------------------- //
//  0x1002aee2 -- draw everything queued, a material, a pass and a
//  texture at a time.
// ---------------------------------------------------------------------- //
struct RenderWorld
{
	uint8		m_pad_00[0x0c];
	uint16		m_FrameCode;                            // +0x0c
};

extern RenderWorld*	g_pRenderWorld;                     // 0x1005bd1c
extern int32		g_CV_FixTJunc;                      // 0x1005dbb4
extern void		AddDebugMessage(int level, const char* pMsg, ...);  // 0x10013b13
extern void		dd_ConsolePrint(const char* pMsg, ...); // 0x10013ade

void DrawPolyMgr::Flush()
{
	MaterialInst* pInst;
	Material* pMaterial;
	PassDesc* pDesc;
	PassState* pState;
	LTLink* pPending;
	LTLink* pLink;
	LTLink* pNextLink;
	PolyNode* pNode;
	WorldPoly* pFirst;
	CheapLTLink* pCur;
	CheapLTLink* pNext;
	uint32 iPass, iStage, iNextPass;
	uint32 oldAlphaBlend, oldSrcBlend, oldDestBlend;

	m_768 = g_pRenderWorld->m_FrameCode;

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, (unsigned long*)&oldAlphaBlend);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, (unsigned long*)&oldSrcBlend);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_DESTBLEND, (unsigned long*)&oldDestBlend);

	if (g_CV_TestGouraud.m_nValue)
	{
		m_Material.m_Passes[0].m_00 = 0;
		m_Material.m_Passes[0].m_14 = 0;
		m_Material.m_nPasses = 1;
		m_Material.m_Passes[0].m_10 = 1;
		m_Material.m_Passes[0].m_18 = 0;
		m_Material.m_Passes[0].m_1c = 1;
		m_Material.m_Passes[0].m_Stages[0].m_18 = 0;
		m_Material.m_Passes[0].m_Stages[0].m_1c = 0;
	}
	else if (g_CV_TestLightmap.m_nValue)
	{
		m_Material.m_nPasses = 2;
		m_Material.m_Passes[0].m_00 = 0;
		m_Material.m_Passes[0].m_10 = 0;
		m_Material.m_Passes[0].m_14 = 0;
		m_Material.m_Passes[0].m_18 = 0;
		m_Material.m_Passes[0].m_1c = 1;
		m_Material.m_Passes[0].m_Stages[0].m_18 = 0;
		m_Material.m_Passes[0].m_Stages[0].m_1c = 0;

		m_Material.m_Passes[1].m_00 = 1;
		m_Inst.m_Passes[1].m_180 = D3DBLEND_DESTCOLOR;
		m_Inst.m_Passes[1].m_184 = D3DBLEND_SRCCOLOR;
		m_Material.m_Passes[1].m_10 = 0;
		m_Material.m_Passes[1].m_14 = 0;
		m_Material.m_Passes[1].m_18 = 1;
		m_Material.m_Passes[1].m_1c = 1;
		m_Material.m_Passes[1].m_Stages[0].m_18 = 1;
		m_Material.m_Passes[1].m_Stages[0].m_1c = 1;
	}
	else
	{
		m_Material.m_Passes[0].m_00 = 0;
		m_Material.m_nPasses = 1;
		m_Material.m_Passes[0].m_10 = 0;
		m_Material.m_Passes[0].m_14 = 0;
		m_Material.m_Passes[0].m_1c = 2;
		m_Material.m_Passes[0].m_Stages[0].m_18 = 0;
		m_Material.m_Passes[0].m_Stages[0].m_1c = 0;
		m_Material.m_Passes[0].m_18 = 0;

		m_Inst.m_Passes[0].m_Stages[0].Set(D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE,
			D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);

		m_Material.m_Passes[0].m_Stages[1].m_18 = 1;
		m_Material.m_Passes[0].m_Stages[1].m_1c = 1;

		m_Inst.m_Passes[0].m_Stages[1].Set(D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT,
			D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
	}

	m_7d0 = 0;

	pInst = m_pInst;
	while (pInst)
	{
		pMaterial = pInst->m_pMaterial;
		for (iPass=0; iPass < pMaterial->m_nPasses; iPass++)
		{
			pDesc = &pMaterial->m_Passes[iPass];
			pState = &pInst->m_Passes[iPass];
			pPending = &pState->m_Pending;
			if (pState->m_Pending.m_pNext == pPending)
				break;

			SetPassStates(pDesc, pState);

			if (pDesc->m_1c == 1)
			{
				m_GenTexCoordsAll = g_TexCoordFns[pDesc->m_Stages[0].m_18];
			}
			else
			{
				for (iStage=0; iStage < pDesc->m_1c; iStage++)
					m_GenTexCoords[iStage] = g_TexCoordFns[pDesc->m_Stages[iStage].m_18];

				m_GenTexCoordsAll = g_MultiTexCoordFns[pDesc->m_1c];
			}

			for (pLink = pPending->m_pNext; pLink != pPending; pLink = pNextLink)
			{
				pNextLink = pLink->m_pNext;
				pNode = (PolyNode*)pLink->m_pData;

				pFirst = (WorldPoly*)pNode->m_Head.m_pNext;
				if ((LTLink*)pFirst != &pNode->m_Head)
				{
					for (iStage=0; iStage < pDesc->m_1c; iStage++)
					{
						if (!(this->*g_SetTextureFns[pDesc->m_Stages[iStage].m_1c])(pFirst, iStage))
						{
							AddDebugMessage(1, "Material '%s', pass %d, stage %d: TextureSrcInitFn failed.",
								pMaterial, iPass, iStage);

							if (iPass == 0)
								DrawNodeFlat(pNode);

							goto NextNode;
						}
					}

					if (iPass == pMaterial->m_nPasses - 1)
						iNextPass = 0;
					else
						iNextPass = iPass + 1;

					if (iPass == 0)
					{
						for (pCur = pNode->m_Head.m_pNext; pCur != &pNode->m_Head; pCur = pNext)
						{
							pNext = pCur->m_pNext;
							DrawPolyFirstPass((WorldPoly*)pCur, pDesc, iNextPass);
						}
					}
					else
					{
						for (pCur = pNode->m_Head.m_pNext; pCur != &pNode->m_Head; pCur = pNext)
						{
							pNext = pCur->m_pNext;
							DrawPolyAdditionalPass((WorldPoly*)pCur, pDesc, iNextPass);
						}
					}
				}

NextNode:
				m_NodeBank.Free(pNode);
			}
		}

		pInst = pInst->m_pNext;
	}

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldAlphaBlend);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldSrcBlend);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, oldDestBlend);
}


// ---------------------------------------------------------------------- //
//  0x1002b22f -- a polygon's first pass: build its vertices, clip them,
//  draw, and keep the projected positions for the passes after.
// ---------------------------------------------------------------------- //
inline void d3d_GetPolyVerts(WorldPoly* pPoly, PolyVert** ppVerts, int* pnVerts)
{
	if (g_CV_FixTJunc)
	{
		*ppVerts = pPoly->m_pTJuncVerts;
		*pnVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		*ppVerts = (PolyVert*)pPoly->m_Verts;
		*pnVerts = pPoly->m_nVerts;
	}
}

struct StoredVert
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
};

void DrawPolyMgr::DrawPolyFirstPass(WorldPoly* pPoly, PassDesc* pDesc, uint32 iNextPass)
{
	PolyBatch batch;
	PolyVert* pPolyVerts;
	uint8* pOut;
	uint8* pVerts;
	StoredVert* pStored;
	int i;
	uint32 iStage;
	SetColorFn pSetColor;
	SpecularFn pSpecular;

	batch.m_pList = pPoly;
	d3d_GetPolyVerts(pPoly, &pPolyVerts, &batch.m_nVerts);

	batch.m_VertSize = g_PassVertSizes[pDesc->m_1c - 1];
	pSetColor = g_SetColorFns[pDesc->m_10];
	pSpecular = *g_SpecularFns[pDesc->m_14];

	pOut = batch.m_Verts;
	for (i=0; i < batch.m_nVerts; i++)
	{
		*(LTVector*)pOut = *pPolyVerts->m_pVec;
		(this->*pSetColor)(pPolyVerts, pOut);
		pSpecular((TLVertex*)pOut, (uint32*)(pOut + 0x14));
		(this->*m_GenTexCoordsAll)(pPolyVerts, (TexPair*)(pOut + 0x18), 0);
		pPolyVerts++;
		pOut += batch.m_VertSize;
	}

	batch.m_TexResult[2] = 0;
	batch.m_TexResult[1] = 0;
	batch.m_TexResult[0] = 0;

	if (pPoly->m_pLights && pDesc->m_0c)
	{
		for (iStage=0; iStage < pDesc->m_1c; iStage++)
			(this->*g_DrawBatchFns[pDesc->m_Stages[iStage].m_20])(&batch);
	}

	pVerts = batch.m_Verts;
	if (!g_ClipFns[pDesc->m_1c - 1](pPoly->m_ClipFlags & 0x3f, &pVerts, &batch.m_nVerts))
		return;

	for (iStage=0; iStage < pDesc->m_1c; iStage++)
		(this->*g_SetTextureFns2[pDesc->m_Stages[iStage].m_1c])(pPoly, iStage, batch.m_TexResult[iStage]);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, g_PassFVFs[pDesc->m_1c - 1], pVerts, batch.m_nVerts, 0);

	if (!iNextPass)
		return;

	if (m_Batches.GetSize() - m_7d0 < (uint32)batch.m_nVerts)
	{
		if (!m_Batches.NiceSetSize(m_Batches.GetSize() + 0x100))
			return;
	}

	pPoly->m_BatchInfo = (m_7d0 << 16) | (batch.m_nVerts & 0xFFFF);
	if (pVerts == batch.m_Verts)
		pPoly->m_ClipFlags &= ~0x3f;

	pStored = (StoredVert*)&m_Batches[m_7d0];
	for (i=0; i < batch.m_nVerts; i++)
	{
		pStored->m_Vec = *(LTVector*)pVerts;
		pStored->m_RHW = ((float*)pVerts)[3];
		pVerts += batch.m_VertSize;
		pStored++;
	}

	m_7d0 += batch.m_nVerts;
	AddPoly(pPoly, iNextPass);
}


// ---------------------------------------------------------------------- //
//  0x1002b4b3 -- a later pass: the first pass's projected positions with
//  this pass's colours and coordinates.
// ---------------------------------------------------------------------- //
void DrawPolyMgr::DrawPolyAdditionalPass(WorldPoly* pPoly, PassDesc* pDesc, uint32 iNextPass)
{
	uint8 verts[MAX_PASS_VERTS * 0x28];
	PolyVert* pPolyVerts;
	uint8* pOut;
	uint8* pVerts;
	StoredVert* pStored;
	int nVerts, nStored, i;
	uint32 iStage, vertSize;
	SetColorFn pSetColor;
	SpecularFn pSpecular;

	vertSize = g_PassVertSizes[pDesc->m_1c - 1];
	pSetColor = g_SetColorFns[pDesc->m_10];
	pSpecular = *g_SpecularFns[pDesc->m_14];

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	nStored = (uint16)pPoly->m_BatchInfo;
	pStored = (StoredVert*)&m_Batches.GetArray()[pPoly->m_BatchInfo >> 16];
	pVerts = verts;

	if (pPoly->m_ClipFlags & 0x3f)
	{
		for (i=0; i < nVerts; i++)
		{
			*(LTVector*)pVerts = *pPolyVerts->m_pVec;
			(this->*pSetColor)(pPolyVerts, pVerts);
			pSpecular((TLVertex*)pVerts, (uint32*)(pVerts + 0x14));
			(this->*m_GenTexCoordsAll)(pPolyVerts, (TexPair*)(pVerts + 0x18), 0);
			pPolyVerts++;
			pVerts += vertSize;
		}

		pVerts = verts;
		if (!g_ClipFns[pDesc->m_1c - 1](pPoly->m_ClipFlags & 0x3f, &pVerts, &nVerts))
			return;

		if (nStored != nVerts)
		{
			dd_ConsolePrint("DrawPolyMgr::DrawPolyAdditionalPass: nQVerts != nVertices");
			return;
		}

		pOut = verts;
		for (i=0; i < nVerts; i++)
		{
			*(LTVector*)pOut = pStored->m_Vec;
			((float*)pOut)[3] = pStored->m_RHW;
			pStored++;
			pOut += vertSize;
		}
	}
	else
	{
		pOut = verts;
		for (i=0; i < nVerts; i++)
		{
			*(LTVector*)pOut = pStored->m_Vec;
			((float*)pOut)[3] = pStored->m_RHW;
			(this->*pSetColor)(pPolyVerts, pOut);
			pSpecular((TLVertex*)pOut, (uint32*)(pOut + 0x14));
			(this->*m_GenTexCoordsAll)(pPolyVerts, (TexPair*)(pOut + 0x18), 0);
			pStored++;
			pOut += vertSize;
			pPolyVerts++;
		}
	}

	for (iStage=0; iStage < pDesc->m_1c; iStage++)
		(this->*g_SetTextureFns2[pDesc->m_Stages[iStage].m_1c])(pPoly, iStage, 0);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, g_PassFVFs[pDesc->m_1c - 1], pVerts, nVerts, 0);

	if (iNextPass)
		AddPoly(pPoly, iNextPass);
}


// ---------------------------------------------------------------------- //
//  0x1002b6dc -- a node whose texture would not bind: its polygons
//  untextured, each in a colour of its own.
// ---------------------------------------------------------------------- //
struct FlatVert
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
	uint32		color;                                  // +0x10
	uint32		specular;                               // +0x14
	float		tu, tv;                                 // +0x18
};

void DrawPolyMgr::DrawNodeFlat(PolyNode* pNode)
{
	FlatVert verts[MAX_PASS_VERTS];
	int nVerts, i;
	FlatVert* pVerts;
	PolyVert* pPolyVerts;
	WorldPoly* pPoly;

	d3d_DisableTexture(0);

	pPoly = (WorldPoly*)pNode->m_Head.m_pNext;
	while ((LTLink*)pPoly != &pNode->m_Head)
	{
		d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);
		if (nVerts > MAX_PASS_VERTS)
		{
			dd_ConsolePrint("Error: vertex buffer overflow");
			return;
		}

		pVerts = verts;
		for (i=0; i < nVerts; i++)
		{
			pVerts[i].m_Vec = *pPolyVerts->m_pVec;
			pVerts[i].color = (uint32)pPoly;
			pPolyVerts++;
		}

		g_ClipFlags = 0x3f;
		if (d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0x1c4, pVerts, nVerts, 0);

		pPoly = (WorldPoly*)pPoly->m_Link.m_pNext;
	}
}
